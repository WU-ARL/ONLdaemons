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
#include "nprd_pluginmessage.h"
#include "nprd_pluginconn.h"

#include "halMev2Api.h"
#include "hal_sram.h"
#include "hal_scratch.h"

using namespace npr;

PluginConn::PluginConn() throw()
{
  pthread_mutex_init(&read_lock, NULL);
  pthread_mutex_init(&write_lock, NULL);

  msg_ready = false;
  plugin = -1;
  hdr.v = 0;
}

PluginConn::~PluginConn() throw()
{
  pthread_mutex_destroy(&read_lock);
  pthread_mutex_destroy(&write_lock);
}

bool PluginConn::check() throw()
{
  unsigned int val;

  autoLock rlock(read_lock);

  if(msg_ready) return false;
  
  val = SRAM_RING_GET(SRAM_BANK_3,FROM_PLUGIN_0_CONTROL_RING);
  if(val != 0) { msg_ready=true; plugin=0; hdr.v=val; return true; }
  
  val = SRAM_RING_GET(SRAM_BANK_3,FROM_PLUGIN_1_CONTROL_RING);
  if(val != 0) { msg_ready=true; plugin=1; hdr.v=val; return true; }
  
  val = SRAM_RING_GET(SRAM_BANK_3,FROM_PLUGIN_2_CONTROL_RING);
  if(val != 0) { msg_ready=true; plugin=2; hdr.v=val; return true; }
  
  val = SRAM_RING_GET(SRAM_BANK_3,FROM_PLUGIN_3_CONTROL_RING);
  if(val != 0) { msg_ready=true; plugin=3; hdr.v=val; return true; }
  
  val = SRAM_RING_GET(SRAM_BANK_3,FROM_PLUGIN_4_CONTROL_RING);
  if(val != 0) { msg_ready=true; plugin=4; hdr.v=val; return true; }
  
  return false;
}

npr::PluginMessage *PluginConn::readmsg() throw(pluginconn_exception,pluginmessage_exception)
{
  autoLock rlock(read_lock);
  
  unsigned int ring;
  if(plugin == 0) ring=FROM_PLUGIN_0_CONTROL_RING;
  else if(plugin == 1) ring=FROM_PLUGIN_1_CONTROL_RING;
  else if(plugin == 2) ring=FROM_PLUGIN_2_CONTROL_RING;
  else if(plugin == 3) ring=FROM_PLUGIN_3_CONTROL_RING;
  else if(plugin == 4) ring=FROM_PLUGIN_4_CONTROL_RING;
  else
  {
    plugin = -1;
    hdr.v = 0;
    msg_ready = false;
    throw pluginconn_exception("system error");
  }

  unsigned int *bytes = new unsigned int[hdr.num_words + 1];
  bytes[0] = hdr.v;
  for(unsigned int i=1; i<=hdr.num_words; ++i)
  {
    bytes[i] = SRAM_RING_GET(SRAM_BANK_3,ring);
  }

  PluginMessage *msg = new PluginMessage(bytes, plugin);

  plugin = -1;
  hdr.v = 0;
  msg_ready = false;

  delete[] bytes;

  return msg;
}

void PluginConn::writemsg(npr::PluginMessage *msg) throw(pluginconn_exception,pluginmessage_exception)
{
  autoLock wlock(write_lock);

  write_log("PluginConn:writemsg:");

  unsigned int num_words = msg->getnumwords() + 1;
  unsigned int plugin = msg->getplugin();
  unsigned int val;

  write_log("PluginConn:writemsg: num_words =" + int2str(num_words) + ", plugin = " + int2str(plugin));

  unsigned int ring;
  if(plugin == 0) ring=TO_PLUGIN_0_CONTROL_RING;
  else if(plugin == 1) ring=TO_PLUGIN_1_CONTROL_RING;
  else if(plugin == 2) ring=TO_PLUGIN_2_CONTROL_RING;
  else if(plugin == 3) ring=TO_PLUGIN_3_CONTROL_RING;
  else if(plugin == 4) ring=TO_PLUGIN_4_CONTROL_RING;
  else { throw pluginconn_exception("invalid plugin"); }

  write_log("PluginConn::writemsg: ring = " + int2str(ring));

  for(unsigned int i=0; i<num_words; ++i)
  {
    val = msg->getword(i);
    char logstr[256];
    sprintf(logstr, "PluginConn:writemsg: word(%u) = 0x%.8x", i, val);
    write_log(logstr);
    SRAM_RING_PUT(SRAM_BANK_3,ring,&val); 
  }
}
