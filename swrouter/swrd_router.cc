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

#include <errno.h>
#include <string.h>

#include <unistd.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
//#include <net/if.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <glob.h>

#include <netlink/socket.h>
#include <netlink/route/qdisc.h>
#include <netlink/route/link.h>
#include <netlink/route/tc.h>

#include <boost/shared_ptr.hpp>

#include "shared.h"

#include "swrd_types.h"
#include "swrd_router.h"


using namespace swr;

Router::Router(int rtr_type) throw(configuration_exception)
{
  struct ifreq ifr;
  char addr_str[INET_ADDRSTRLEN];
  struct sockaddr_in sa,*temp;

  int temp_sock;
  int temp_port=5555;
  next_mark = 10;
  next_priority = 255;

  router_type = rtr_type;
  if (rtr_type == SWR_2P_10G)
    virtual_ports = true;
  else
    virtual_ports = false;

  // get my ip addr
  if((temp_sock = socket(PF_INET,SOCK_DGRAM,0)) < 0)
  {
    throw configuration_exception("socket failed");
  }
  strcpy(ifr.ifr_name, "control");
  if(ioctl(temp_sock, SIOCGIFADDR, &ifr) < 0)
  {
    throw configuration_exception("ioctl failed");
  }
  close(temp_sock);
  sa.sin_family = AF_INET;
  sa.sin_port = htons(temp_port);
  temp = (struct sockaddr_in *)&ifr.ifr_addr;
  sa.sin_addr.s_addr = temp->sin_addr.s_addr;
  control_address = (unsigned int) sa.sin_addr.s_addr;
  if(inet_ntop(AF_INET,&sa.sin_addr,addr_str,INET_ADDRSTRLEN) == NULL)
  {
    throw configuration_exception("inet_ntop failed");
  }
  // done getting ip addr

  // last initialize all the locks used here
  pthread_mutex_init((&conf_lock), NULL);

  state = STOP;

  numConfiguredNics = 0;
  for (unsigned int i=0; i < max_nic; i++)
  {
    nicTable[i].configured = true;//false;
    if (router_type == SWR_2P_10G)
      nicTable[i].rate = 10000000;
    else if (router_type == SWR_5P_1G)
      nicTable[i].rate = 1000000;
    nicTable[i].nic = "data" + int2str(i);
  }

  numConfiguredPorts = 0;
  for (unsigned int i=0; i < max_port; i++)
  {
    portTable[i].configured = false;
  }

  username = "";

}

Router::~Router() throw()
{
  if(state == START)
  {
    stop_router();
  }

  pthread_mutex_destroy(&conf_lock);

}

// start_router(): Do all the initial setup stuff before we start receiving RLI messages
//                 Turn on ip forwarding
//                 Initialize routes and ip_tables so that we do NOT route to or from control interface.
void Router::start_router() throw(configuration_exception)
{
  autoLock glock(conf_lock);

  if(state == START)
  {
    write_log("start_router called, but the router has already been started..");
    glock.unlock();
    return;
  }

  write_log("Router::start_router: entering");
  int num_ports = router_type;
  char shcmd[256];
  //run tuning script
  for (int i = 0; i < num_ports; ++i)
      {
	sprintf(shcmd, "/usr/local/bin/ixgb_perf.sh data%d /usr/local/bin", i);
	write_log("Router::start_router: tuning-system(" + std::string(shcmd) + ")");
	if (system_cmd(shcmd) < 0)
	  {
	    write_log("Router::start_router: tuning script for data" + int2str(i) + " failed");
	    throw configuration_exception("tuning failed");
	  }
      }
  
  write_log("Router::start_router: system_tuned");

  if (router_type == SWR_5P_1G)
    {
      sprintf(shcmd, "/usr/local/bin/swrd_iprules.sh");
      write_log("Router::start_router: adding iprules-system(" + std::string(shcmd) + ")");
      if (system_cmd(shcmd) < 0)
	{
	  write_log("Router::start_router: iprules script failed");
	  throw configuration_exception("iprules failed");
	}
    }
  
  
  /* the router is all ready to go now */
  state = START;
  
  glock.unlock();
  
  return;
}

// stop_router(): Turn off ip forwarding
void Router::stop_router() throw()
{

  autoLock glock(conf_lock);

  if(state == STOP)
  { 
    write_log("stop_router called, but the router is already stopped..");
    return;
  }
  // Turn off ip forwarding?
  write_log("stop_router: ip_forwarding turned off");

  state = STOP;

  write_log("stop_router: done");

  return;
}

unsigned int Router::router_started() throw()
{
  autoLock glock(conf_lock);
  if(state == START) return 1;
  return 0;
}

