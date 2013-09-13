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
#include <net/if.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <glob.h>

#include "shared.h"

#include "swrd_types.h"
#include "swrd_configuration.h"


using namespace swr;

Configuration::Configuration() throw(Configuration_exception)
{
  struct ifreq ifr;
  char addr_str[INET_ADDRSTRLEN];
  struct sockaddr_in sa,*temp;

  int temp_sock;
  int temp_port=5555;

  // get my ip addr
  if((temp_sock = socket(PF_INET,SOCK_DGRAM,0)) < 0)
  {
    throw Configuration_exception("socket failed");
  }
  strcpy(ifr.ifr_name, "control");
  if(ioctl(temp_sock, SIOCGIFADDR, &ifr) < 0)
  {
    throw Configuration_exception("ioctl failed");
  }
  close(temp_sock);
  sa.sin_family = AF_INET;
  sa.sin_port = htons(temp_port);
  temp = (struct sockaddr_in *)&ifr.ifr_addr;
  sa.sin_addr.s_addr = temp->sin_addr.s_addr;
  control_address = (unsigned int) sa.sin_addr.s_addr;
  if(inet_ntop(AF_INET,&sa.sin_addr,addr_str,INET_ADDRSTRLEN) == NULL)
  {
    throw Configuration_exception("inet_ntop failed");
  }
  // done getting ip addr

  // last initialize all the locks used here
  pthread_mutex_init((&conf_lock), NULL);

  state = STOP;

  numConfiguredNics = 0;
  for (unsigned int i=0; i < max_nic; i++)
  {
    nicTable[i].configured = false;
    nicTable[i].rate = 0;
  }

  numConfiguredPorts = 0;
  for (unsigned int i=0; i < max_port; i++)
  {
    portTable[i].configured = false;
  }

  username = "";

}

Configuration::~Configuration() throw()
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
void Configuration::start_router() throw(Configuration_exception)
{
  autoLock glock(conf_lock);

  if(state == START)
  {
    write_log("start_router called, but the router has already been started..");
    glock.unlock();
    return;
  }

  write_log("start_router: entering");

  // Turn on ip_forwarding

  write_log("start_router: ip_forwarding turned on");

  // set up ip_tables rules so we do not route in/out the control interface

  write_log("start_router: done initializing ip_tables");

  /* the router is all ready to go now */
  state = START;

  glock.unlock();

  return;
}

// stop_router(): Turn off ip forwarding
void Configuration::stop_router() throw()
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

unsigned int Configuration::router_started() throw()
{
  autoLock glock(conf_lock);
  if(state == START) return 1;
  return 0;
}

void
Configuration::configure_port(unsigned int port, unsigned int realPort, uint16_t vlan, std::string ip, std::string maskStr, uint32_t rate) throw(Configuration_exception)
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
  struct in_addr addr;

  inet_pton(AF_INET, ip.c_str(), &addr);
  portTable[port].ip_addr = addr.s_addr;


  inet_pton(AF_INET, nexthop.c_str(), &addr);
  portTable[port].next_hop = addr.s_addr;

  if (portTable[port].configured == false) {
    char shcmd[256];
    char *device = "dataX"; // change device[4] to 0, 1, ... depending on which device
    portTable[port].configured = true;
    numConfiguredPorts++;
    write_log("Reconfiguring port " + int2str(port));
    //echo "Usage: $0 <portnum> <iface> <vlanNum> <ifaceIP> <ifaceMask> <ifaceRate> <ifaceDefaultMin> <iptMark>"
    //echo "Example: $0 1 data0 241 192.168.82.1 255.255.255.0 1000000 241"
    device[4] = '0' + (unsigned char) realPort;
#define IFACE_DEFAULT_MIN 1000
    sprintf(shcmd, "/usr/local/bin/swrd_configure_port %d %s %d %s %s %d %d", port, device, vlan, ip, maskStr, rate, IFACE_DEFAULT_MIN, vlan);
  }
  else {
    write_log("Initial configuration of port " + int2str(port));
  }

  write_log("Configuration::configure_node: port=" + int2str(port) + ", ipstr=" + ip + ", ip=" + portTable[port].ip_addr);


}


