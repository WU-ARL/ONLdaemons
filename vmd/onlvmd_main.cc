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
#include <getopt.h>
#include <stdlib.h>
#include <stdint.h>
#include <memory.h>
#include <pthread.h>
#include <sys/types.h>
#include <netinet/in.h>


#include <boost/shared_ptr.hpp>

#include "shared.h"

#include "onlvmd_session.h"
#include "onlvmd_session_manager.h"
#include "onlvmd_globals.h"
#include "onlvmd_requests.h"

namespace onlvmd
{
  dispatcher *the_dispatcher;
  nccp_listener *rli_conn;
  nccp_connection *spec_conn;
  session_manager *the_session_manager;

  bool using_spec_daemon;
  std::string user;
  bool testing;
  bool root_only;//set to true if specialty daemons are only run as root
};

using namespace onlvmd;

int main(int argc, char** argv)
{
  the_dispatcher = dispatcher::get_dispatcher();
  the_session_manager = new session_manager();
  rli_conn = NULL;
  spec_conn = NULL;
  using_spec_daemon = false;
  user = "nobody";
  testing = false;

  static struct option longopts[] =
  {{"help",       0, NULL, 'h'},
   {"testing",    0, NULL, 't'},
   {"root_only",    0, NULL, 'r'},
   {NULL,         0, NULL,  0}
  };

  int c;
  while ((c = getopt_long(argc, argv, "htr", longopts, NULL)) != -1) {
    switch (c) {
      case 't':
        testing = true;
        break;
      case 'r':
	root_only = true;
	break;
      case 'h':
        std::cout << "usage: onlvmd [-h] [-t] [-r]" << std::endl;
        std::cout << "       -h:       print this message" << std::endl;
        std::cout << "       -t:       run in testing mode, i.e., don't reboot on refresh" << std::endl;
        std::cout << "       -r:       only launch subtype daemons as root" << std::endl;
        exit(0);
    }
  }
  try
  {
    std::string tmp_addr("127.0.0.1");
    rli_conn = new nccp_listener(tmp_addr, Default_ND_Port);
  }
  catch(std::exception& e)
  {
    write_log(e.what());
    exit(1);
  }

  register_req<start_experiment_req>(NCCP_Operation_StartExperiment);
  register_req<refresh_req>(NCCP_Operation_Refresh);

  //register_req<user_data_req>(NCCP_Operation_UserData);
  //register_req<user_data_ts_req>(NCCP_Operation_UserDataTS);

  register_req<configure_node_req>(NCCP_Operation_CfgNode);
  register_req<end_configure_node_req>(NCCP_Operation_EndCfgNode);
  //register_resp<crd_response>(NCCP_Operation_CfgNode);


  rli_conn->receive_messages(true);

  while (!the_session_manager->isDone()) { sleep(2); }
  exit(0);
}