void
Router::configure_port(unsigned int port, unsigned int realPort, uint16_t vlan, std::string ip, std::string mask, uint32_t rate, std::string nhip) throw(configuration_exception)
{
  // For software router, port is virtual port and realPort is index to add to "data" to get the device
  // For example for an 8 port software router we might have this:
  //  port    realPort   Device
  //   0         0        data0
  //   1         0        data0
  //   2         0        data0
  //   3         0        data0
  //   4         1        data1
  //   5         1        data1
  //   6         1        data1
  //   7         1        data1

  // update portTable with this ports info

  //first check nicTable to see if there is enough bandwidth
  if (rate > nicTable[realPort].rate) 
    {
      write_log("Router::configure_port for port " + int2str(port) + " failed not enough bw on nic " + int2str(realPort) + " rate requested " + int2str(rate) + " nic rate " + int2str(nicTable[realPort].rate));
      throw configuration_exception("configure_port failed not enough bandwidth on nic " + int2str(realPort));
    }
  else nicTable[realPort].rate -= rate;

  struct in_addr addr;
  int mark = get_next_mark();

  inet_pton(AF_INET, ip.c_str(), &addr);
  portTable[port].ip_addr = addr.s_addr;

  inet_pton(AF_INET, nhip.c_str(), &addr);
  portTable[port].next_hop = addr.s_addr;

  //portTable[port].next_hop = ""; // no predefined next hop for this port
  portTable[port].vlan = vlan;
  portTable[port].nicIndex = realPort;
  portTable[port].rate = rate;
  portTable[port].mark = mark;
  if (portTable[port].configured == false) {
    char shcmd[256];
    //char device[] = "dataX"; // change device[4] to 0, 1, ... depending on which device
    //char ipStr[32];
    //char maskStr[32];

    // convert prefix and mask to ip dot string
    //ip.copy(ipStr, 32, 0);//0, 32);
    //mask.copy(maskStr, 32,0);//0, 32);

    portTable[port].configured = true;
    numConfiguredPorts++;
    write_log("Reconfiguring port " + int2str(port));
    //echo "Usage: $0 <portnum> <iface> <vlanNum> <ifaceIP> <ifaceMask> <ifaceRate> <ifaceDefaultMin> <iptMark>"
    //echo "Example: $0 1 data0 241 192.168.82.1 255.255.255.0 1000000 241"
    //device[4] = '0' + (unsigned char) realPort;
#define IFACE_DEFAULT_MIN 1000
    // sprintf(shcmd, "/usr/local/bin/swrd_configure_port %d %s %d %s %s %d %d %d", port, device, vlan, ipStr, maskStr, rate, IFACE_DEFAULT_MIN, vlan);
    sprintf(shcmd, "/usr/local/bin/swrd_configure_port.sh %d %s %d %s %s %d %d %d", port, nicTable[realPort].nic.c_str(), vlan, ip.c_str(), mask.c_str(), rate, IFACE_DEFAULT_MIN, mark);//vlan);
    write_log("Router::configure_port system(" + std::string(shcmd) + ")");
    if(system_cmd(shcmd) < 0)
      {
	throw configuration_exception("system (/usr/local/bin/swrd_configure_port failed");
      }
    sprintf(shcmd, "/usr/local/bin/swrd_add_route.sh 192.168.0.0 16 %s %d %s port%dFilters", nicTable[realPort].nic.c_str(), vlan, nhip.c_str(), port);
    write_log("Router::configure_port system(" + std::string(shcmd) + ")");
    if(system_cmd(shcmd) < 0)
      {
	throw configuration_exception("system (/usr/local/bin/swrd_add_route.sh failed");
      }
  }
  else {
    write_log("Initial configuration of port " + int2str(port));
  }

  write_log("Router::configure_node: port=" + int2str(port) + ", ipstr=" + ip + ", ip=" + portTable[port].ip_addr);


}


/* scripts/cfgRouterAddRoute.sh
 * echo "Usage: $0 <prefix> <mask> <dev> <vlan> [<gw>]"
 * echo "Example: $0 192.168.81.0 255.255.255.0 data0 241"
 * echo "  OR"
 * echo "Example: $0 192.168.81.0 255.255.255.0 data0 241 192.168.81.249"
 * exit 0
 */