/* scripts/cfgRouterAddRoute.sh
 * echo "Usage: $0 <prefix> <mask> <dev> <vlan> [<gw>]"
 * echo "Example: $0 192.168.81.0 255.255.255.0 data0 241"
 * echo "  OR"
 * echo "Example: $0 192.168.81.0 255.255.255.0 data0 241 192.168.81.249"
 * exit 0
 */

// Add route to main route table with no gateway (next hop ip)
void Configuration::add_route_main(uint32_t prefix, uint32_t mask, uint16_t output_port) throw(Configuration_exception)
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
    char prefixStr[32];
    char maskStr[32];
    char nicStr[32];
    std::string tmp;

    // convert prefix and mask to ip dot string
    tmp = addr_int2str(prefix);
    tmp.copy(prefixStr, 0, 32);
    //prefixStr = addr_int2str(prefix);
    tmp = addr_int2str(mask);
    tmp.copy(maskStr, 0, 32);
    //maskStr = addr_int2str(mask);


    // get device and vlan from port_info struct
    nic = nicTable[portTable[output_port].nicIndex].nic;
    nic.copy(nicStr, 0, 32);

    sprintf(shcmd, "/usr/local/bin/swrd_add_route_main %s %s %s %d", prefixStr, maskStr, nicStr, portTable[output_port].vlan);
    if(system(shcmd) < 0)
    {
      throw Configuration_exception("system (/usr/local/bin/swrd_add_route_main failed");
    }


  }

  glock.unlock();

  write_log("add_route: done");

  return;
}

// Add route to main route table with a gateway (next hop ip)
void Configuration::add_route_main(uint32_t prefix, uint32_t mask, uint16_t output_port, uint32_t nexthop_ip) throw(Configuration_exception)
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
    char prefixStr[32];
    char maskStr[32];
    char nicStr[32];
    std::string tmp;

    // convert prefix and mask to ip dot string
    tmp = addr_int2str(prefix);
    tmp.copy(prefixStr, 0, 32);
    //prefixStr = addr_int2str(prefix);
    tmp = addr_int2str(mask);
    tmp.copy(maskStr, 0, 32);
    //maskStr = addr_int2str(mask);


    // get device and vlan from port_info struct
    // <gw> is optional?
    nic = nicTable[portTable[output_port].nicIndex].nic;
    nic.copy(nicStr, 0, 32);

    sprintf(shcmd, "/usr/local/bin/swrd_add_route_main %s %s %s %d %d", prefixStr, maskStr, nicStr, portTable[output_port].vlan, nexthop_ip);
    if(system(shcmd) < 0)
    {
      throw Configuration_exception("system (/usr/local/bin/swrd_add_route_main failed");
    }


  }

  glock.unlock();

  write_log("add_route: done");

  return;
}

// Add route to a port specific route table with no gateway (next hop ip)
void Configuration::add_route_port(uint16_t port, uint32_t prefix, uint32_t mask, uint16_t output_port) throw(Configuration_exception)
{
  bool is_multicast = false;

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
    char prefixStr[32];
    char maskStr[32];
    char nicStr[32];
    std::string tmp;
    std::string nic;

    // convert prefix and mask to ip dot string
    
    tmp = addr_int2str(prefix);
    tmp.copy(prefixStr, 0, 32);
    //prefixStr = addr_int2str(prefix);
    tmp = addr_int2str(mask);
    tmp.copy(maskStr, 0, 32);
    //maskStr = addr_int2str(mask);


    // get device and vlan from port_info struct
    // <gw> is optional?
    nic = nicTable[portTable[output_port].nicIndex].nic;
    nic.copy(nicStr, 0, 32);

    // port is used to direct this to a route table dedicated to a particular port
    sprintf(shcmd, "/usr/local/bin/swrd_add_route_port %s %s %s %d", prefixStr, maskStr, nicStr, portTable[output_port].vlan);
    if(system(shcmd) < 0)
    {
      throw Configuration_exception("system (/usr/local/bin/swrd_add_route_port failed");
    }


  }

  glock.unlock();

  write_log("add_route: done");

  return;
}
// Add route to a port specific route table with a gateway (next hop ip)
void Configuration::add_route_port(uint16_t port, uint32_t prefix, uint32_t mask, uint16_t output_port, uint32_t nexthop_ip) throw(Configuration_exception)
{
  bool is_multicast = false;

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
    char prefixStr[32];
    char maskStr[32];
    char nicStr[32];
    std::string tmp;
    std::string nic;

    // convert prefix and mask to ip dot string
    
    tmp = addr_int2str(prefix);
    tmp.copy(prefixStr, 0, 32);
    //prefixStr = addr_int2str(prefix);
    tmp = addr_int2str(mask);
    tmp.copy(maskStr, 0, 32);
    //maskStr = addr_int2str(mask);


    // get device and vlan from port_info struct
    // <gw> is optional?
    nic = nicTable[portTable[output_port].nicIndex].nic;
    nic.copy(nicStr, 0, 32);

    // port is used to direct this to a route table dedicated to a particular port
    sprintf(shcmd, "/usr/local/bin/swrd_add_route_port %s %s %s %d %d", prefixStr, maskStr, nicStr, portTable[output_port].vlan, nexthop_ip);
    if(system(shcmd) < 0)
    {
      throw Configuration_exception("system (/usr/local/bin/swrd_add_route_port failed");
    }


  }

  glock.unlock();

  write_log("add_route: done");

  return;
}

