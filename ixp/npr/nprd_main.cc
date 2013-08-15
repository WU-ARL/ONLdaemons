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
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <pthread.h>

#include "shared.h"

#include "nprd_types.h"
#include "nprd_configuration.h"
#include "nprd_monitor.h"
#include "nprd_datapathmessage.h"
#include "nprd_pluginmessage.h"
#include "nprd_datapathconn.h"
#include "nprd_pluginconn.h"
#include "nprd_plugincontrol.h"
#include "nprd_ipv4.h"
#include "nprd_arp.h"
#include "nprd_globals.h"
#include "nprd_requests.h"

extern "C"
{
  #include "api.h"
}

namespace npr
{
  dispatcher* the_dispatcher;
  // connections to world
  nccp_listener* rli_conn;      // RLI 
  DataPathConn* data_path_conn; // PLC, mux, QM
  PluginConn* plugin_conn;      // plugins (control/config messages)

  // major components
  ARP* arp;                      // arp processing
  IPv4* ipv4;                    // misc. ipv4 processing
  Configuration* configuration;  // manages all actual configuration of system
  Monitor* monitor;              // read stats
  PluginControl* plugin_control; // handle control messages to/from plugins

  bool rsys_api_start();
  void rsys_api_stop();
  int rsys_api_whichnpu();
  Configuration::rtm_mac_addrs *rsys_api_get_mac_addrs(int npu);

  bool init();
  void cleanup();

  void* handle_task(void *); // thread main

  enum conns { PLUGINCONN, DATAPATHCONN };

  bool rsys_api_start()
  {
    RppClientOpts rpp_options = {50, FALSE, FALSE, ""};
    char rpp_addr[16];
  
    memset(rpp_addr, 0, 16);
    strcpy(rpp_addr, "192.168.111.1");
    if(rpclOpen((UCHAR *)rpp_addr,&rpp_options) != RC_SUCCESS)
    {
      return false;
    }
    return true;
  }
  
  void rsys_api_stop()
  {
    /* can't call this because it raises SIGTERM and NEVER RETURNS @#$@@#!*/
    //rpclClose();
  }

  int rsys_api_whichnpu()
  {
    int status = system("/root/whichNpu");
    if(status == -1)
    {
      return -1;
    }
    if(WIFEXITED(status) == 0)
    {
      return -1;
    }
    return WEXITSTATUS(status);
  }

  Configuration::rtm_mac_addrs *rsys_api_get_mac_addrs(int npu)
  {
    Configuration::rtm_mac_addrs *macs = new Configuration::rtm_mac_addrs();

    for(unsigned int i = 0; i<=4; ++i)
    {
      rtm_mac_address_t rmat;
      rmat.portNumber = i+1;
      if(npu == 0) // NPUB 
      {
        rmat.portNumber += 5;
      }
      if(RtmGetMacAddress(&rmat) != RC_SUCCESS)
      {
        return NULL;
      }
      macs->port_hi16[i] = (rmat.macAddr[5] << 8) | (rmat.macAddr[4]);
      macs->port_low32[i] = (rmat.macAddr[3] << 24) | (rmat.macAddr[2] << 16) | (rmat.macAddr[1] << 8) | (rmat.macAddr[0]);
    }

    return macs;
  }

  bool init()
  {
    onld::log = new log_file("/log/nprd.log");
    the_dispatcher = dispatcher::get_dispatcher();
    rli_conn = NULL;

    write_log("init: Initializing");

    if(!rsys_api_start())
    {
      write_log("radisys api start failed");
      return false;
    }
    int npu = rsys_api_whichnpu();
    if(npu == -1)
    {
      write_log("radisys api whichnpu failed");
      return false;
    }
    Configuration::rtm_mac_addrs *macs = rsys_api_get_mac_addrs(npu);
    if(macs == NULL)
    {
      write_log("radisys api get mac addresses failed");
      return false;
    }
 
    configuration = new Configuration(npu, macs);
    delete macs;

    data_path_conn = new DataPathConn();
    plugin_conn = new PluginConn();
    std::string tmp_addr("127.0.0.1");
    rli_conn = new nccp_listener(tmp_addr, Default_ND_Port);
    //rli_conn = new nccp_listener("127.0.0.1", Default_ND_Port);
    plugin_control = new PluginControl();
    arp = new ARP();
    ipv4 = new IPv4();
    monitor = new Monitor();

    configuration->start_router(NPRD_DEFAULT_BASE_CODE);
    return true;
  }

  void cleanup()
  {
    write_log("cleanup: Cleaning up");
    
    if(rli_conn) { delete rli_conn; }

    if(monitor) { delete monitor; }
    if(ipv4) { delete ipv4; }
    if(arp) { delete arp; }
    if(plugin_control) { delete plugin_control; }
    if(plugin_conn) { delete plugin_conn; }
    if(data_path_conn) { delete data_path_conn; }

    if(configuration) { delete configuration; }
  
    rsys_api_stop();
  }
    