// Add route to main route table with no gateway (next hop ip)
void Router::add_route_main(uint32_t prefix, uint32_t mask, uint16_t output_port) throw(configuration_exception)
{
  bool is_multicast = false;
  std::string nic;

  write_log("add_route: entering");

  // for routes, there are two cases: unicast and multicast

  autoLock glock(conf_lock);
  if(is_multicast) // multicast route
  {
    write_log("add_route: got multicast route");

  }
  else // unicast route
  {
    // call cfgRouterAddRouter.sh prefix mask dev vlan <gw>
    char shcmd[256];
    std::string prefixStr;
    std::string maskStr;

    // convert prefix and mask to ip dot string
    prefixStr = addr_int2str(prefix);
    maskStr = addr_int2str(mask);
    if (prefixStr.substr(0,3) != "192")
      {
	write_log("Router::add_route: prefix does not start with 192");
	throw configuration_exception("add route prefix needs to start with 192");
      }


    // get device and vlan from port_info struct
    nic = nicTable[portTable[output_port].nicIndex].nic;

    sprintf(shcmd, "/usr/local/bin/swrd_add_route.sh %s %d %s %d main", prefixStr.c_str(), mask, nic.c_str(), portTable[output_port].vlan);
    write_log("Router::add_route_main system(" + std::string(shcmd) + ")");
    int rtn = system_cmd(shcmd);
    if(rtn != 0)
    {
      throw configuration_exception("system (/usr/local/bin/swrd_add_route main failed");
    }

    //write_log("Router::add_route_main system(" + std::string(shcmd) + ") rtn=" + int2str(rtn));

  }

  glock.unlock();

  write_log("add_route: done");

  return;
}

// Add route to main route table with a gateway (next hop ip)
void Router::add_route_main(uint32_t prefix, uint32_t mask, uint16_t output_port, uint32_t nexthop_ip) throw(configuration_exception)
{
  bool is_multicast = false;
  std::string nic;

  write_log("add_route_main: entering");

  // for routes, there are two cases: unicast and multicast

  autoLock glock(conf_lock);
  if(is_multicast) // multicast route
  {
    write_log("add_route_main: got multicast route");

  }
  else // unicast route
  {
    // call cfgRouterAddRouter.sh prefix mask dev vlan <gw>
    char shcmd[256];
    std::string prefixStr;//char prefixStr[32];
    //std::string maskStr;//char maskStr[32];
    std::string nhipStr;
    //char nicStr[32];
    std::string tmp;

    // convert prefix and mask to ip dot string
    prefixStr = addr_int2str(prefix);
    //maskStr = addr_int2str(mask);
    nhipStr = addr_int2str(nexthop_ip);


    // get device and vlan from port_info struct
    // <gw> is optional?
    nic = nicTable[portTable[output_port].nicIndex].nic; 

    sprintf(shcmd, "/usr/local/bin/swrd_add_route.sh %s %d %s %d %s main", prefixStr.c_str(), mask, nic.c_str(), portTable[output_port].vlan, nhipStr.c_str());
    write_log("Router::add_route_main system(" + std::string(shcmd) + ")");
    int rtn = system_cmd(shcmd);
    if(rtn != 0)
    {
      throw configuration_exception("system (/usr/local/bin/swrd_add_route main failed");
    }

    // write_log("Router::add_route_main system(" + std::string(shcmd) + ") rtn=" + int2str(rtn));

  }

  glock.unlock();

  write_log("add_route: done");

  return;
}

// Add route to a port specific route table with no gateway (next hop ip)
void Router::add_route_port(uint16_t port, uint32_t prefix, uint32_t mask, uint16_t output_port) throw(configuration_exception)
{
  bool is_multicast = false;
  std::string nic;

  write_log("add_route_port: entering");

  // for routes, there are two cases: unicast and multicast

  autoLock glock(conf_lock);
  if(is_multicast) // multicast route
  {
    write_log("add_route_port: got multicast route");

  }
  else // unicast route
  {
    // call cfgRouterAddRouter.sh prefix mask dev vlan <gw>
    char shcmd[256];
    std::string prefixStr;
    //std::string maskStr;

    // convert prefix and mask to ip dot string
    prefixStr = addr_int2str(prefix);
    //maskStr = addr_int2str(mask);


    // get device and vlan from port_info struct
    // <gw> is optional?
    nic = nicTable[portTable[output_port].nicIndex].nic;

    // port is used to direct this to a route table dedicated to a particular port
    sprintf(shcmd, "/usr/local/bin/swrd_add_route.sh %s %d %s %d port%dRoutes", prefixStr.c_str(), mask, nic.c_str(), portTable[output_port].vlan, port);
    write_log("Router::add_route_port system(" + std::string(shcmd) + ")");
    if(system_cmd(shcmd) < 0)
    {
      throw configuration_exception("system (/usr/local/bin/swrd_add_route port failed");
    }


  }

  glock.unlock();

  write_log("add_route_port: done");

  return;
}
// Add route to a port specific route table with a gateway (next hop ip)
void Router::add_route_port(uint16_t port, uint32_t prefix, uint32_t mask, uint16_t output_port, uint32_t nexthop_ip) throw(configuration_exception)
{
  bool is_multicast = false;
  std::string nic;

  write_log("add_route_port: entering");

  // for routes, there are two cases: unicast and multicast

  autoLock glock(conf_lock);
  if(is_multicast) // multicast route
  {
    write_log("add_route_port: got multicast route");

  }
  else // unicast route
  {
    // call cfgRouterAddRouter.sh prefix mask dev vlan <gw>
    char shcmd[256];
    std::string prefixStr;
    //std::string maskStr;
    std::string nhipStr;

    // convert prefix and mask to ip dot string
    prefixStr = addr_int2str(prefix);
    //maskStr = addr_int2str(mask);
    nhipStr = addr_int2str(nexthop_ip);


    // get device and vlan from port_info struct
    // <gw> is optional?
    nic = nicTable[portTable[output_port].nicIndex].nic;

    // port is used to direct this to a route table dedicated to a particular port
    sprintf(shcmd, "/usr/local/bin/swrd_add_route.sh %s %d %s %d %s port%dRoutes", prefixStr.c_str(), mask, nic.c_str(), portTable[output_port].vlan, nhipStr.c_str(), port);
    write_log("Router::add_route_port system(" + std::string(shcmd) + ")");
    if(system_cmd(shcmd) < 0)
    {
      throw configuration_exception("system (/usr/local/bin/swrd_add_route port failed");
    }


  }

  glock.unlock();

  write_log("add_route_port: done");

  return;
}

