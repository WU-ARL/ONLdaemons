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
#include <set>
#include <exception>
#include <stdexcept>

#include <boost/shared_ptr.hpp>

#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <getopt.h>
#include <memory.h>
#include <pthread.h>
#include <sys/types.h>
#include <netinet/in.h>

#include "shared.h"
#include "onl_database.h"

#include "onlcrd_reservation.h"
#include "onlcrd_session.h"
#include "onlcrd_component.h"
#include "onlcrd_session_manager.h"
#include "onlcrd_globals.h"
#include "onlcrd_requests.h"
#include "onlcrd_responses.h"
#include "onlcrd_session_sharing.h"

namespace onlcrd
{
  dispatcher* the_dispatcher;
  session_manager* the_session_manager;
  nccp_connection* the_gige_conn;
  nccp_listener* listener;

  bool testing;
};

using namespace onlcrd;

int main(int argc, char** argv)
{
  uint16_t listen_port = Default_CRD_Port;
  testing = false;

  static struct option longopts[] =
  {{"help",       0, NULL, 'h'},
   {"port",       1, NULL, 'p'},
   {"testing",    0, NULL, 't'},
   {NULL,         0, NULL,  0}
  };

  int c;
  while ((c = getopt_long(argc, argv, "hp:t", longopts, NULL)) != -1) {
    switch (c) {
      case 'p':
        listen_port = atoi(optarg);
        break;
      case 't':
        testing = true;
        break;
      case 'h':
      default:
        std::cout << "usage: onlcrd [-h] [-p port] [-t]" << std::endl;
        std::cout << "       -h:       print this message" << std::endl;
        std::cout << "       -p port:  change listening port. defaults to " << int2str(Default_CRD_Port) << std::endl;
        std::cout << "       -t:       run in testing mode, i.e., do not actually connect to other daemons" << std::endl;
        exit(0);
    }
  }

  usage_log = new log_file("/etc/ONL/crd_usage.log");
  write_usage_log("ONL CRD Usage Log");
  log = new log_file("/etc/ONL/crd.log");
  the_dispatcher = dispatcher::get_dispatcher();
  the_gige_conn = NULL;
  listener = NULL;

  try
  {
    the_session_manager = new session_manager();
    if (!testing)
      {
         the_gige_conn = new nccp_connection("127.0.0.1", Default_NMD_Port);
         std::string tmp_addr("10.0.1.2");//onlsrv
         listener = new nccp_listener(tmp_addr, listen_port);
      }
    else
      {
         std::string tmp_addr("10.0.1.7");//onlsrv2
         listener = new nccp_listener(tmp_addr, listen_port);
      }
  }
  catch(std::exception& e)
  {
    if(the_session_manager) delete the_session_manager;
    if(listener) delete listener;
    if(the_gige_conn) delete the_gige_conn;
    write_log(e.what());
    exit(1);
  }

  // periodic pings from RLI
  register_req<ping_req>(NCCP_Operation_NOP);

  // reservation commands from RLI
  register_req<begin_reservation_req>(NCCP_Operation_BeginReservation);
  register_req<reservation_add_component_req>(NCCP_Operation_ReservationAddComponent);
  register_req<reservation_add_cluster_req>(NCCP_Operation_ReservationAddCluster);
  register_req<reservation_add_link_req>(NCCP_Operation_ReservationAddLink);
  register_req<commit_reservation_req>(NCCP_Operation_CommitReservation);
  register_req<extend_reservation_req>(NCCP_Operation_ExtendReservation);
  register_req<cancel_reservation_req>(NCCP_Operation_CancelReservation);

  // session commands from RLI
  register_req<begin_session_req>(NCCP_Operation_BeginSession);
  register_req<session_add_component_req>(NCCP_Operation_SessionAddComponent);
  register_req<session_add_cluster_req>(NCCP_Operation_SessionAddCluster);
  register_req<session_add_link_req>(NCCP_Operation_SessionAddLink);
  register_req<commit_session_req>(NCCP_Operation_CommitSession);
  register_req<clear_session_req>(NCCP_Operation_EndSession);

  // session responses from nodes
  register_resp<crd_response>(NCCP_Operation_StartExperiment);
  register_resp<crd_response>(NCCP_Operation_EndExperiment);
  register_resp<crd_response>(NCCP_Operation_Refresh);
  register_resp<crd_response>(NCCP_Operation_CfgNode);

  // done refreshing command from nodes
  register_req<i_am_up_req>(NCCP_Operation_IAmUp);

  // configuration responses from switches
  register_resp<switch_response>(NCCP_Operation_Initialize);
  register_resp<add_vlan_response>(NCCP_Operation_AddVlan);
  register_resp<switch_response>(NCCP_Operation_DeleteVlan);
  register_resp<switch_response>(NCCP_Operation_AddToVlan);
  register_resp<switch_response>(NCCP_Operation_DeleteFromVlan);
 
  // session sharing commands from RLI
  register_req<add_observer_req>(NCCP_Operation_SessionAddObserver);
  register_req<get_observable_req>(NCCP_Operation_GetObservableSessions);
  register_req<observe_session_req>(NCCP_Operation_ObserveSession);
  register_req<grant_observe_req>(NCCP_Operation_GrantObserveSession);

/* these operations are no longer supported
  register_req<>(NCCP_Operation_RemapComponent);
  register_req<>(NCCP_Operation_DeleteComponent);
  register_req<>(NCCP_Operation_DeleteLink:);
*/

  if (!testing)
    the_gige_conn->receive_messages(true);
  the_session_manager->initialize();
  listener->receive_messages(false);

  pthread_exit(NULL);
}
