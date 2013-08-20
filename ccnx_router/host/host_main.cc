/*
 * ONL Notice
 *
 * Copyright (c) 2009-2013 Pierluigi Rolando, Charlie Wiseman, Jyoti Parwatikar, John DeHart
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
 *   limitations under the License.
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

  std::string username;

  std::vector<std::string> paths;
  std::vector<std::string> nexthops;
  pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
};

using namespace host;

int main(int argc, char *argv[])
{
	log = new log_file("/tmp/ccnx_router.log");
	//std::system("chmod a+rw /tmp/ccnx_router.log");

	// Initialize the username from command line parameters
	host::username = "onl";	// set a default
	++argv; 				// skip program name
	while(argc > 1 && argv != 0) {
		if((*argv)[0] != '\0' && (*argv)[0] != '-') {
			host::username = *argv;
			break;
		}
		++argv;
	}

	write_log("username is " + host::username);

	// Set up monitoring by adding the appropriate iptables rules
	/*
	write_log("iptables create chains");
	std::system("/sbin/iptables -N MONITOR_PKT_IN");
	std::system("/sbin/iptables -N MONITOR_BYTE_IN");
	std::system("/sbin/iptables -N MONITOR_PKT_OUT");
	std::system("/sbin/iptables -N MONITOR_BYTE_OUT");

	write_log("iptables appena chains");
	std::system("/sbin/iptables -I INPUT -j MONITOR_PKT_IN");
	std::system("/sbin/iptables -I INPUT -j MONITOR_BYTE_IN");
	std::system("/sbin/iptables -I OUTPUT -j MONITOR_PKT_OUT");
	std::system("/sbin/iptables -I OUTPUT -j MONITOR_BYTE_OUT");

	// Make chains capture all traffic
	write_log("iptables capture traffic");
	std::system("/sbin/iptables -A MONITOR_PKT_IN -i data0");
	std::system("/sbin/iptables -A MONITOR_BYTE_IN -i data0");
	std::system("/sbin/iptables -A MONITOR_PKT_OUT -o data0");
	std::system("/sbin/iptables -A MONITOR_BYTE_OUT -o data0");
        */

	write_log("normal setup");
	the_dispatcher = dispatcher::get_dispatcher();
	rli_conn = NULL;
	conf = new configuration();

	try {
                std::string tmp_addr("127.0.0.1");
		//rli_conn = new nccp_listener("127.0.0.1", Default_ND_Port);
		rli_conn = new nccp_listener(tmp_addr, Default_ND_Port);
	} catch(std::exception& e) {
		write_log(e.what());
		exit(1);
	}
    	try {
                std::string t;
                //std::string prologue = "sudo -i -u " + host::username + " /users/onl/ccnx/scripts/";
                std::string prologue = "/users/onl/ccnx/scripts/";
                                                                                                                                           t = prologue + "launch_daemon.sh 2>&1 > /tmp/ccnd.log &";
                                                                                                                                           std::system(t.c_str());
                std::system("chmod a+rw /tmp/ccnd.log");
                write_log("launch ccnd: executed: " + t);

                struct timespec interval;
                interval.tv_sec = 1;
                interval.tv_nsec = 0;

                nanosleep(&interval, 0);

        } catch(std::exception& e) {
                std::string msg = e.what();
                write_log("launch ccnd: got exception: " + msg);
        }

	for(unsigned i = 0; i < 256; ++i) {
		switch(i) {
			case CCNX_GetRXPkt: case CCNX_GetRXByte:
			case CCNX_GetTXPkt:	case CCNX_GetTXByte:

			case CCNX_GetMemUsage: case CCNX_GetCPUUsage:

			case CCNX_DoRole: case CCNX_DoDaemon:

			case CCNX_AddRoute: case CCNX_DelRoute: case CCNX_UpdateRoute:

			case CCNX_SubtypeInit:
			case CCNX_SetLogLevel:

			case NCCP_Operation_CfgNode:
			case HOST_AddRoute:
			case HOST_DeleteRoute:
				break;
			default:
				register_req<req_logger>(i);
		}
	}

	register_req<configure_node_req>(NCCP_Operation_CfgNode);

	register_req<add_route_req>(HOST_AddRoute);
	register_req<delete_route_req>(HOST_DeleteRoute);

	register_req<get_rx_pkt_req>(CCNX_GetRXPkt);
	register_req<get_rx_byte_req>(CCNX_GetRXByte);
	register_req<get_tx_pkt_req>(CCNX_GetTXPkt);
	register_req<get_tx_byte_req>(CCNX_GetTXByte);

	register_req<get_mem_usage_req>(CCNX_GetMemUsage);
	register_req<get_cpu_usage_req>(CCNX_GetCPUUsage);

	register_req<get_do_role_req>(CCNX_DoRole);
	register_req<get_do_daemon_req>(CCNX_DoDaemon);

	register_req<get_add_route_req>(CCNX_AddRoute);
	register_req<get_del_route_req>(CCNX_DelRoute);
	register_req<get_update_route_req>(CCNX_UpdateRoute);

	register_req<get_subtype_init_req>(CCNX_SubtypeInit);
	register_req<set_log_level_req>(CCNX_SetLogLevel);

	rli_conn->receive_messages(false);
        std::system("/users/onl/ccnx/scripts/kill_daemon.sh");
        write_log("ccnx_router exiting. killed ccnd daemon.");
	pthread_exit(NULL);
 }