// Delete a route from the main route table with no gateway (next hop ip)
void Router::del_route_main(uint32_t prefix, uint32_t mask, uint16_t output_port) throw(configuration_exception)
{
  bool is_multicast = false;

  write_log("del_route_main: entering");

  // for routes, there are two cases: unicast and multicast

  autoLock glock(conf_lock);
  if(is_multicast) // multicast route
  {
    write_log("del_route_main: got multicast route");

  }
  else // unicast route
  {
    // call cfgRouterAddRouter.sh prefix mask dev vlan <gw>
    char shcmd[256];
    std::string prefixStr; //char prefixStr[32];
    //std::string maskStr;//char maskStr[32];
    std::string tmp;
    std::string nic;

    // convert prefix and mask to ip dot string
    prefixStr = addr_int2str(prefix);
    //maskStr = addr_int2str(mask);

    // get device and vlan from port_info struct
    // <gw> is optional?
    nic = nicTable[portTable[output_port].nicIndex].nic;

    sprintf(shcmd, "/usr/local/bin/swrd_del_route.sh %s %d %s %d main", prefixStr.c_str(), mask, nic.c_str(), portTable[output_port].vlan);
    write_log("Router::del_route_main system(" + std::string(shcmd) + ")");
    if(system_cmd(shcmd) < 0)
    {
      throw configuration_exception("system (/usr/local/bin/swrd_del_route main failed");
    }


  }

  glock.unlock();

  write_log("del_route_main: done");

  return;
}

// Delete a route from the main route table with a gateway (next hop ip)
void Router::del_route_main(uint32_t prefix, uint32_t mask, uint16_t output_port, uint32_t nexthop_ip) throw(configuration_exception)
{
  bool is_multicast = false;
  std::string nic;

  write_log("del_route_main: entering");

  // for routes, there are two cases: unicast and multicast

  autoLock glock(conf_lock);
  if(is_multicast) // multicast route
  {
    write_log("del_route_main: got multicast route");

  }
  else // unicast route
  {
    // call cfgRouterAddRouter.sh prefix mask dev vlan <gw>
    char shcmd[256];
    std::string prefixStr; 
    //std::string maskStr;
    std::string nhipStr;

    // convert prefix and mask to ip dot string
    prefixStr = addr_int2str(prefix);
    //maskStr = addr_int2str(mask);
    nhipStr = addr_int2str(nexthop_ip);

    // get device and vlan from port_info struct
    // <gw> is optional?
    nic = nicTable[portTable[output_port].nicIndex].nic;

    sprintf(shcmd, "/usr/local/bin/swrd_del_route.sh %s %d %s %d %s main", prefixStr.c_str(), mask, nic.c_str(), portTable[output_port].vlan, nhipStr.c_str());
    write_log("Router::del_route_main system(" + std::string(shcmd) + ")");
    if(system_cmd(shcmd) < 0)
    {
      throw configuration_exception("system (/usr/local/bin/swrd_del_route main failed");
    }


  }

  glock.unlock();

  write_log("del_route_main: done");

  return;
}

