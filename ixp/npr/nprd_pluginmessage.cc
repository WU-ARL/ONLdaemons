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
#include "nprd_pluginmessage.h"

using namespace npr;

PluginMessage::PluginMessage(unsigned int *msgwords, unsigned int p) throw(pluginmessage_exception)
{
  if(p > 4)
  {
    throw pluginmessage_exception("plugin is invalid!");
  }
  
  plugin = p;
  hdr = (plugin_control_header *)&msgwords[0];
  
  if(hdr->type > PMAXTYPE)
  {
    throw pluginmessage_exception("type is invalid!");
  }

  msg = new unsigned int[hdr->num_words + 1];
  for(unsigned int i=0; i<=hdr->num_words; ++i)
  {
    msg[i] = msgwords[i];
  }
  hdr = (plugin_control_header *)&msg[0];

  write_log("PluginMessage::PluginMessage: new message with type=" + int2str(hdr->type));
}

PluginMessage::~PluginMessage() throw()
{
  write_log("PluginMessage::PluginMessage: deleting message with type=" + int2str(hdr->type));

  delete[] msg;
}

unsigned int PluginMessage::type() throw()
{
  return hdr->type;
}

unsigned int PluginMessage::getnumwords() throw()
{
  return hdr->num_words;
}

unsigned int PluginMessage::getplugin() throw()
{
  return plugin;
}

unsigned int PluginMessage::getword(unsigned int word) throw(pluginmessage_exception)
{
  if(word > hdr->num_words)
  { 
    throw pluginmessage_exception("invalid word!");
  }
  return msg[word];
}

plugin_control_header *PluginMessage::gethdr() throw()
{
  return hdr;
}

unsigned int *PluginMessage::getmsg() throw()
{
  return msg;
}
