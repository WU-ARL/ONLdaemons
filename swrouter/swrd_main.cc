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
#include <net/if.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "shared.h"

#include "swrd_types.h"
#include "swrd_configuration.h"
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
  Configuration* configuration;  // manages all actual configuration of system
  //Monitor* monitor;              // read stats


  bool init();
  void cleanup();

  void* handle_task(void *); // thread main


  bool init()
  {
    onld::log = new log_file("/tmp/swrd.log");
    the_dispatcher = dispatcher::get_dispatcher();
    rli_conn = NULL;

    write_log("init: Initializing");

    configuration = new Configuration();

    std::string tmp_addr("127.0.0.1");
    rli_conn = new nccp_listener(tmp_addr, Default_ND_Port);
    //monitor = new Monitor();

    configuration->start_router();
    return true;
  }

  void cleanup()
  {
    write_log("cleanup: Cleaning up");
    
    if(rli_conn) { delete rli_conn; }

    //if(monitor) { delete monitor; }

    if(configuration) { delete configuration; }
  
  }
    
  void* handle_task(void *arg)
  {
    return NULL;
  }

}; // namespace swr

using namespace swr;

int main()
{
  try
  {  
    if(!init())
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

/*
  // manage filters
  register_req<add_filter_req>(SWR_AddFilter);
  register_req<del_filter_req>(SWR_DeleteFilter);
  
  // configure queues 
  register_req<add_queue_req>(SWR_AddQueue);
  register_req<del_queue_req>(SWR_DeleteQueue);
  register_req<set_queue_params_req>(SWR_SetQueueParams);
*/

  // configure ports
  register_req<configure_node_req>(SWR_SetPortRate);
  register_req<set_port_rate_req>(SWR_SetPortRate);

/*
  // Monitoring
  // Per port rx and tx byte and pkt counts
  register_req<get_rx_pkt_req>(SWR_GetRXPkt);
  register_req<get_rx_byte_req>(SWR_GetRXByte);
  register_req<get_tx_pkt_req>(SWR_GetTXPkt);
  register_req<get_tx_byte_req>(SWR_GetTXByte);
*/

/*
  // queue lengths
  register_req<get_queue_len_req>(SWR_GetQueueLength);
*/

  rli_conn->receive_messages(true);

/*
  // now loop forever checking for messages 
  bool started = false;
  while(true)
  {
    if(!started) { started = configuration->router_started(); }
    //this sets up a thread that processes any messages coming from the router process
    //for the npr this would have checked messages coming from the ixp
    // For the software router we have nothing for this while(true) loop.
    // Nothing comes to the controller except from the RLI.
    // so we can just exit this thread. The RLI thread will continue.
  }
*/

  while(true);

  pthread_exit(NULL);
}