// Delete a route from a port specific route table with no gateway (next hop ip)
void Router::del_route_port(uint16_t port, uint32_t prefix, uint32_t mask, uint16_t output_port) throw(configuration_exception)
{
  bool is_multicast = false;
  std::string nic;

  write_log("del_route_port: entering");

  // for routes, there are two cases: unicast and multicast

  autoLock glock(conf_lock);
  if(is_multicast) // multicast route
  {
    write_log("del_route_port: got multicast route");

  }
  else // unicast route
  {
    // call cfgRouterAddRouter.sh prefix mask dev vlan <gw>
    char shcmd[256];
    std::string prefixStr;
    //std::string maskStr;

    // convert prefix and mask to ip dot string
    prefixStr = addr_int2str(prefix);
    //maskStr = addr_int2str(mask);

    // get device and vlan from port_info struct
    // <gw> is optional?
    nic = nicTable[portTable[output_port].nicIndex].nic;

    // port is used to direct this to a route table dedicated to a particular port
    sprintf(shcmd, "/usr/local/bin/swrd_del_route.sh %s %d %s %d port%dRoutes", prefixStr.c_str(), mask, nic.c_str(), portTable[output_port].vlan, port);
    write_log("Router::del_route_port system(" + std::string(shcmd) + ")");
    if(system_cmd(shcmd) < 0)
    {
      throw configuration_exception("system (/usr/local/bin/swrd_del_route port failed");
    }


  }

  glock.unlock();

  write_log("del_route_port: done");

  return;
}

// Delete a route from a port specific route table with a gateway (next hop ip)
void Router::del_route_port(uint16_t port, uint32_t prefix, uint32_t mask, uint16_t output_port, uint32_t nexthop_ip) throw(configuration_exception)
{
  bool is_multicast = false;
  std::string nic;

  write_log("del_route_port: entering");

  // for routes, there are two cases: unicast and multicast

  autoLock glock(conf_lock);
  if(is_multicast) // multicast route
  {
    write_log("del_route_port: got multicast route");

  }
  else // unicast route
  {
    // call cfgRouterAddRouter.sh prefix mask dev vlan <gw>
    char shcmd[256];
    std::string prefixStr;
    //std::string maskStr; 
    std::string nhipStr;

    // convert prefix and mask to ip dot string
    prefixStr = addr_int2str(prefix);
    //maskStr = addr_int2str(mask);
    nhipStr = addr_int2str(nexthop_ip);

    // get device and vlan from port_info struct
    // <gw> is optional?
    nic = nicTable[portTable[output_port].nicIndex].nic;

    // port is used to direct this to a route table dedicated to a particular port
    sprintf(shcmd, "/usr/local/bin/swrd_del_route.sh %s %d %s %d %s port%dRoutes", prefixStr.c_str(), mask, nic.c_str(), portTable[output_port].vlan,  nhipStr.c_str(), port);
    write_log("Router::del_route_port system(" + std::string(shcmd) + ")");
    if(system_cmd(shcmd) < 0)
    {
      throw configuration_exception("system (/usr/local/bin/swrd_del_route port failed");
    }

  }

  glock.unlock();

  write_log("del_route_port: done");

  return;
}



// rates should be in Kbps
unsigned int Router::get_port_rate(unsigned int port) throw(configuration_exception)
{
  write_log("get_port_rate:");

  if(port > max_port)
  {
    throw configuration_exception("invalid port");
  }
  if(!portTable[port].configured)
  {
    throw configuration_exception("unconfigured port");
  }

  return (portTable[port].rate);
}

// rates should be in Kbps
void Router::set_port_rate(unsigned int port, unsigned int rate) throw(configuration_exception)
{
  write_log("set_port_rate:");

  if(port > max_port)
  {
    throw configuration_exception("invalid port");
  } 
  if(!portTable[port].configured)
  {
    throw configuration_exception("unconfigured port");
  }
  portTable[port].rate = rate;
}

void Router::add_filter(filter_ptr f) throw(configuration_exception)
{ 
  f->mark = get_next_mark();
  f->rule_priority = get_next_priority();
  if (f->rule_priority < 0)
    {
      write_log("Router::add_filter: rule priority maxed out");
      throw configuration_exception("add_filter:rule priority maxed out");
    }

  filter_command(f, false);
  
  autoLock cnflock(conf_lock);
  filters.push_back(f);
}

void Router::del_filter(filter_ptr f) throw(configuration_exception)
{
  filter_ptr rtn_f = get_filter(f);
  if (rtn_f) 
    {
      filter_command(rtn_f, true);
      autoLock cnflock(conf_lock);
      filters.remove(rtn_f);
    }
}

filter_ptr Router::get_filter(filter_ptr f)
{
  filter_ptr empty_ptr;
  std::list<filter_ptr>::iterator fit;
  for (fit = filters.begin(); fit != filters.end(); ++fit)
    {
      if ((*fit)->dest_prefix == f->dest_prefix &&
	  (*fit)->dest_mask == f->dest_mask &&
	  (*fit)->src_prefix == f->src_prefix &&
	  (*fit)->src_mask == f->src_mask &&
	  (*fit)->protocol == f->protocol &&
	  (*fit)->dest_port == f->dest_port &&
	  (*fit)->src_port == f->src_port &&
	  (*fit)->tcp_fin == f->tcp_fin &&
	  (*fit)->tcp_syn == f->tcp_syn &&
	  (*fit)->tcp_rst == f->tcp_rst &&
	  (*fit)->tcp_psh == f->tcp_psh &&
	  (*fit)->tcp_ack == f->tcp_ack &&
	  (*fit)->tcp_urg == f->tcp_urg &&
	  (*fit)->unicast_drop == f->unicast_drop)
	return (*fit);
    }
  return empty_ptr;
}


