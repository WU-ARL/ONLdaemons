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

#include <boost/shared_ptr.hpp>

#include "shared.h"

#include "vmd_session.h"
#include "vmd_configuration.h"
#include "vmd_globals.h"
#include "vmd_requests.h"

namespace vmd
{
  dispatcher* the_dispatcher;
  nccp_listener* rli_conn;
  configuration* conf;
  session_manager* global_session;
  int listen_port;
};

using namespace vmd;

int main(int argc, char** argv)
{
  log = new log_file("/tmp/vmd.log");
  the_dispatcher = dispatcher::get_dispatcher();
  rli_conn = NULL;
  conf = new configuration();
  global_session = new session_manager();

  listen_port = Default_ND_Port;

  for (int i = 0; i < argc; ++i)
    {
      if (!strcmp(argv[i], "--onlport"))
	{
	  listen_port = atoi(argv[++i]);
	  break;
	}
    }

  try
  {
    std::string tmp_addr("127.0.0.1");
    rli_conn = new nccp_listener(tmp_addr, listen_port);//Default_ND_Port);
  }
  catch(std::exception& e)
  {
    write_log(e.what());
    exit(1);
  }

  register_req<start_experiment_req>(NCCP_Operation_StartExperiment);
  register_req<configure_node_req>(NCCP_Operation_CfgNode);
  register_req<end_configure_node_req>(NCCP_Operation_EndCfgNode);
  register_req<refresh_req>(NCCP_Operation_Refresh);

  rli_conn->receive_messages(false);
  pthread_exit(NULL);
}
