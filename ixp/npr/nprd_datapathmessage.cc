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
#include "nprd_datapathmessage.h"

using namespace npr;

DataPathMessage::DataPathMessage(unsigned int *msgwords, unsigned int a) throw(datapathmessage_exception)
{
  if(a > MAX_MSG_ASSOC)
  {
    throw datapathmessage_exception("invalid assocation!");
  } 

  association = a;

  unsigned int num_words = getnumwords();
  msg = new unsigned int[num_words];
  for(unsigned int i=0; i<num_words; ++i)
  {
    msg[i] = msgwords[i];
  }

  write_log("DataPathMessage::DataPathMessage: new message has assoc " + int2str(association));
}

DataPathMessage::~DataPathMessage() throw()
{
  write_log("DataPathMessage::DataPathMessage: deleting message with assoc " + int2str(association));

  delete[] msg;
}

unsigned int DataPathMessage::type() throw()
{
  write_log("DataPathMessage::type: entering");
  if(association == MSG_FROM_PLC)
  {
    write_log("DataPathMessage::type: msg is from PLC");

    plc_to_xscale_data *ptxd = (plc_to_xscale_data *)msg;
    if(ptxd->arp_needed == 1)
    {
      if(ptxd->arp_source == 1)
      {
        write_log("DataPathMessage::type: msg needs ARP for destination IP");
        return DP_ARP_NEEDED_DIP;
      }
      else
      {
        write_log("DataPathMessage::type: msg needs ARP for next hop router");
        return DP_ARP_NEEDED_NHR;
      }
    }
    if(ptxd->ttl_expired == 1)
    { 
      write_log("DataPathMessage::type: msg has expired ttl");
      return DP_TTL_EXPIRED;
    }
    if(ptxd->ip_options == 1)
    {
      write_log("DataPathMessage::type: msg has IP options");
      return DP_IP_OPTIONS;
    }
    if(ptxd->no_route == 1)
    {
      write_log("DataPathMessage::type: msg has no route");
      return DP_NO_ROUTE;
    }
    if(ptxd->nh_invalid == 1)
    {
      write_log("DataPathMessage::type: msg has invalid next hop");
      return DP_NH_INVALID;
    }
    if(ptxd->non_ip == 1)
    {
      if(ptxd->ethertype == 0x0806)
      {
        write_log("DataPathMessage::type: msg is ARP");
        return DP_ARP_PACKET;
      }
      else
      {
        write_log("DataPathMessage::type: msg is general error packet");
        return DP_GENERAL_ERROR;
      }
    }
    write_log("DataPathMessage::type: msg is local delivery");
    return DP_LD_PACKET;
  }

  if(association == MSG_TO_MUX)
  {
    write_log("DataPathMessage::type: msg is to MUX");
    return DP_RECLASSIFY;
  }

  if(association == MSG_TO_QM)
  {
    write_log("DataPathMessage::type: msg is to QM");
    return DP_PASS_THROUGH;
  }

  if(association == MSG_TO_PLUGIN_0)
  {
    write_log("DataPathMessage::type: msg is to plugin 0");
    return DP_DEST_PLUGIN_0;
  }

  if(association == MSG_TO_PLUGIN_1)
  {
    write_log("DataPathMessage::type: msg is to plugin 1");
    return DP_DEST_PLUGIN_1;
  }

  if(association == MSG_TO_PLUGIN_2)
  {
    write_log("DataPathMessage::type: msg is to plugin 2");
    return DP_DEST_PLUGIN_2;
  }

  if(association == MSG_TO_PLUGIN_3)
  {
    write_log("DataPathMessage::type: msg is to plugin 3");
    return DP_DEST_PLUGIN_3;
  }

  if(association == MSG_TO_PLUGIN_4)
  {
    write_log("DataPathMessage::type: msg is to plugin 4");
    return DP_DEST_PLUGIN_4;
  }
  
  write_log("DataPathMessage::type: msg is to freelist manager");
  return DP_DROP;
}

unsigned int DataPathMessage::getnumwords() throw()
{
  if(association == MSG_FROM_PLC)
  {
    return PLC_TO_XSCALE_DATA_NUM_WORDS;
  }

  if(association == MSG_TO_MUX)
  {
    return XSCALE_TO_MUX_DATA_NUM_WORDS;
  }

  if(association == MSG_TO_QM)
  {
    return XSCALE_TO_QM_DATA_NUM_WORDS;
  }

  if(association == MSG_TO_PLUGIN_0 || association == MSG_TO_PLUGIN_1 || association == MSG_TO_PLUGIN_2 ||
     association == MSG_TO_PLUGIN_3 || association == MSG_TO_PLUGIN_4)
  {
    return XSCALE_TO_PLUGIN_DATA_NUM_WORDS;
  }

  return XSCALE_TO_FLM_DATA_NUM_WORDS;
}

unsigned int DataPathMessage::getmsgassociation() throw()
{
  return association;
}

unsigned int DataPathMessage::getword(unsigned int word) throw(datapathmessage_exception)
{
  if(association == MSG_FROM_PLC)
  {
    if(word >= PLC_TO_XSCALE_DATA_NUM_WORDS)
    {
      throw datapathmessage_exception("invalid word");
    }
  }
  else if(association == MSG_TO_MUX)
  {
    if(word >= XSCALE_TO_MUX_DATA_NUM_WORDS)
    {
      throw datapathmessage_exception("invalid word");
    }
  }
  else if(association == MSG_TO_QM)
  {
    if(word >= XSCALE_TO_QM_DATA_NUM_WORDS)
    {
      throw datapathmessage_exception("invalid word");
    }
  }
  else if(association == MSG_TO_PLUGIN_0 || association == MSG_TO_PLUGIN_1 || association == MSG_TO_PLUGIN_2 ||
          association == MSG_TO_PLUGIN_3 || association == MSG_TO_PLUGIN_4)
  {
    if(word >= XSCALE_TO_PLUGIN_DATA_NUM_WORDS)
    {
      throw datapathmessage_exception("invalid word");
    }
  }
  else // association == MSG_TO_FLM
  {
    if(word >= XSCALE_TO_FLM_DATA_NUM_WORDS)
    {
      throw datapathmessage_exception("invalid word");
    }
  }
  
  return msg[word];
}

unsigned int *DataPathMessage::getmsg() throw()
{
  return msg;
}
