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
#include <netinet/in.h>

#include "shared.h"

#include "host_configuration.h"
#include "host_globals.h"
#include "host_requests.h"

namespace host
{
  dispatcher* the_dispatcher;
  nccp_listener* rli_conn;
  
  configuration* conf;
};

using namespace host;

int main()
{
  log = new log_file("/tmp/host.log");
  the_dispatcher = dispatcher::get_dispatcher();
  rli_conn = NULL;
  conf = new configuration();

  try
  {
    std::string tmp_addr("127.0.0.1");
    rli_conn = new nccp_listener(tmp_addr, Default_ND_Port);
    //rli_conn = new nccp_listener("127.0.0.1", Default_ND_Port);
  }
  catch(std::exception& e)
  {
    write_log(e.what());
    exit(1);
  }

  register_req<configure_node_req>(NCCP_Operation_CfgNode);
	register_req<start_vm_req>(NCCP_Operation_startVM);

  register_req<add_route_req>(HOST_AddRoute);
  register_req<delete_route_req>(HOST_DeleteRoute);
	register_req<configure_vm_req>(HOST_ConfigureVM);

  rli_conn->receive_messages(false);

  pthread_exit(NULL);
}