void Router::filter_command(filter_ptr f, bool isdel) throw(configuration_exception)
{

  std::string shcmd = "iptables -t mangle";
  std::string command_type = "add_filter";
  if (!isdel)
    {
      if ((f->dest_prefix.substr(0,3) != "192" && f->dest_prefix != "*") ||
	  (f->src_prefix.substr(0,3) != "192" && f->src_prefix != "*"))
      {
	write_log("Router::add_route: prefix does not start with 192");
	throw configuration_exception("add filter prefix needs to start with 192");
      }
      shcmd += " -I PREROUTING";
    }
  else
    {
      shcmd += " -D PREROUTING";
      command_type = "del_filter";
    }

  bool use_ports = false;
  bool is_tcp = false;
  if (f->protocol == "tcp") is_tcp = true;
  if (is_tcp || f->protocol == "udp" || f->protocol == "dccp" || f->protocol == "sctp") use_ports = true;
  if (f->protocol != "*") 
    shcmd += " -p " + f->protocol;
  if (f->dest_prefix != "*")
    shcmd += " -d " + f->dest_prefix + "/" + int2str(f->dest_mask);
  if (f->src_prefix != "*")
    shcmd += " -s " + f->src_prefix + "/" + int2str(f->src_mask);
  if (f->dest_port > 0 && use_ports)
    shcmd += " --dport " + int2str(f->dest_port);
  if (f->src_port > 0 && use_ports)
    shcmd += " --sport " + int2str(f->src_port);
  if (is_tcp && 
      (f->tcp_fin >= 0 ||
       f->tcp_syn >= 0 ||
       f->tcp_rst >= 0 ||
       f->tcp_psh >= 0 ||
       f->tcp_ack >= 0 ||
       f->tcp_urg >= 0))
    {
      shcmd += " --tcp-flags ";
      bool list_started = false;
      std::string on_flags;
      bool on = false;
      if (f->tcp_fin >= 0) 
	{
	  shcmd += " FIN";
	  list_started = true;
	  if (f->tcp_fin > 0) on_flags += " FIN";
	}
      if (f->tcp_syn >= 0) 
	{
	  if (f->tcp_syn > 0) on = true;
	  if (list_started) 
	    {
	      if (on) on_flags += ",";
	      shcmd += ",";
	    }
	  else shcmd += " ";
	  shcmd += "SYN";
	  if (on) on_flags += "SYN";
	  list_started = true;
	}
      if (f->tcp_rst >= 0) 
	{
	  if (f->tcp_rst > 0) on = true;
	  if (list_started) 
	    {
	      if (on) on_flags += ",";
	      shcmd += ",";
	    }
	  else shcmd += " ";
	  shcmd += "RST";
	  if (on) on_flags += "RST";
	  list_started = true;
	}
      if (f->tcp_psh >= 0) 
	{
	  if (f->tcp_psh > 0) on = true;
	  if (list_started) 
	    {
	      if (on) on_flags += ",";
	      shcmd += ",";
	    }
	  else shcmd += " ";
	  shcmd += "PSH";
	  if (on) on_flags += "PSH";
	  list_started = true;
	}
      if (f->tcp_ack >= 0) 
	{
	  if (f->tcp_ack > 0) on = true;
	  if (list_started) 
	    {
	      if (on) on_flags += ",";
	      shcmd += ",";
	    }
	  else shcmd += " ";
	  shcmd += "ACK";
	  if (on) on_flags += "ACK";
	  list_started = true;
	}
      if (f->tcp_urg >= 0) 
	{
	  if (f->tcp_urg > 0) on = true;
	  if (list_started) 
	    {
	      if (on) on_flags += ",";
	      shcmd += ",";
	    }
	  else shcmd += " ";
	  shcmd += "URG";
	  if (on) on_flags += "URG";
	  list_started = true;
	}

      shcmd += on_flags;
    }
  if (f->sampling > 1)
    shcmd += " --mode nth --every " + int2str(f->sampling);
  
  shcmd += " -j MARK --set-mark " + int2str(f->mark);
  
  write_log("Router::" + command_type + ": " + shcmd);
  if (system_cmd(shcmd) < 0)
    {
      write_log("Router::" + command_type + ": iptables failed");
      throw configuration_exception("iptables failed");
    }
  
  //set ip rule
  if (!isdel)
    shcmd = "ip rule add";
  else shcmd = "ip rule del";
  if (f->unicast_drop) shcmd += " blackhole";
  shcmd += " fwmark " + int2str(f->mark);
  if (!f->unicast_drop)
    {
      if (f->output_port == "use route") f->table = "main";
      else
	{
	  f->table = "port" + f->output_port + "Filters";
	}
      shcmd += " table " + f->table;
    }

  shcmd += " priority " + int2str(f->rule_priority);
  
  
  write_log("Router::" + command_type + ": " + shcmd);
  if (system_cmd(shcmd) < 0)
    {
      write_log("Router::" + command_type + ": ip rule failed");
      throw configuration_exception("ip rule failed");
    }
}
/*
unsigned int Router::get_port_addr(unsigned int port) throw()
{
  if(port > 4)
  {
    return 0;
  }
  return (portTable[port].ip_addr);
}

unsigned int Router::get_next_hop_addr(unsigned int port) throw()
{
  if(port > 4)
  {
    return 0;
  }
  return (portTable[port].next_hop);
}
*/