// Delete a route from the main route table with no gateway (next hop ip)
void Configuration::del_route_main(uint32_t prefix, uint32_t mask, uint16_t output_port) throw(Configuration_exception)
{
  bool is_multicast = false;

  write_log("del_route: entering");

  // for routes, there are two cases: unicast and multicast

  autoLock glock(conf_lock);
  if(is_multicast) // multicast route
  {
    write_log("del_route: got multicast route");

  }
  else // unicast route
  {
    // call cfgRouterAddRouter.sh prefix mask dev vlan <gw>
    char shcmd[256];
    char prefixStr[32];
    char maskStr[32];
    char nicStr[32];
    std::string tmp;
    std::string nic;

    // convert prefix and mask to ip dot string
    tmp = addr_int2str(prefix);
    tmp.copy(prefixStr, 0, 32);
    //prefixStr = addr_int2str(prefix);
    tmp = addr_int2str(mask);
    tmp.copy(maskStr, 0, 32);
    //maskStr = addr_int2str(mask);

    // get device and vlan from port_info struct
    // <gw> is optional?
    nic = nicTable[portTable[output_port].nicIndex].nic;
    nic.copy(nicStr, 0, 32);

    sprintf(shcmd, "/usr/local/bin/swrd_del_route_main %s %s %s %d", prefixStr, maskStr, nicStr, portTable[output_port].vlan);
    if(system(shcmd) < 0)
    {
      throw Configuration_exception("system (/usr/local/bin/swrd_del_route_main failed");
    }


  }

  glock.unlock();

  write_log("del_route: done");

  return;
}

// Delete a route from the main route table with a gateway (next hop ip)
void Configuration::del_route_main(uint32_t prefix, uint32_t mask, uint16_t output_port, uint32_t nexthop_ip) throw(Configuration_exception)
{
  bool is_multicast = false;

  write_log("del_route: entering");

  // for routes, there are two cases: unicast and multicast

  autoLock glock(conf_lock);
  if(is_multicast) // multicast route
  {
    write_log("del_route: got multicast route");

  }
  else // unicast route
  {
    // call cfgRouterAddRouter.sh prefix mask dev vlan <gw>
    char shcmd[256];
    char prefixStr[32];
    char maskStr[32];
    char nicStr[32];
    std::string tmp;
    std::string nic;

    // convert prefix and mask to ip dot string
    tmp = addr_int2str(prefix);
    tmp.copy(prefixStr, 0, 32);
    //prefixStr = addr_int2str(prefix);
    tmp = addr_int2str(mask);
    tmp.copy(maskStr, 0, 32);
    //maskStr = addr_int2str(mask);

    // get device and vlan from port_info struct
    // <gw> is optional?
    nic = nicTable[portTable[output_port].nicIndex].nic;
    nic.copy(nicStr, 0, 32);

    sprintf(shcmd, "/usr/local/bin/swrd_del_route_main %s %s %s %d %d", prefixStr, maskStr, nicStr, portTable[output_port].vlan, nexthop_ip);
    if(system(shcmd) < 0)
    {
      throw Configuration_exception("system (/usr/local/bin/swrd_del_route_main failed");
    }


  }

  glock.unlock();

  write_log("del_route: done");

  return;
}

