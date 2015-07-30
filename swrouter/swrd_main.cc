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
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
//#include <net/if.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "shared.h"

#include <boost/shared_ptr.hpp>
#include <netlink/route/tc.h>
#include <netlink/route/link.h>
#include "swrd_types.h"
#include "swrd_router.h"
//#include "swrd_monitor.h"
#include "swrd_globals.h"
#include "swrd_requests.h"

extern "C"
{
//  #include "api.h"
}

namespace swr
{
  dispatcher* the_dispatcher;
  // connections to world
  nccp_listener* rli_conn;      // RLI 

  // major components
  Router* router;  // manages all actual configuration of system
  //Monitor* monitor;              // read stats


  bool init();
  void cleanup();

  void* handle_task(void *); // thread main


  bool init(int rtr_type)
  {
    onld::log = new log_file("/tmp/swrd.log");
    write_log("init: Initializing");
    /*
    char shcmd[256];
    int num_ports = rtr_type;
    //run tuning script
    for (int i = 0; i < num_ports; ++i)
      {
	sprintf(shcmd, "/usr/local/bin/ixgb_perf.sh data%d /usr/local/bin", i);
	write_log("init tuning-system(" + std::string(shcmd) + ")");
	if (system(shcmd) < 0)
	  {
	    write_log("tuning script for data" + int2str(i) + " failed");
	    return false;
	  }
      }*/
    the_dispatcher = dispatcher::get_dispatcher();
    rli_conn = NULL;


    router = new Router(rtr_type);

    std::string tmp_addr("127.0.0.1");
    rli_conn = new nccp_listener(tmp_addr, Default_ND_Port);
    //monitor = new Monitor();

    try
    {
      router->start_router();
    }
    catch(std::exception& e)
    {
      return false;
    }
    return true;
  }

  void cleanup()
  {
    write_log("cleanup: Cleaning up");
    
    if(rli_conn) { delete rli_conn; }

    //if(monitor) { delete monitor; }

    if(router) { delete router; }
  
  }
    
  void* handle_task(void *arg)
  {
    return NULL;
  }

}; // namespace swr

using namespace swr;

int main(int argc, char** argv)
{

  int rtr_type = SWR_2P_10G;
  for (int i = 0; i < argc; ++i)
    {
      if (strcmp(argv[i], "-swr5") == 0) 
	{
	  rtr_type = SWR_5P_1G;
	  break;
	} 
    }
  try
  {  
    if(!init(rtr_type))
    {
      exit(1);
    }
  }
  catch(std::exception& e)
  {
    write_log(e.what());
    exit(1);
  }
  catch(...)
  {
    write_log("got unknown exception");
    exit(1);
  }

  //register rli messages
 
  // initial configuration
  register_req<configure_node_req>(NCCP_Operation_CfgNode);

  // manage routes
  // main router route table
  register_req<add_route_main_req>(SWR_AddRouteMain);
  register_req<del_route_main_req>(SWR_DeleteRouteMain);

  // per port route tables
  register_req<add_route_port_req>(SWR_AddRoutePort);
  register_req<del_route_port_req>(SWR_DeleteRoutePort);

  // per port add delay fot outgoing traffic
  register_req<add_delay_req>(SWR_AddDelay);
  register_req<delete_delay_req>(SWR_DeleteDelay);

  // manage filters
  register_req<filter_req>(SWR_AddFilter);
  register_req<filter_req>(SWR_DeleteFilter);
  register_req<filter_req>(SWR_GetFilterBytes);
  register_req<filter_req>(SWR_GetFilterPkts);

  
  // configure queues 
  register_req<set_queue_params_req>(SWR_AddQueue);
  register_req<set_queue_params_req>(SWR_ChangeQueue);
  register_req<set_queue_params_req>(SWR_DeleteQueue);
  register_req<set_queue_params_req>(SWR_AddNetemParams);
  register_req<set_queue_params_req>(SWR_DeleteNetemParams);

  // configure ports
  register_req<set_port_rate_req>(SWR_SetPortRate);

  // Monitoring
  // Per port rx and tx byte and pkt counts
  register_req<get_tx_pkt_req>(SWR_GetTXPkt);
  register_req<get_tx_kbits_req>(SWR_GetTXKBits);

  //link stats
  register_req<get_link_tx_pkt_req>(SWR_GetLinkTXPkt);
  register_req<get_link_rx_pkt_req>(SWR_GetLinkRXPkt);
  register_req<get_link_tx_kbits_req>(SWR_GetLinkTXKBits);
  register_req<get_link_rx_kbits_req>(SWR_GetLinkRXKBits);
  register_req<get_link_tx_drops_req>(SWR_GetLinkTXDrops);
  register_req<get_link_rx_drops_req>(SWR_GetLinkRXDrops);
  register_req<get_link_tx_errors_req>(SWR_GetLinkTXErrors);
  register_req<get_link_rx_errors_req>(SWR_GetLinkRXErrors);

  // queue lengths
  register_req<get_queue_len_req>(SWR_GetDefQueueLength);
  register_req<get_queue_len_req>(SWR_GetQueueLength);
  register_req<get_queue_len_req>(SWR_GetClassLength);
  register_req<get_drops_req>(SWR_GetDrops);
  register_req<get_drops_req>(SWR_GetQueueDrops);
  register_req<get_drops_req>(SWR_GetClassDrops);
  register_req<get_backlog_req>(SWR_GetBacklog);
  register_req<get_backlog_req>(SWR_GetQueueBacklog);
  register_req<get_backlog_req>(SWR_GetClassBacklog);
  register_req<get_tx_pkt_req>(SWR_GetQueueTXPkt);
  register_req<get_tx_pkt_req>(SWR_GetClassTXPkt);
  register_req<get_tx_kbits_req>(SWR_GetQueueTXKBits);
  register_req<get_tx_kbits_req>(SWR_GetClassTXKBits);


  rli_conn->receive_messages(true);

/*
  // now loop forever checking for messages 
  bool started = false;
  while(true)
  {
    if(!started) { started = router->router_started(); }
    //this sets up a thread that processes any messages coming from the router process
    //for the npr this would have checked messages coming from the ixp
    // For the software router we have nothing for this while(true) loop.
    // Nothing comes to the controller except from the RLI.
    // so we can just exit this thread. The RLI thread will continue.
  }
*/

  while(true){ sleep(1);}

  pthread_exit(NULL);
}