void Router::set_username(std::string un) throw()
{
  username = un;
  write_log("set_username: got user name " + username);
}

std::string Router::get_username() throw()
{
  return username;
}

std::string Router::addr_int2str(uint32_t addr)
{
  char addr_cstr[INET_ADDRSTRLEN];
  struct in_addr ia;
  ia.s_addr = htonl(addr);
  if(inet_ntop(AF_INET,&ia,addr_cstr,INET_ADDRSTRLEN) == NULL)
  {
    return "";
  }
  return std::string(addr_cstr);
}


uint64_t 
Router::read_stats_pkts(int port) throw(monitor_exception)
{
  uint64_t pkts = read_stats(port, RTNL_TC_PACKETS);
  write_log("Router::read_stats_pkts for port(" + int2str(port) + ") packets: " + int2str(pkts));
  return pkts;
}

uint64_t 
Router::read_stats_bytes(int port) throw(monitor_exception)
{  
  uint64_t bytes = read_stats(port, RTNL_TC_BYTES);
  write_log("Router::read_stats_bytes for port(" + int2str(port) + ") bytes: " + int2str(bytes));
  return bytes;
}

uint64_t 
Router::read_stats_qlength(int port) throw(monitor_exception)
{  
  uint64_t rtn = read_stats(port, RTNL_TC_QLEN);
  write_log("Router::read_stats_qlength for port(" + int2str(port) + ") qlength: " + int2str(rtn));
  return rtn;
}

uint64_t 
Router::read_stats_drops(int port) throw(monitor_exception)
{  
  uint64_t rtn = read_stats(port, RTNL_TC_DROPS);
  write_log("Router::read_stats_drops for port(" + int2str(port) + ") drops: " + int2str(rtn));
  return rtn;
}

uint64_t 
Router::read_stats_backlog(int port) throw(monitor_exception)
{  
  uint64_t rtn = read_stats(port, RTNL_TC_BACKLOG);
  write_log("Router::read_stats_backlog for port(" + int2str(port) + ") backlog: " + int2str(rtn));
  return rtn;
}

uint64_t 
Router::read_stats(int port, enum rtnl_tc_stat sid) throw(monitor_exception) //enum rtnl_tc_stat
{  
  struct nl_sock *my_sock = nl_socket_alloc();
  struct nl_cache *link_cache, *all_qdiscs;
  struct rtnl_link *link_ptr;
  struct rtnl_qdisc *qdisc;
  uint64_t rtn_stat = 0;
  int rtn = 0;
  std::string nic;

  if (!portTable[port].configured) return 0;
  nl_connect(my_sock, NETLINK_ROUTE);
  if ((rtn = rtnl_link_alloc_cache(my_sock, AF_UNSPEC, &link_cache)) < 0)
    {
      nl_socket_free(my_sock);
      throw monitor_exception("rtnl_link_alloc_cache failed");
    }

  if (virtual_ports)
    nic = (nicTable[portTable[port].nicIndex].nic) + "." + int2str(portTable[port].vlan);
  else
    nic = nicTable[portTable[port].nicIndex].nic;
  if (!(link_ptr = rtnl_link_get_by_name(link_cache, nic.c_str())))
    {
      nl_socket_free(my_sock);
      nl_cache_free(link_cache);
      throw monitor_exception("rtnl_link_get_by_name failed");
    }

  int ifindex = rtnl_link_get_ifindex(link_ptr);

  if ((rtn = rtnl_qdisc_alloc_cache(my_sock, &all_qdiscs)) < 0)
    {
      nl_socket_free(my_sock);
      nl_cache_free(link_cache);
      throw monitor_exception("rtnl_qdisc_alloc_cache failed ");
    }

  if (!(qdisc = rtnl_qdisc_get(all_qdiscs, ifindex, TC_HANDLE((port + 1),0))))
    {
      nl_socket_free(my_sock);
      nl_cache_free(link_cache);
      nl_cache_free(all_qdiscs);
      throw monitor_exception("rtnl_qdisc_get failed");
    }

  rtn_stat = rtnl_tc_get_stat(TC_CAST(qdisc), sid);
  nl_socket_free(my_sock);
  nl_cache_free(link_cache);
  nl_cache_free(all_qdiscs);
  return rtn_stat;
}


