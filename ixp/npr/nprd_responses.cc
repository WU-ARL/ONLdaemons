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
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <memory.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>

#include "shared.h"

#include "nprd_types.h"
#include "nprd_configuration.h"
#include "nprd_monitor.h"
#include "nprd_pluginmessage.h"
#include "nprd_pluginconn.h"
#include "nprd_datapathmessage.h"
#include "nprd_datapathconn.h"
#include "nprd_plugincontrol.h"
#include "nprd_ipv4.h"
#include "nprd_arp.h"
#include "nprd_globals.h"
#include "nprd_requests.h"
#include "nprd_responses.h"

using namespace npr;

get_plugins_resp::get_plugins_resp(get_plugins_req* req, NCCP_StatusType stat): response(req)
{
  num_plugins = 0;
  status = stat;
}

get_plugins_resp::get_plugins_resp(get_plugins_req* req, std::vector<std::string>& label_list, std::vector<std::string>& path_list): response(req)
{
  labels = label_list;
  paths = path_list;
  num_plugins = paths.size();
  status = NCCP_Status_Fine;
}

get_plugins_resp::~get_plugins_resp()
{
  labels.clear();
  paths.clear();
}

void
get_plugins_resp::write()
{
  response::write();
  
  buf << status;
  buf << num_plugins;
  for(uint32_t i=0; i<num_plugins; ++i)
  {
    nccp_string label = labels[i].c_str();
    nccp_string path = paths[i].c_str();
    buf << label;
    buf << (uint32_t)0;
    buf << path;
  }
}
