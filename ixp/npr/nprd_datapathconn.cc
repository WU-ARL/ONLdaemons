/*
 * Copyright (c) 2009-2013 Charlie Wiseman, Jyoti Parwatikar, John DeHart 
 * and Washington University in St. Louis
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 *    limitations under the License.
 *
 */

#include <iostream>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <list>
#include <vector>
#include <map>
#include <exception>
#include <stdexcept>

#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <pthread.h>

#include "shared.h"

#include "nprd_types.h"
#include "nprd_configuration.h"
#include "nprd_datapathmessage.h"
#include "nprd_datapathconn.h"

#include "halMev2Api.h"
#include "hal_sram.h"
#include "hal_scratch.h"

using namespace npr;

DataPathConn::DataPathConn() throw()
{
  pthread_mutex_init(&read_lock, NULL);
  pthread_mutex_init(&write_lock, NULL);

  msg_ready = false;
  source = INVALIDSRC;
  first_word = 0;
}

DataPathConn::~DataPathConn() throw()
{
  pthread_mutex_destroy(&read_lock);
  pthread_mutex_destroy(&write_lock);
}

bool DataPathConn::check() throw()
{
  unsigned int val;

  autoLock rlock(read_lock);

  if(msg_ready) return false;

  // JDD: 9/29/2010 changed order of ring processing. Do ERR first.
  //                that is where ARP pkts should be
  val = SCRATCH_RING_GET(ERRORS_RING);
  if(val != 0) 
  { 
    char logstr[256];
    sprintf(logstr, "DataPathConn:check(): pkt recv'd from ERR Ring");
    write_log(logstr);

    msg_ready=true; 
    source=ERRORS; 
    first_word=val; 
    return true; 
  }

  val = SCRATCH_RING_GET(LOCAL_DELIVERY_RING);
  if(val != 0)
  {
    char logstr[256];
    sprintf(logstr, "DataPathConn:check(): pkt recv'd from LD  Ring");
    write_log(logstr);

    msg_ready=true; 
    source=LOCAL_DELIVERY; 
    first_word=val; 
    return true; 
  }

  val = SCRATCH_RING_GET(EXCEPTIONS_RING);
  if(val != 0) 
  { 
    char logstr[256];
    sprintf(logstr, "DataPathConn:check(): pkt recv'd from EXC Ring");
    write_log(logstr);

    msg_ready=true; 
    source=EXCEPTIONS; 
    first_word=val; 
    return true; 
  }

  return 0;
}

npr::DataPathMessage *DataPathConn::readmsg() throw(datapathconn_exception)
{
  autoLock rlock(read_lock);


  unsigned int ring;
  if(source == LOCAL_DELIVERY) ring=LOCAL_DELIVERY_RING;
  else if(source == EXCEPTIONS) ring=EXCEPTIONS_RING;
  else if(source == ERRORS) ring=ERRORS_RING;
  else
  {
    first_word = 0;
    source = INVALIDSRC;
    msg_ready = false;
    throw datapathconn_exception("source is invalid! something's really broken..");
  }

  unsigned int num_words = PLC_TO_XSCALE_DATA_NUM_WORDS;
  unsigned int *bytes = new unsigned int[num_words];
  bytes[0] = first_word;

  char logstr[256];
  sprintf(logstr, "readmsg: msg word %d/%d: %.8x", 0, num_words-1, bytes[0]);
  write_log(logstr);

  for(unsigned int i=1; i<num_words; ++i)
  {
    bytes[i] = SCRATCH_RING_GET(ring);

    sprintf(logstr, "readmsg: ring %d msg word %d/%d: %.8x", ring, i, num_words-1, bytes[i]);
    write_log(logstr);
  }

/*
  write_log(LOG_VERBOSE, "blah");
  write_log(LOG_VERBOSE, "readmsg: about to allocate new data path message");
  write_log(LOG_VERBOSE, "blah2");

  int* foo = new int;

  *foo = 0;
*/
  DataPathMessage *msg = new DataPathMessage(bytes, MSG_FROM_PLC);

  delete[] bytes;
//  delete foo;

  first_word = 0;
  source = INVALIDSRC;
  msg_ready = false;

  write_log("readmsg: done");

  return msg;
}

void DataPathConn::writemsg(npr::DataPathMessage *msg) throw(datapathconn_exception)
{
  autoLock wlock(write_lock);

  unsigned int num_words = msg->getnumwords();
  unsigned int dest = msg->getmsgassociation();
  unsigned int val;

  if(dest == MSG_TO_MUX)
  {
    write_log("writemsg: msg to MUX, sram bank " + int2str(SRAM_BANK_1) + ", ring " + int2str(MUX_RING));

    // if the current occupancy is full, then drop the packet
    unsigned int ring_occ = SCRATCH_READ(XSCALE_TO_MUXQM_OCCUPANCY_CNTR);
    if(ring_occ >= (ONL_SRAM_RINGS_SIZE / num_words) - num_words)
    {
      while(SCRATCH_RING_GET_IS_FULL_STATUS(FREELIST_RING)) {}
      SCRATCH_RING_PUT(FREELIST_RING,msg->getword(0));
      return;
    }

    for(unsigned int i=0; i<num_words; ++i)
    {
      val = msg->getword(i);

      char logstr[256];
      sprintf(logstr, "writemsg: msg to MUX, word %u: 0x%.8x", i, val);
      write_log(logstr);

      SRAM_RING_PUT(SRAM_BANK_1,MUX_RING,&val);

      sprintf(logstr, "writemsg: msg to MUX, got result 0x%.8x", val);
      write_log(logstr);
    }
    if(halMe_AtomicAdd(HALME_MEM_SCRATCH, XSCALE_TO_MUXQM_OCCUPANCY_CNTR, 1) != HALME_SUCCESS)
    {
      throw datapathconn_exception("scratch increment failed!");
    }
  }
  else if(dest == MSG_TO_PLUGIN_0 || dest == MSG_TO_PLUGIN_1 || dest == MSG_TO_PLUGIN_2 || dest == MSG_TO_PLUGIN_3 || dest == MSG_TO_PLUGIN_4)
  {
    // if the current occupancy is full, then drop the packet
    unsigned int ring_occ = SCRATCH_READ(XSCALE_TO_MUXPL_OCCUPANCY_CNTR);
    if(ring_occ >= (ONL_SRAM_RINGS_SIZE / num_words) - num_words)
    {
      while(SCRATCH_RING_GET_IS_FULL_STATUS(FREELIST_RING)) {}
      SCRATCH_RING_PUT(FREELIST_RING,msg->getword(0));
      return;
    }

    for(unsigned int i=0; i<num_words; ++i)
    {
      val = msg->getword(i);
      SRAM_RING_PUT(SRAM_BANK_1,MUX_PLUGIN_RING,&val);
    }
    if(halMe_AtomicAdd(HALME_MEM_SCRATCH, XSCALE_TO_MUXPL_OCCUPANCY_CNTR, 1) != HALME_SUCCESS)
    {
      throw datapathconn_exception("scratch increment failed!");
    }
  }
  else // dest is freelist manager
  { 
    while(SCRATCH_RING_GET_IS_FULL_STATUS(FREELIST_RING)) {}
    SCRATCH_RING_PUT(FREELIST_RING,msg->getword(0));
  }
}