// Delete a route from a port specific route table with no gateway (next hop ip)
void Configuration::del_route_port(uint16_t port, uint32_t prefix, uint32_t mask, uint16_t output_port) throw(Configuration_exception)
{
  bool is_multicast = false;

  write_log("del_route: entering");

  // for routes, there are two cases: unicast and multicast

  autoLock glock(conf_lock);
  if(is_multicast) // multicast route
  {
    write_log("del_route: got multicast route");

  }
  else // unicast route
  {
    // call cfgRouterAddRouter.sh prefix mask dev vlan <gw>
    char shcmd[256];
    char prefixStr[32];
    char maskStr[32];
    char nicStr[32];
    std::string nic;
    std::string tmp;

    // convert prefix and mask to ip dot string
    tmp = addr_int2str(prefix);
    tmp.copy(prefixStr, 0, 32);
    //prefixStr = addr_int2str(prefix);
    tmp = addr_int2str(mask);
    tmp.copy(maskStr, 0, 32);
    //maskStr = addr_int2str(mask);

    // get device and vlan from port_info struct
    // <gw> is optional?
    nic = nicTable[portTable[output_port].nicIndex].nic;
    nic.copy(nicStr, 0, 32);

    // port is used to direct this to a route table dedicated to a particular port
    sprintf(shcmd, "/usr/local/bin/swrd_del_route_port %s %s %s %d", prefixStr, maskStr, nicStr, portTable[output_port].vlan);
    if(system(shcmd) < 0)
    {
      throw Configuration_exception("system (/usr/local/bin/swrd_del_route_port failed");
    }


  }

  glock.unlock();

  write_log("del_route: done");

  return;
}
// Delete a route from a port specific route table with a gateway (next hop ip)
void Configuration::del_route_port(uint16_t port, uint32_t prefix, uint32_t mask, uint16_t output_port, uint32_t nexthop_ip) throw(Configuration_exception)
{
  bool is_multicast = false;

  write_log("del_route: entering");

  // for routes, there are two cases: unicast and multicast

  autoLock glock(conf_lock);
  if(is_multicast) // multicast route
  {
    write_log("del_route: got multicast route");

  }
  else // unicast route
  {
    // call cfgRouterAddRouter.sh prefix mask dev vlan <gw>
    char shcmd[256];
    char prefixStr[32];
    char maskStr[32];
    char nicStr[32];
    std::string nic;
    std::string tmp;

    // convert prefix and mask to ip dot string
    tmp = addr_int2str(prefix);
    tmp.copy(prefixStr, 0, 32);
    //prefixStr = addr_int2str(prefix);
    tmp = addr_int2str(mask);
    tmp.copy(maskStr, 0, 32);
    //maskStr = addr_int2str(mask);

    // get device and vlan from port_info struct
    // <gw> is optional?
    nic = nicTable[portTable[output_port].nicIndex].nic;
    nic.copy(nicStr, 0, 32);

    // port is used to direct this to a route table dedicated to a particular port
    sprintf(shcmd, "/usr/local/bin/swrd_del_route_port %s %s %s %d %d", prefixStr, maskStr, nicStr, portTable[output_port].vlan, nexthop_ip);
    if(system(shcmd) < 0)
    {
      throw Configuration_exception("system (/usr/local/bin/swrd_del_route_port failed");
    }


  }

  glock.unlock();

  write_log("del_route: done");

  return;
}


/*
unsigned int Configuration::conv_str_to_uint(std::string str) throw(Configuration_exception)
{
  const char* cstr = str.c_str();
  char* endptr;
  unsigned int val;
  errno = 0;
  val = strtoul(cstr, &endptr, 0);
  if((errno == ERANGE && (val == ULONG_MAX)) || (errno != 0 && val == 0))
  {
    throw Configuration_exception("invalid value");
  }
  if(endptr == cstr)
  {
    throw Configuration_exception("invalid value");
  }
  if(*endptr != '\0')
  {
    throw Configuration_exception("invalid value");
  }
  return val;
}
*/

/*
unsigned int Configuration::get_proto(std::string proto_str) throw(Configuration_exception)
{
  if(proto_str == "icmp" || proto_str == "ICMP")
  {
    return 1;
  }
  if(proto_str == "igmp" || proto_str == "IGMP")
  {
    return 2;
  }
  if(proto_str == "tcp" || proto_str == "tcp")
  {
    return 6;
  }
  if(proto_str == "udp" || proto_str == "udp")
  {
    return 17;
  }
  if(proto_str == "*")
  {
    return WILDCARD_VALUE;
  }
  unsigned int val;
  try
  {
    val = conv_str_to_uint(proto_str);
  }
  catch(std::exception& e)
  {
    throw Configuration_exception("protocol string is not valid");
  }
  return val;
}
*/