uint64_t 
Router::read_link_stats(int port, rtnl_link_stat_id_t sid) throw(monitor_exception) //enum rtnl_tc_stat
{ 
  struct nl_sock *my_sock = nl_socket_alloc();
  struct nl_cache *link_cache;
  struct rtnl_link *link_ptr;
  uint64_t rtn_stat = 0;
  int rtn = 0;
  std::string nic;

  if (!portTable[port].configured) return 0;
  nl_connect(my_sock, NETLINK_ROUTE);
  if ((rtn = rtnl_link_alloc_cache(my_sock, AF_UNSPEC, &link_cache)) < 0)
    {
      nl_socket_free(my_sock);
      throw monitor_exception("rtnl_link_alloc_cache failed");
    }

  if (virtual_ports)
    nic = (nicTable[portTable[port].nicIndex].nic) + "." + int2str(portTable[port].vlan);
  else
    nic = nicTable[portTable[port].nicIndex].nic;

  if (!(link_ptr = rtnl_link_get_by_name(link_cache, nic.c_str())))
    {
      nl_socket_free(my_sock);
      nl_cache_free(link_cache);
      throw monitor_exception("rtnl_link_get_by_name failed");
    }

  rtn_stat = rtnl_link_get_stat(link_ptr, sid);
  nl_cache_free(link_cache);
  nl_socket_free(my_sock);
  return rtn_stat;
}


uint64_t 
Router::read_link_stats_rxpkts(int port) throw(monitor_exception)
{
  uint64_t pkts = read_link_stats(port, RTNL_LINK_RX_PACKETS);
  write_log("Router::read_link_stats_rxpkts for port(" + int2str(port) + ") packets: " + int2str(pkts));
  return pkts;
}


uint64_t 
Router::read_link_stats_txpkts(int port) throw(monitor_exception)
{
  uint64_t pkts = read_link_stats(port, RTNL_LINK_TX_PACKETS);
  write_log("Router::read_link_stats_txpkts for port(" + int2str(port) + ") packets: " + int2str(pkts));
  return pkts;
}


uint64_t 
Router::read_link_stats_rxbytes(int port) throw(monitor_exception)
{
  uint64_t bytes = read_link_stats(port, RTNL_LINK_RX_BYTES);
  write_log("Router::read_link_stats_rxbytes for port(" + int2str(port) + ") bytes: " + int2str(bytes));
  return bytes;
}


uint64_t 
Router::read_link_stats_txbytes(int port) throw(monitor_exception)
{
  uint64_t bytes = read_link_stats(port, RTNL_LINK_TX_BYTES);
  write_log("Router::read_link_stats_txbytes for port(" + int2str(port) + ") bytes: " + int2str(bytes));
  return bytes;
}

uint64_t 
Router::read_link_stats_rxerrors(int port) throw(monitor_exception)
{
  uint64_t pkts = read_link_stats(port, RTNL_LINK_RX_ERRORS);
  write_log("Router::read_link_stats_rxerrors for port(" + int2str(port) + ") packets: " + int2str(pkts));
  return pkts;
}

uint64_t 
Router::read_link_stats_txerrors(int port) throw(monitor_exception)
{
  uint64_t pkts = read_link_stats(port, RTNL_LINK_TX_ERRORS);
  write_log("Router::read_link_stats_txerrors for port(" + int2str(port) + ") packets: " + int2str(pkts));
  return pkts;
}

uint64_t 
Router::read_link_stats_rxdrops(int port) throw(monitor_exception)
{
  uint64_t pkts = read_link_stats(port, RTNL_LINK_RX_DROPPED);
  write_log("Router::read_link_stats_rxdrops for port(" + int2str(port) + ") packets: " + int2str(pkts));
  return pkts;
}

uint64_t 
Router::read_link_stats_txdrops(int port) throw(monitor_exception)
{
  uint64_t pkts = read_link_stats(port, RTNL_LINK_TX_DROPPED);
  write_log("Router::read_link_stats_txdrops for port(" + int2str(port) + ") packets: " + int2str(pkts));
  return pkts;
}

int
Router::get_next_mark()
{
  return next_mark++;
}

int 
Router::get_next_priority()
{
  if (next_priority >= 32700) return -1;
  else return next_priority++; 
}



int
Router::system_cmd(std::string cmd)
{
  //write_log("Router::system_cmd(): cmd = " + cmd);
  int rtn = system(cmd.c_str());
  if(rtn == -1) return rtn;
  return WEXITSTATUS(rtn);
}