  void* handle_task(void *arg)
  {
    int *conn = (int *)arg;

    // in general, this function is responsible for deleting any messages read (instead of the task functions)
    if(*conn == PLUGINCONN)
    {
      PluginMessage *msg = NULL;
      unsigned int type = PMAXTYPE+1;

      write_log("handle_task: got a packet from the plugins");

      try
      {
        msg = (PluginMessage *)plugin_conn->readmsg();

        type = msg->type();
        write_log("handle_task: packet has type " + int2str(type));

        switch(type)
        {
          case PCONTROLMSGRSP: 
            plugin_control->pass_result_to_rli(msg); delete msg; break;
          case PDEBUGMSG: 
            plugin_control->plugin_debug_msg(msg); delete msg; break;
          default:
            write_log("handle_task: don't know what to do with packet of type " + int2str(type));
            delete msg;
            break;
        }
      }
      catch(std::exception& e)
      {
        write_log("handle_task: got exception: " + std::string(e.what()));
        if(msg != NULL) delete msg;
      }
    }
    else if(*conn == DATAPATHCONN)
    {
      DataPathMessage *msg;
      unsigned int type = DP_MAX_TYPE+1;

      write_log("handle_task: got a packet from the data path");

      try
      {
        msg = (DataPathMessage *)data_path_conn->readmsg();

        type = msg->type();
        write_log("handle_task: packet has type " + int2str(type));

        switch(type)
        {
          case DP_TTL_EXPIRED: 
            ipv4->send_ttl_expired(msg); delete msg; break;
          case DP_NO_ROUTE: 
            ipv4->send_no_route(msg); delete msg; break;
          case DP_LD_PACKET: 
            ipv4->handle_local_delivery(msg); delete msg; break;
          case DP_IP_OPTIONS:
            ipv4->handle_ip_options(msg); delete msg; break;
          case DP_NH_INVALID:
            ipv4->handle_next_hop_invalid(msg); delete msg; break;
          case DP_GENERAL_ERROR:
            ipv4->handle_general_error(msg); delete msg; break;
          case DP_ARP_NEEDED_DIP:
          case DP_ARP_NEEDED_NHR:
            arp->send_arp_request(msg); break; // this function is special in that it takes care of deleting msgs
          case DP_ARP_PACKET:
            arp->process_arp_packet(msg); delete msg; break;
          default:
            write_log("handle_task: don't know what to do with packet of type " + int2str(type));
            delete msg;
            break;
        }
      }
      catch(std::exception& e)
      {
        write_log("handle_task: got exception: " + std::string(e.what()));
        if(msg != NULL && type != DP_ARP_NEEDED_DIP && type != DP_ARP_NEEDED_NHR) delete msg;
      }
    }

    delete conn;
    return NULL;
  }

}; // namespace npr

using namespace npr;

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
  register_req<add_route_req>(NPR_AddRoute);
  register_req<delete_route_req>(NPR_DeleteRoute);

  // manage filters
  register_req<add_filter_req>(NPR_AddFilter);
  register_req<delete_filter_req>(NPR_DeleteFilter);
  register_req<set_sample_rates_req>(NPR_SetSampleRates);
  
  // configure queues and ports
  register_req<set_queue_params_req>(NPR_SetQueueParams);
  register_req<set_port_rate_req>(NPR_SetPortRate);

  // manage plugins
  register_req<get_plugins_req>(NPR_GetPlugins);
  register_req<add_plugin_req>(NPR_AddPlugin);
  register_req<delete_plugin_req>(NPR_DeletePlugin);
  register_req<plugin_debug_req>(NPR_PluginDebug);
  register_req<plugin_control_msg_req>(NPR_PluginControlMsg);

  // read counters and such
  register_req<get_rx_pkt_req>(NPR_GetRXPkt);
  register_req<get_rx_byte_req>(NPR_GetRXByte);
  register_req<get_tx_pkt_req>(NPR_GetTXPkt);
  register_req<get_tx_byte_req>(NPR_GetTXByte);
  register_req<get_reg_pkt_req>(NPR_GetRegPkt);
  register_req<get_reg_byte_req>(NPR_GetRegByte);
  register_req<get_stats_preq_pkt_req>(NPR_GetStatsPreQPkt);
  register_req<get_stats_postq_pkt_req>(NPR_GetStatsPostQPkt);
  register_req<get_stats_preq_byte_req>(NPR_GetStatsPreQByte);
  register_req<get_stats_postq_byte_req>(NPR_GetStatsPostQByte);
  register_req<get_queue_len_req>(NPR_GetQueueLength);
  register_req<get_plugin_counter_req>(NPR_GetPluginCounter);

  rli_conn->receive_messages(true);

  // now loop forever checking for messages from data path and plugins
  bool started = false;
  while(true)
  {
    if(!started) { started = configuration->router_started(); }
    if(started && plugin_conn->check())
    {
      pthread_t tid;
      pthread_attr_t tattr;
      int *arg = new int();
      *arg = PLUGINCONN;

      if(pthread_attr_init(&tattr) != 0)
      {
        write_log("pthread_attr_init failed"); break;
      }
      if(pthread_attr_setdetachstate(&tattr,PTHREAD_CREATE_DETACHED) != 0)
      {
        write_log("pthread_attr_setdetachstate failed"); break;
      }
      if(pthread_create(&tid, &tattr, npr::handle_task, (void *)arg) != 0)
      {
        write_log("pthread_create failed"); break;
      }
    }
    if(started && data_path_conn->check())
    {
      pthread_t tid;
      pthread_attr_t tattr;
      int *arg = new int();
      *arg = DATAPATHCONN;

      if(pthread_attr_init(&tattr) != 0)
      {
        write_log("pthread_attr_init failed"); break;
      }
      if(pthread_attr_setdetachstate(&tattr,PTHREAD_CREATE_DETACHED) != 0)
      {
        write_log("pthread_attr_setdetachstate failed"); break;
      }
      if(pthread_create(&tid, &tattr, npr::handle_task, (void *)arg) != 0)
      {
        write_log("pthread_create failed"); break;
      }
    }
  }

  pthread_exit(NULL);
}