/*
unsigned int Configuration::get_tcpflags(unsigned int fin, unsigned int syn, unsigned int rst, unsigned int psh, unsigned int ack, unsigned urg) throw(Configuration_exception)
{
  unsigned val = 0;

  if(fin == 0 || fin == WILDCARD_VALUE) { }
  else if(fin == 1) { val = val | 1; }
  else { throw Configuration_exception("tcpfin value is not valid"); }

  if(syn == 0 || syn == WILDCARD_VALUE) { }
  else if(syn == 1) { val = val | 2; }
  else { throw Configuration_exception("tcpsyn value is not valid"); }

  if(rst == 0 || rst == WILDCARD_VALUE) { }
  else if(rst == 1) { val = val | 4; }
  else { throw Configuration_exception("tcprst value is not valid"); }

  if(psh == 0 || psh == WILDCARD_VALUE) { }
  else if(psh == 1) { val = val | 8; }
  else { throw Configuration_exception("tcppsh value is not valid"); }

  if(ack == 0 || ack == WILDCARD_VALUE) { }
  else if(ack == 1) { val = val | 16; }
  else { throw Configuration_exception("tcpack value is not valid"); }

  if(urg == 0 || urg == WILDCARD_VALUE) { }
  else if(urg == 1) { val = val | 32; }
  else { throw Configuration_exception("tcpurg value is not valid"); }

  return val;
}
*/

/*
unsigned int Configuration::get_tcpflags_mask(unsigned int fin, unsigned int syn, unsigned int rst, unsigned int psh, unsigned int ack, unsigned urg) throw(Configuration_exception)
{
  unsigned val = 0;

  if(fin == WILDCARD_VALUE) { }
  else if(fin == 0 || fin == 1) { val = val | 1; }
  else { throw Configuration_exception("tcpfin value is not valid"); }

  if(syn == WILDCARD_VALUE) { }
  else if(syn == 0 || syn == 1) { val = val | 2; }
  else { throw Configuration_exception("tcpsyn value is not valid"); }

  if(rst == WILDCARD_VALUE) { }
  else if(rst == 0 || rst == 1) { val = val | 4; }
  else { throw Configuration_exception("tcprst value is not valid"); }

  if(psh == WILDCARD_VALUE) { }
  else if(psh == 0 || psh == 1) { val = val | 8; }
  else { throw Configuration_exception("tcppsh value is not valid"); }

  if(ack == WILDCARD_VALUE) { }
  else if(ack == 0 || ack == 1) { val = val | 16; }
  else { throw Configuration_exception("tcpack value is not valid"); }

  if(urg == WILDCARD_VALUE) { }
  else if(urg == 0 || urg == 1) { val = val | 32; }
  else { throw Configuration_exception("tcpurg value is not valid"); }

  return val;
}
*/

/*
unsigned int Configuration::get_output_port(std::string port_str) throw(Configuration_exception)
{
  if(port_str == "")
  {
    return 0;
  }
  if(port_str == "use route")
  {
    return FILTER_USE_ROUTE;
  }
  
  unsigned int val;
  try
  {
    val = conv_str_to_uint(port_str);
  }
  catch(std::exception& e)
  {
    throw Configuration_exception("output port is not valid");
  }
  if(val > 4)
  {
    throw Configuration_exception("output port is not valid");
  }
  return val;
}
*/

