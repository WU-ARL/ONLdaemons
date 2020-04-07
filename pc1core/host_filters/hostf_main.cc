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

#include "hostf_configuration.h"
#include "hostf_globals.h"
#include "hostf_requests.h"

namespace hostf
{
  dispatcher* the_dispatcher;
  nccp_listener* rli_conn;
  
  HostConfiguration* conf;
};

using namespace hostf;

int main()
{
  log = new log_file("/tmp/host.log");
  the_dispatcher = dispatcher::get_dispatcher();
  rli_conn = NULL;
  conf = new HostConfiguration();

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

  register_req<add_route_req>(HOST_AddRoute);
  register_req<delete_route_req>(HOST_DeleteRoute);

  // per port add delay fot outgoing traffic
  register_req<add_delay_req>(HOST_AddDelay);
  register_req<delete_delay_req>(HOST_DeleteDelay);

  // manage filters
  register_req<filter_req>(HOST_AddFilter);
  register_req<filter_req>(HOST_DeleteFilter);
  register_req<filter_req>(HOST_GetFilterBytes);
  register_req<filter_req>(HOST_GetFilterPkts);

  
  // configure queues 
  register_req<set_queue_params_req>(HOST_AddQueue);
  register_req<set_queue_params_req>(HOST_ChangeQueue);
  register_req<set_queue_params_req>(HOST_DeleteQueue);
  register_req<set_queue_params_req>(HOST_AddNetemParams);
  register_req<set_queue_params_req>(HOST_DeleteNetemParams);

  // configure ports
  register_req<set_port_rate_req>(HOST_SetPortRate);

  // Monitoring
  // Per port rx and tx byte and pkt counts
  register_req<get_tx_pkt_req>(HOST_GetTXPkt);
  register_req<get_tx_kbits_req>(HOST_GetTXKBits);

  //link stats
  register_req<get_link_tx_pkt_req>(HOST_GetLinkTXPkt);
  register_req<get_link_rx_pkt_req>(HOST_GetLinkRXPkt);
  register_req<get_link_tx_kbits_req>(HOST_GetLinkTXKBits);
  register_req<get_link_rx_kbits_req>(HOST_GetLinkRXKBits);
  register_req<get_link_tx_drops_req>(HOST_GetLinkTXDrops);
  register_req<get_link_rx_drops_req>(HOST_GetLinkRXDrops);
  register_req<get_link_tx_errors_req>(HOST_GetLinkTXErrors);
  register_req<get_link_rx_errors_req>(HOST_GetLinkRXErrors);

  // queue lengths
  register_req<get_queue_len_req>(HOST_GetDefQueueLength);
  register_req<get_queue_len_req>(HOST_GetQueueLength);
  register_req<get_queue_len_req>(HOST_GetClassLength);
  register_req<get_drops_req>(HOST_GetDrops);
  register_req<get_drops_req>(HOST_GetQueueDrops);
  register_req<get_drops_req>(HOST_GetClassDrops);
  register_req<get_backlog_req>(HOST_GetBacklog);
  register_req<get_backlog_req>(HOST_GetQueueBacklog);
  register_req<get_backlog_req>(HOST_GetClassBacklog);
  register_req<get_tx_pkt_req>(HOST_GetQueueTXPkt);
  register_req<get_tx_pkt_req>(HOST_GetClassTXPkt);
  register_req<get_tx_kbits_req>(HOST_GetQueueTXKBits);
  register_req<get_tx_kbits_req>(HOST_GetClassTXKBits);
  rli_conn->receive_messages(false);

  pthread_exit(NULL);
}
