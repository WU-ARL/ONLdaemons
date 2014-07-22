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
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "shared.h"

#include "vmd_configuration.h"
#include "vmd_globals.h"

using namespace vmd;

configuration::configuration()
{
  port[0].nic = "data0";
  port[1].nic = "data1";
}

configuration::~configuration()
{ 
}

void
configuration::set_network_info(uint32_t portnum, std::string ip, std::string subnet, std::string nexthop)
{
  std::string logstr = "configuration::set_port_info(): port=" + int2str(portnum);
  write_log(logstr);

  if(portnum > max_port) return;

  port[portnum].ip_addr = ip;
  port[portnum].next_hop = nexthop;
  port[portnum].subnet = subnet;

  if(port[portnum].next_hop == "0.0.0.0")
  {
    //JP: 3_22_12 changed to launch subtype daemon as user 
    system_cmd("sudo /usr/local/bin/onl_cfgport.sh " + port[portnum].nic + " " + port[portnum].ip_addr + " 255.255.255.0");
    //system_cmd("/sbin/ifconfig " + port[portnum].nic + " down");
    //system_cmd("/sbin/ifconfig " + port[portnum].nic + " " + port[portnum].ip_addr + " netmask 255.255.255.0");
    //system_cmd("/sbin/ifconfig " + port[portnum].nic + " up");

    //end change JP: 3_22_12 changed to launch subtype daemon as user 
    //system_cmd("/sbin/route add -net 224.0.0.0 netmask 240.0.0.0 " + port[portnum].nic);
  }
  else
  {
    //JP: 3_22_12 changed to launch subtype daemon as user 
    system_cmd("sudo /usr/local/bin/onl_cfgport.sh " + port[portnum].nic + " " + port[portnum].ip_addr + " " + port[portnum].subnet);
    //system_cmd("sudo /usr/local/bin/onl_cfgport.sh " + port[portnum].nic + " " + port[portnum].ip_addr + " 255.255.255.240");
    //system_cmd("/sbin/ifconfig " + port[portnum].nic + " down");
    //system_cmd("/sbin/ifconfig " + port[portnum].nic + " " + port[portnum].ip_addr + " netmask 255.255.255.240");
    //system_cmd("/sbin/ifconfig " + port[portnum].nic + " up");

    system_cmd("sudo /usr/local/bin/onl_addroute.sh 192.168.0.0 255.255.0.0 " + port[portnum].nic);
    //end change JP: 3_22_12 changed to launch subtype daemon as user 
    //system_cmd("/sbin/route add -net 192.168.0.0 netmask 255.255.0.0 gw " + port[portnum].next_hop);
    //system_cmd("/sbin/route add -net 224.0.0.0 netmask 240.0.0.0 gw " + port[portnum].next_hop);
  }

  //JP: 3_22_12 changed to launch subtype daemon as user 
  //system_cmd("echo 14400 > /proc/sys/net/ipv4/neigh/" + port[portnum].nic + "/gc_stale_time");
  //system_cmd("arping -q -c 3 -A -I " + port[portnum].nic + " " + port[portnum].ip_addr);
  //end change JP: 3_22_12 changed to launch subtype daemon as user 
}

bool
configuration::add_route(uint16_t portnum, std::string prefix, uint32_t mask, std::string nexthop)
{
  std::string cmd = "sudo /usr/local/bin/onl_addroute.sh " + prefix + " " + int2str(mask);
  //JP: 3_22_12 changed to launch subtype daemon as user 
  //std::string cmd = "/sbin/route add -net " + prefix + "/" + int2str(mask);
  if(nexthop == "0.0.0.0")
  {
    cmd += " dev " + port[portnum].nic;
  }
  else
  {
    cmd += " gw " + nexthop;
  }
  if(system_cmd(cmd) != 0) { return false; }
  return true;
}

bool
configuration::delete_route(std::string prefix, uint32_t mask)
{
  std::string cmd = "sudo /usr/local/bin/onl_delroute.sh " + prefix + " " + int2str(mask);
  //JP: 3_22_12 changed to launch subtype daemon as user 
  //std::string cmd = "/sbin/route del -net " + prefix + "/" + int2str(mask);
  if(system_cmd(cmd) != 0) { return false; }
  return true;
}

std::string
configuration::addr_int2str(uint32_t addr)
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

int
configuration::system_cmd(std::string cmd)
{
  write_log("configuration::system_cmd(): cmd = " + cmd);
  int rtn = system(cmd.c_str());
  if(rtn == -1) return rtn;
  return WEXITSTATUS(rtn);
}