/*
unsigned int Configuration::get_outputs(std::string port_str, std::string plugin_str) throw(Configuration_exception)
{
  unsigned int ports = 0;
  char portcstr[port_str.size()+1];
  strcpy(portcstr, port_str.c_str());
  char *tok, *saveptr;
  tok = strtok_r(portcstr, ",", &saveptr);
  try
  {
    while(tok)
    {
      std::string tmp = tok;
      unsigned int val = conv_str_to_uint(tmp);
      if(val > 4)
      {
        throw Configuration_exception("output port is not valid");
      }
      ports = ports | (1 << val); 
    
      tok = strtok_r(NULL, ",", &saveptr);
    }
  }
  catch(std::exception& e)
  {
    throw Configuration_exception("output port is not valid");
  }

  unsigned int plugins = 0;
  char plugincstr[plugin_str.size()+1];
  strcpy(plugincstr, plugin_str.c_str());
  char *tok2, *saveptr2;
  tok2 = strtok_r(plugincstr, ",", &saveptr2);
  try
  {
    while(tok2)
    {
      std::string tmp = tok2;
      unsigned int val = conv_str_to_uint(tmp);
      if(val > 4)
      {
        throw Configuration_exception("output plugin is not valid");
      }
      plugins = plugins | (1 << val);

      tok2 = strtok_r(NULL, ",", &saveptr2);
    }
  }
  catch(std::exception& e)
  {
    throw Configuration_exception("output plugin is not valid");
  }

  return ((ports << 5) | (plugins));
}
*/

/*
void Configuration::add_filter(pfilter_key *key, pfilter_key *mask, unsigned int priority, pfilter_result *result) throw(Configuration_exception)
{
  bool failed = false;

  write_log("add_filter: entering");

  if(priority > MAX_PRIORITY)
  {
    throw Configuration_exception("invalid priority");
  }


  autoLock glock(conf_lock);
  glock.unlock();

  if(failed)
  {
    throw Configuration_exception("adding the filter failed!");
  }

  write_log("add_filter: done");

  return;
}

void Configuration::del_filter(pfilter_key *key) throw(Configuration_exception)
{
  bool failed = false;
  write_log("del_filter: entering");


  if(failed)
  {
    throw Configuration_exception("deleting the filter failed!");
  }

  write_log("del_filter: done");

  return;
}
*/

/*
unsigned int Configuration::get_queue_quantum(unsigned int port, unsigned int queue) throw()
{

  queue = queue | ((port+1) << 13);
  write_log("get_queue_quantum: queue " + int2str(queue));

  unsigned int quantum = 0;
  return quantum;
}

//JP stopped here 7/8/13
void Configuration::set_queue_quantum(unsigned int port, unsigned int queue, unsigned int quantum) throw()
{
  unsigned int addr = QPARAMS_BASE_ADDR;

  queue = queue | ((port+1) << 13);
  write_log("set_queue_quantum: queue " + int2str(queue) + ", quantum " + int2str(quantum));


  addr += queue * QPARAMS_UNIT_SIZE;
}

unsigned int Configuration::get_queue_threshold(unsigned int port, unsigned int queue) throw()
{
  unsigned int threshold = 0;

  queue = queue | ((port+1) << 13);
  write_log("get_queue_threshold: queue " + int2str(queue));

  return threshold;
}

void Configuration::set_queue_threshold(unsigned int port, unsigned int queue, unsigned int threshold) throw()
{

  queue = queue | ((port+1) << 13);
  write_log("set_queue_threshold: queue " + int2str(queue) + ", threshold " + int2str(threshold));

}
*/

// rates should be in Kbps
unsigned int Configuration::get_port_rate(unsigned int port) throw(Configuration_exception)
{
  write_log("get_port_rate:");

  if(port > max_port)
  {
    throw Configuration_exception("invalid port");
  }
  if(!portTable[port].configured)
  {
    throw Configuration_exception("unconfigured port");
  }

  return (portTable[port].rate);
}

// rates should be in Kbps
void Configuration::set_port_rate(unsigned int port, unsigned int rate) throw(Configuration_exception)
{
  write_log("set_port_rate:");

  if(port > max_port)
  {
    throw Configuration_exception("invalid port");
  } 
  if(!portTable[port].configured)
  {
    throw Configuration_exception("unconfigured port");
  }
  portTable[port].rate = rate;
}

/*
unsigned int Configuration::get_port_addr(unsigned int port) throw()
{
  if(port > 4)
  {
    return 0;
  }
  return (portTable[port].ip_addr);
}

unsigned int Configuration::get_next_hop_addr(unsigned int port) throw()
{
  if(port > 4)
  {
    return 0;
  }
  return (portTable[port].next_hop);
}
*/

void Configuration::set_username(std::string un) throw()
{
  username = un;
  write_log("set_username: got user name " + username);
}

std::string Configuration::get_username() throw()
{
  return username;
}

std::string Configuration::addr_int2str(uint32_t addr)
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

