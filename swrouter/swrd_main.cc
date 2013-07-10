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
#include "swrd_monitor.h"
#include "swrd_globals.h"
#include "swrd_requests.h"

extern "C"
{
  #include "api.h"
}

namespace swr
{
  dispatcher* the_dispatcher;
  // connections to world
  nccp_listener* rli_conn;      // RLI 

  // major components
  Configuration* configuration;  // manages all actual configuration of system
  Monitor* monitor;              // read stats


  bool init();
  void cleanup();

  void* handle_task(void *); // thread main

  //enum conns { PLUGINCONN, DATAPATHCONN };



  bool init()
  {
    onld::log = new log_file("/log/swrd.log");
    the_dispatcher = dispatcher::get_dispatcher();
    rli_conn = NULL;

    write_log("init: Initializing");

    Configuration::rtm_mac_addrs *macs = NULL;
    if(macs == NULL)
    {
      write_log("radisys api get mac addresses failed");
      return false;
    }
 
    configuration = new Configuration(npu, macs);
    delete macs;

    rli_conn = new nccp_listener("127.0.0.1", Default_ND_Port);
    monitor = new Monitor();

    configuration->start_router();
    return true;
  }

  void cleanup()
  {
    write_log("cleanup: Cleaning up");
    
    if(rli_conn) { delete rli_conn; }

    if(monitor) { delete monitor; }

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
  register_req<add_route_req>(SWR_AddRoute);
  register_req<delete_route_req>(SWR_DeleteRoute);

  // manage filters
  register_req<add_filter_req>(SWR_AddFilter);
  register_req<delete_filter_req>(SWR_DeleteFilter);
  
  // configure queues and ports
  //register_req<set_queue_params_req>(SWR_SetQueueParams);
  register_req<set_port_rate_req>(SWR_SetPortRate);

  /*
  // read counters and such
  register_req<get_rx_pkt_req>(SWR_GetRXPkt);
  register_req<get_rx_byte_req>(SWR_GetRXByte);
  register_req<get_tx_pkt_req>(SWR_GetTXPkt);
  register_req<get_tx_byte_req>(SWR_GetTXByte);
  register_req<get_reg_pkt_req>(SWR_GetRegPkt);
  register_req<get_reg_byte_req>(SWR_GetRegByte);
  register_req<get_stats_preq_pkt_req>(SWR_GetStatsPreQPkt);
  register_req<get_stats_postq_pkt_req>(SWR_GetStatsPostQPkt);
  register_req<get_stats_preq_byte_req>(SWR_GetStatsPreQByte);
  register_req<get_stats_postq_byte_req>(SWR_GetStatsPostQByte);
  register_req<get_queue_len_req>(SWR_GetQueueLength);
  */

  rli_conn->receive_messages(true);

  // now loop forever checking for messages from data path and plugins
  bool started = false;
  while(true)
  {
    if(!started) { started = configuration->router_started(); }
    //this sets up a thread that processes any messages coming from the router process
    //for the npr this would have checked messages coming from the ixp
      /*
	if(started)
	{
	pthread_t tid;
      pthread_attr_t tattr;
      int *arg = new int();
      if(pthread_attr_init(&tattr) != 0)
      {
      write_log("pthread_attr_init failed"); break;
      }
      if(pthread_attr_setdetachstate(&tattr,PTHREAD_CREATE_DETACHED) != 0)
      {
      write_log("pthread_attr_setdetachstate failed"); break;
      }
      if(pthread_create(&tid, &tattr, swr::handle_task, (void *)arg) != 0)
      {
      write_log("pthread_create failed"); break;
      }
      
      }
      */
  }

  pthread_exit(NULL);
}
