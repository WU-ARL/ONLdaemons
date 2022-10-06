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
#include <fstream>
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
//#include <netlink/route/tc.h>
#include <netlink/route/class.h>
#include <netlink/route/qdisc/netem.h>
#include <netlink/route/qdisc/htb.h>

//#include <boost/shared_ptr.hpp>

#include "shared.h"

//#include "swrd_types.h"
#include "ndne_configuration.h"
#include "ndne_requests.h"
//#define START_PRIORITY 255
#define IFACE_DEFAULT_MIN 1000

using namespace ndne;

HostConfiguration::HostConfiguration() throw(configuration_exception)
{ 
  // struct ifreq ifr;
  // char addr_str[INET_ADDRSTRLEN];
  // struct sockaddr_in sa,*temp;

  // //system_cmd("rm -f /tmp/swr_fstats*");

  // int temp_sock;
  // int temp_port=5555;
   next_mark = 10;
   start_priority = 255;
   delay_index = max_port + 2;
  // router_type = rtr_type;
  // if (rtr_type == SWR_2P_10G)
  //   virtual_ports = true;
  // else
  //   virtual_ports = false;

  // // get my ip addr
  // if((temp_sock = socket(PF_INET,SOCK_DGRAM,0)) < 0)
  // {
  //   throw configuration_exception("socket failed");
  // }
  // strcpy(ifr.ifr_name, "control");
  // if(ioctl(temp_sock, SIOCGIFADDR, &ifr) < 0)
  // {
  //   throw configuration_exception("ioctl failed");
  // }
  // close(temp_sock);
  // sa.sin_family = AF_INET;
  // sa.sin_port = htons(temp_port);
  // temp = (struct sockaddr_in *)&ifr.ifr_addr;
  // sa.sin_addr.s_addr = temp->sin_addr.s_addr;
  // control_address = (unsigned int) sa.sin_addr.s_addr;
  // if(inet_ntop(AF_INET,&sa.sin_addr,addr_str,INET_ADDRSTRLEN) == NULL)
  // {
  //   throw configuration_exception("inet_ntop failed");
  // }
  // // done getting ip addr

  // last initialize all the locks used here
  pthread_mutex_init((&conf_lock), NULL);
  pthread_mutex_init((&iptable_lock), NULL);

  state = STOP;

  //numConfiguredPorts = 0;
  for (unsigned int i=0; i < max_port; i++)
  {
    portTable[i].configured = false;
    portTable[i].nic = "data" + int2str(i);
  }

  username = "";

}

HostConfiguration::~HostConfiguration() throw()
{
  // if(state == START)
  // {
  //   stop();
  // }

  state = STOP;
  pthread_mutex_destroy(&conf_lock);

}

// start(): Do all the initial setup stuff before we start receiving RLI messages
//                 Turn on ip forwarding
//                 Initialize routes and ip_tables so that we do NOT route to or from control interface.
 void HostConfiguration::start() throw(configuration_exception)
 {
   autoLock glock(conf_lock);

   if(state == START)
  {
    write_log("start called, but the router has already been started..");
    glock.unlock();
    return;
  }

  write_log("HostConfiguration::start: entering");

  //FOR NOW: I don't believe we want a main route table
  
  //   int num_ports = router_type;
  //   char shcmd[256];

 
  //sets up rules for route tables
  // sprintf(shcmd, "/usr/local/bin/swrd_iprules.sh");
  // write_log("HostConfiguration::start: adding iprules-system(" + std::string(shcmd) + ")");
  // if (system_cmd(shcmd) < 0)
  //   {
  //     write_log("HostConfiguration::start: iprules script failed");
  //     throw configuration_exception("iprules failed");
  //   }
  
  
  /* the router is all ready to go now */
  state = START;
  
  glock.unlock();

  return;
}

// // stop(): Turn off ip forwarding
 void HostConfiguration::stop() throw()
 {

   autoLock glock(conf_lock);

  if(state == STOP)
  { 
    write_log("stop called, but the host is already stopped..");
    return;
  }

  state = STOP;

  write_log("stop: done");

  return;
}

unsigned int HostConfiguration::started() throw()
{
  autoLock glock(conf_lock);
  if(state == START) return 1;
  return 0;
}

void
HostConfiguration::configure_port(unsigned int port, std::string ip, std::string mask, uint32_t rate, std::string nhip) throw(configuration_exception)
{
  std::string logstr = "configuration::set_port_info(): port=" + int2str(port);
  write_log(logstr);
  char shcmd[256];

  if(port > max_port) return;

  if (!portTable[port].configured)
    {
      portTable[port].configured = true;
    
      
      portTable[port].ip_addr = ip;
      portTable[port].next_hop = nhip;
      portTable[port].netmask = mask;
      int mark = get_next_mark();
      portTable[port].mark = mark;
      portTable[port].rate = rate;
      portTable[port].max_rate = rate;
      
//      if (portTable[port].next_hop == "0.0.0.0")
//	{
//	  system_cmd("sudo /usr/local/bin/onl_cfgport.sh " + portTable[port].nic + " " + portTable[port].ip_addr + " 255.255.255.0");
//	}
 //     else
//	{
	  sprintf(shcmd, "/etc/ONL/bin/subtypes/onl_ndne_cfgport.sh %d %s %s 255.255.255.0 %d %d %d", port, portTable[port].nic.c_str(), ip.c_str(), rate, IFACE_DEFAULT_MIN, mark);
	  if (system_cmd(shcmd) != 0)
	    {
	      throw configuration_exception("HostConfiguration::configure_port system onl_ndne_cfgport failed");
	    }

//	  if(portTable[port].next_hop != "0.0.0.0")
//	    {
//	      if (0 != system_cmd("sudo /usr/local/bin/onl_addroute.sh 192.168.0.0 255.255.0.0 " + portTable[port].nic))
//		{
//		  throw configuration_exception("HostConfiguration::configure_port system onl_addroute failed");
//		};
//	    }

	  //add default netem queue to be consistent with any added queues
	  int p1 = port + 1;
	  sprintf(shcmd, "tc qdisc add dev %s parent %d:1 handle %d:0 netem delay 0ms", portTable[port].nic.c_str(), p1, (delay_index + 1));
	  write_log("HostConfiguration::configure_port system(" + std::string(shcmd) + ")");
	  if(system_cmd(shcmd) != 0)
	    {
	      throw configuration_exception("HostConfiguration::configure_port system (tc qdisc add) failed");
	    }
//	}

      write_log("HostConfiguration::configure_node: port=" + int2str(port) + ", ipstr=" + ip + ", ip=" + portTable[port].ip_addr);
    }
}

bool
HostConfiguration::add_route(uint16_t port, std::string prefix, uint32_t mask, std::string nexthop)
{
  std::string cmd = "sudo /usr/local/bin/onl_addroute.sh " + prefix + " " + int2str(mask);
  //JP: 3_22_12 changed to launch subtype daemon as user 
  //std::string cmd = "/sbin/route add -net " + prefix + "/" + int2str(mask);
  if(nexthop == "0.0.0.0")
  {
    cmd += " dev " + portTable[port].nic;
  }
  else
  {
    cmd += " gw " + nexthop;
  }
  if(system_cmd(cmd) != 0) { return false; }
  return true;
}


bool
HostConfiguration::delete_route(std::string prefix, uint32_t mask)
{
  std::string cmd = "sudo /usr/local/bin/onl_delroute.sh " + prefix + " " + int2str(mask);
  //JP: 3_22_12 changed to launch subtype daemon as user 
  //std::string cmd = "/sbin/route del -net " + prefix + "/" + int2str(mask);
  if(system_cmd(cmd) != 0) { return false; }
  return true;
}

// rates should be in Kbps
unsigned int HostConfiguration::get_port_rate(unsigned int port) throw(configuration_exception)
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
void HostConfiguration::set_port_rate(unsigned int port, uint32_t rate) throw(configuration_exception)
{
  //struct nl_sock *my_sock = nl_socket_alloc();
  //struct nl_cache *link_cache, *all_classes;
  //struct rtnl_link *link_ptr;
  //struct rtnl_class *tcclass;
  std::string nic;
  char shcmd[256];
  write_log("set_port_rate:");

  if(port > max_port)
  {
    throw configuration_exception("invalid port");
  } 
  if(!portTable[port].configured)
  {
    throw configuration_exception("unconfigured port");
  }
  if (rate > portTable[port].max_rate)
  {
    throw configuration_exception("invalid link rate");
  }
  portTable[port].rate = rate;

  nic = portTable[port].nic;

  int p1 = port + 1;
  sprintf(shcmd, "tc class change dev %s parent root classid %d:0 htb rate %dkbit ceil %dkbit mtu 1500", nic.c_str(), p1, rate, rate); 
  write_log("HostConfiguration::set_port_rate: (" + std::string(shcmd) + ")");
  if (system_cmd(shcmd) != 0)
    {
      write_log("HostConfiguration::set_port_rate: tc class change failed");
      throw configuration_exception("tc class change failed");
    }
  sprintf(shcmd, "tc class change dev %s parent %d:0 classid %d:1 htb rate %dkbit ceil %dkbit mtu 1500", nic.c_str(), p1, p1, rate, rate); 
  write_log("HostConfiguration::set_port_rate for netem queue: (" + std::string(shcmd) + ")");
  if (system_cmd(shcmd) != 0)
    {
      write_log("HostConfiguration::set_port_rate: tc class change failed for netem queue");
      throw configuration_exception("tc class change failed");
    }
}

void HostConfiguration::add_filter(filter_ptr f) throw(configuration_exception)
{ 
  if (f->mark < 1) f->mark = get_next_mark();
  f->rule_priority = f->mark + start_priority;//get_next_priority();
  if (f->rule_priority < 0)
    {
      write_log("HostConfiguration::add_filter: rule priority maxed out");
      throw configuration_exception("add_filter:rule priority maxed out");
    }

  filter_command(f, false);
  
  autoLock cnflock(conf_lock);
  filters.push_back(f);
}

void HostConfiguration::del_filter(filter_ptr f) throw(configuration_exception)
{
  filter_ptr rtn_f = get_filter(f);
  if (rtn_f) 
    {
      filter_command(rtn_f, true);
      autoLock cnflock(conf_lock);
      filters.remove(rtn_f);
    }
}

filter_ptr HostConfiguration::get_filter(filter_ptr f)
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

filter_ptr HostConfiguration::get_filter(int port, int qid)
{
  filter_ptr empty_ptr;
  std::list<filter_ptr>::iterator fit;
  std::string port_str = int2str(port);
  for (fit = filters.begin(); fit != filters.end(); ++fit)
    {
      if ((*fit)->qid == qid &&
	  (*fit)->output_port == port_str)
	return (*fit);
    }
  return empty_ptr;
}

uint32_t HostConfiguration::filter_stats(filter_ptr f, bool ispkts) throw(configuration_exception)
{
  filter_ptr rtn_f = get_filter(f);
  //PROBLEM: do I need to lock for get_filter could an add or delete happen here
  std::string shcmd = "/etc/ONL/bin/subtypes/get_filter_stats.py -m " + int2str(rtn_f->mark);
  std::string filenm = "/tmp/swr_fstats_";
  if (ispkts) 
    {
      shcmd += " -p ";
      filenm += "pkts." + int2str(rtn_f->mark);
    }
  else 
    {
      shcmd += " -b ";
      filenm += "bytes." + int2str(rtn_f->mark);
    }
  shcmd += " > " + filenm;
  if (system_cmd(shcmd) < 0)
    {
      write_log("HostConfiguration::filter_stats failed:" + shcmd);
      throw configuration_exception("get_filter_stats failed");
    }
  std::ifstream ifs(filenm.c_str());

  int rtn = 0;
  std::string line;
  if (ifs.is_open())
    {
      getline(ifs, line);
      rtn = atoi(line.c_str());
      ifs.close();
    }
  else
    {
      write_log("HostConfiguration::filter_stats failed: file " + filenm + " didn't open");
      throw configuration_exception("get_filter_stats failed");
    }
  write_log("HostConfiguration:filter_stats for mark:" + int2str(rtn_f->mark) + " " + int2str(rtn));
  return (uint32_t)rtn;
}

void HostConfiguration::filter_command(filter_ptr f, bool isdel) throw(configuration_exception)
{
  //need to add 2 entries through iptables one in PREROUTING and one in FORWARD to make packets reach the right queue
  std::string cmdprefix1 = "iptables -t mangle";
  std::string cmdprefix2 = "iptables -t filter";
  std::string shcmd = "";
  std::string command_type = "add_filter";
  if (!isdel)
    {
      if ((f->dest_prefix.substr(0,3) != "192" && f->dest_prefix.substr(0,3) != "224" && f->dest_prefix != "*") ||
	  (f->src_prefix.substr(0,3) != "192" && f->src_prefix.substr(0,3) != "224" && f->src_prefix != "*"))
      {
	write_log("HostConfiguration::add_route: prefix does not start with 192 or 224");
	throw configuration_exception("add filter prefix needs to start with 192 or 224");
      }
      //cmdprefix1 += " -I PREROUTING";
      cmdprefix1 += " -I OUTPUT";
      //cmdprefix2 += " -A FORWARD";
      cmdprefix2 += " -A OUTPUT";
    }
  else
    {
      //cmdprefix1 += " -D PREROUTING";
      cmdprefix1 += " -D OUTPUT";
      //cmdprefix2 += " -D FORWARD";
      cmdprefix2 += " -D OUTPUT";
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
    shcmd += " -m statistic --mode nth --every " + int2str(f->sampling) + " --packet 0";
  
  shcmd += " -j MARK --set-mark " + int2str(f->mark);
  
  autoLock iptlock(iptable_lock);
  int rtn = system_cmd(cmdprefix1 + shcmd);
  iptlock.unlock();
  write_log("HostConfiguration::" + command_type + ": returned("+ int2str(rtn) + "):" + cmdprefix1 + shcmd);
  if (rtn != 0)
    {
      write_log("HostConfiguration::" + command_type + ": iptables1 trying again:" + cmdprefix1 + shcmd);
      sleep(1);
      iptlock.lock();
      rtn = system_cmd(cmdprefix1 + shcmd);
      iptlock.unlock();
      if (rtn != 0)
	{
	  write_log("HostConfiguration::" + command_type + ": iptables1 trying again2:" + cmdprefix1 + shcmd);
	  sleep(1);
	  iptlock.lock();
	  rtn = system_cmd(cmdprefix1 + shcmd);
	  iptlock.unlock();
	  if (rtn != 0)
	    {
	      write_log("HostConfiguration::" + command_type + ": iptables failed returned(" + int2str(rtn) + ") -- " + cmdprefix1);
	      throw configuration_exception("iptables1 failed");
	    }
	}
    }
  iptlock.lock();
  rtn = system_cmd(cmdprefix2 + shcmd);
  iptlock.unlock();
  write_log("HostConfiguration::" + command_type + ": returned("+ int2str(rtn) + "):" + cmdprefix2 + shcmd);
  if (rtn != 0)
    {
      write_log("HostConfiguration::" + command_type + ": iptables2 trying again:" + cmdprefix2 + shcmd);
      sleep(1);
      iptlock.lock();
      rtn = system_cmd(cmdprefix2 + shcmd);
      iptlock.unlock();
      if (rtn != 0)
	{
	  write_log("HostConfiguration::" + command_type + ": iptables2 trying again2:" + cmdprefix2 + shcmd);
	  sleep(1);
	  iptlock.lock();
	  rtn = system_cmd(cmdprefix2 + shcmd);
	  iptlock.unlock();
	  if (rtn != 0)
	    {
	      write_log("HostConfiguration::" + command_type + ": iptables failed returned(" + int2str(rtn) + ") -- " + cmdprefix2);
	      throw configuration_exception("iptables2 failed");
	    }
	}
    }
  
  //set up tc filter for qid if set
  int op = atoi(f->output_port.c_str());
  if (f->qid > 1 && portTable[op].configured)
    {
      std::string nic;
      std::string cmd_type = "add";
      if (isdel) cmd_type = "del";

      nic = portTable[op].nic;

      int p1 = op + 1;

      shcmd = "tc filter " + cmd_type + " dev " + nic + " parent " + int2str(p1) + ":0 protocol ip prio 2 handle " + 
	int2str(f->mark) + " fw flowid " + int2str(p1) + ":" + int2str(f->qid);
      write_log("HostConfiguration::" + command_type + ": " + shcmd);
      if (system_cmd(shcmd) < 0)
	{
	  write_log("HostConfiguration::" + command_type + ": tc filter " + cmd_type + " failed");
	  throw configuration_exception("tc filter " + cmd_type + " failed");
	}
    }
  //set ip rule
  if (!isdel)
    shcmd = "ip rule add";
  else shcmd = "ip rule del";
  if (f->unicast_drop) shcmd += " blackhole";
  shcmd += " fwmark " + int2str(f->mark);
  if (!f->unicast_drop)
    {
      f->table = "port" + f->output_port + "Filters";
      shcmd += " table " + f->table;
    }

  shcmd += " priority " + int2str(f->rule_priority);
  
  
  rtn = system_cmd(shcmd);
  write_log("HostConfiguration::" + command_type + ": returned("+ int2str(rtn) + ":" + shcmd);
  if (rtn < 0)
    {
      write_log("HostConfiguration::" + command_type + ": ip rule failed");
      throw configuration_exception("ip rule failed");
    }
}
/*
unsigned int HostConfiguration::get_port_addr(unsigned int port) throw()
{
  if(port > 4)
  {
    return 0;
  }
  return (portTable[port].ip_addr);
}

unsigned int HostConfiguration::get_next_hop_addr(unsigned int port) throw()
{
  if(port > 4)
  {
    return 0;
  }
  return (portTable[port].next_hop);
}
*/ 

void HostConfiguration::add_queue(uint16_t port, uint32_t qid, uint32_t rate, uint32_t burst, uint32_t ceil_rate, uint32_t cburst, uint32_t mtu, bool change) throw(configuration_exception)
{
  std::string nic;
  std::string shcmd;
  std::string cmd_type = "add";
  if (change) cmd_type = "change";

  if (!portTable[port].configured) 
    throw configuration_exception("port not configured");

  nic = portTable[port].nic;

  int p1 = port + 1;

  shcmd = "tc class " + cmd_type + " dev " + nic + " parent " + int2str(p1) + ":0 classid " + int2str(p1) + ":"
    + int2str(qid) + " htb rate ";

  if (rate == 0 && ceil_rate > 0) shcmd += int2str(ceil_rate) + "kbit ceil ";
  else shcmd += int2str(rate) + "kbit ceil ";

  if (ceil_rate > 0) 
    shcmd += int2str(ceil_rate) + "kbit";
  else 
    shcmd += int2str(rate) + "kbit";

  if (burst > 0) shcmd += " burst " + int2str(burst);
  if (cburst > 0) shcmd += " cburst " + int2str(cburst);
  shcmd += " prio 2";
  if (mtu > 0) shcmd += " mtu " + int2str(mtu);
  else shcmd += " mtu 1500";
  write_log("HostConfiguration::" + cmd_type +"_queue: (" + std::string(shcmd) + ")");
  if (system_cmd(shcmd) != 0)
    {
      write_log("HostConfiguration::" + cmd_type + "_queue: tc class " + cmd_type + " failed");
      throw configuration_exception("tc class " + cmd_type + " failed");
    }
  //need to add qdisc so can get qlength think sfq
  if (!change)
    {
      int newq_handle = qid + delay_index;
      //shcmd = "tc qdisc " + cmd_type + " dev " + nic + " parent " + int2str(p1) + ":" + int2str(qid) + " handle " + int2str(newq_handle) + ":0 sfq perturb 10" ;
      shcmd = "tc qdisc " + cmd_type + " dev " + nic + " parent " + int2str(p1) + ":" + int2str(qid) + " handle " + int2str(newq_handle) + ":0 netem delay 0ms" ;
      write_log("HostConfiguration::" + cmd_type +"_queue: (" + std::string(shcmd) + ")");
      if (system_cmd(shcmd) != 0)
	{
	  write_log("HostConfiguration::" + cmd_type + "_queue: tc qdisc " + cmd_type + " failed");
	  throw configuration_exception("tc qdisc " + cmd_type + " failed");
	}
    }
}

void HostConfiguration::delete_queue(uint16_t port, uint32_t qid) throw(configuration_exception)
{
  std::string nic;
  std::string shcmd;

  if (qid > 1)
    {
      filter_ptr fptr = get_filter(port, qid);
      if (fptr)
  	{
  	  throw configuration_exception("filter attached to this queue. disable filter first.");
  	}
    }

  if (!portTable[port].configured) 
    throw configuration_exception("port not configured");

  nic = portTable[port].nic;

  int p1 = port + 1;

  shcmd = "tc class del dev " + nic + " parent " + int2str(p1) + ":0 classid " + int2str(p1) + ":"
    + int2str(qid);

  write_log("HostConfiguration::delete_queue: (" + std::string(shcmd) + ")");

  if (system_cmd(shcmd) != 0)
    {
      write_log("HostConfiguration::delete_queue: tc class delete failed");
      throw configuration_exception("tc class delete failed");
    }
}

//Add delay on port for outgoing traffic
void HostConfiguration::add_netem_queue(uint16_t port, uint32_t qid, uint32_t dtime, uint32_t jitter, uint32_t loss, uint32_t corrupt, uint32_t duplicate, bool change) throw(configuration_exception)
{
  std::string nic;
  std::string shcmd;
  std::string cmd_type = "add";
  if (change) cmd_type = "change";

  if (!portTable[port].configured) 
    throw configuration_exception("port not configured");

  nic = portTable[port].nic;

  int p1 = port + 1;
  int newq_handle = qid + delay_index;


  //first delete any existing q
  shcmd = "tc qdisc del dev " + nic + " parent " + int2str(p1) + ":" + int2str(qid) + " handle " + int2str(newq_handle) + ":0" ;
  write_log("HostConfiguration::" + cmd_type + "_netem_queue: (" + std::string(shcmd) + ")");
  system_cmd(shcmd);//don't care if this fails

  shcmd = "tc qdisc " + cmd_type + " dev " + nic + " parent " + int2str(p1) + ":" + int2str(qid) + " handle " + int2str(newq_handle) + ":0 netem" ;

  if (dtime > 0)
    {
      shcmd += " delay " + int2str(dtime) + "ms";
      if (jitter > 0) shcmd += " " + int2str(jitter);
    }
  else if (change) 
    shcmd += " delay 0ms";

  if (loss > 0) shcmd += " loss " + int2str(loss);
  else if (change) 
    shcmd += " loss 0";
  if (corrupt > 0) shcmd += " corrupt " + int2str(corrupt);
  else if (change) 
    shcmd += " corrupt 0";
  if (duplicate > 0) shcmd += " duplicate " + int2str(duplicate);
  else if (change) 
    shcmd += " duplicate 0";

  write_log("HostConfiguration::" + cmd_type + "_netem_queue: (" + std::string(shcmd) + ")");
  if (system_cmd(shcmd) != 0)
    {
      write_log("HostConfiguration::" + cmd_type + "_netem_queue: tc qdisc " + cmd_type + " failed");
      throw configuration_exception("tc qdisc " + cmd_type + " failed");
    }
}

void HostConfiguration::delete_netem_queue(uint16_t port, uint32_t qid) throw(configuration_exception) 
{
  std::string nic;
  std::string shcmd;

  if (!portTable[port].configured) 
    throw configuration_exception("port not configured");

  nic = portTable[port].nic;

  int p1 = port + 1;
  int newq_handle = qid + delay_index;
  shcmd = "tc qdisc del dev " + nic + " parent " + int2str(p1) + ":" + int2str(qid) + " handle " + int2str(newq_handle) + ":0";// netem" ;

  write_log("HostConfiguration::delete_netem_queue: (" + std::string(shcmd) + ")");
  if (system_cmd(shcmd) != 0)
    {
      write_log("HostConfiguration::delete_netem_queue: tc qdisc delete failed");
      throw configuration_exception("tc qdisc delete failed");
    }

  //need to add back in another queue for monitoring purposes
  //shcmd = "tc qdisc add dev " + nic + " parent " + int2str(p1) + ":" + int2str(qid) + " handle " + int2str(newq_handle) + ":0 sfq perturb 10" ;
  shcmd = "tc qdisc add dev " + nic + " parent " + int2str(p1) + ":" + int2str(qid) + " handle " + int2str(newq_handle) + ":0 netem delay 0ms" ;
  write_log("HostConfiguration::delete_netem_queue: (" + std::string(shcmd) + ")");
  if (system_cmd(shcmd) != 0)
    {
      write_log("HostConfiguration::delete_netem_queue: tc qdisc add sfq failed");
      //throw configuration_exception("tc qdisc " + cmd_type + " failed");//don't throw and error this will just cause monitoring to fail
    }
}

//Add delay on port for outgoing traffic
void HostConfiguration::add_delay_port(uint16_t port, uint32_t dtime, uint32_t jitter) throw(configuration_exception)
{
  std::string nic;
  char shcmd[256];

  if (!portTable[port].configured) 
    throw configuration_exception("port not configured");

  nic = portTable[port].nic;

  int p1 = port + 1;
  int delay_handle = 1 + delay_index;
  sprintf(shcmd, "tc qdisc add dev %s parent %d:1 handle %d:0 netem delay %dms %d", nic.c_str(), p1, delay_handle, dtime, jitter); 
  write_log("HostConfiguration::add_delay_port: (" + std::string(shcmd) + ")");
  if (system_cmd(shcmd) != 0)
    {
      write_log("HostConfiguration::add_delay_port: tc qdisc add failed trying change");
      sprintf(shcmd, "tc qdisc change dev %s parent %d:1 handle %d:0 netem delay %dms %d", nic.c_str(), p1, delay_handle, dtime, jitter); 
      write_log("HostConfiguration::add_delay_port try 2: (" + std::string(shcmd) + ")");
      if (system_cmd(shcmd) != 0)
	{
	  write_log("HostConfiguration::add_delay_port: tc qdisc change failed");
	  throw configuration_exception("tc qdisc change failed");
	}
    }
}

//Delete delay on port for outgoing traffic
void HostConfiguration::delete_delay_port(uint16_t port) throw(configuration_exception)
{
  struct nl_sock *my_sock = nl_socket_alloc();
  struct nl_cache *link_cache, *all_qdiscs;
  struct rtnl_link *link_ptr;
  struct rtnl_qdisc *qdisc;
  std::string nic;

  if (!portTable[port].configured) 
    throw configuration_exception("port not configured");

  nl_connect(my_sock, NETLINK_ROUTE);
  if (rtnl_link_alloc_cache(my_sock, AF_UNSPEC, &link_cache) < 0)
    {
      nl_socket_free(my_sock);
      throw configuration_exception("rtnl_link_alloc_cache failed");
    }

  nic = portTable[port].nic;

  if (!(link_ptr = rtnl_link_get_by_name(link_cache, nic.c_str())))
    {
      nl_socket_free(my_sock);
      nl_cache_free(link_cache);
      throw configuration_exception("rtnl_link_get_by_name failed");
    }

  if (rtnl_qdisc_alloc_cache(my_sock, &all_qdiscs) < 0)
    {
      nl_socket_free(my_sock);
      nl_cache_free(link_cache);
      throw configuration_exception("rtnl_qdisc_alloc_cache failed");
    }

  int qlen = nl_cache_nitems(all_qdiscs);
  int ifindex = rtnl_link_get_ifindex(link_ptr); 
  
  if (!(qdisc = rtnl_qdisc_get_by_parent(all_qdiscs, ifindex, TC_HANDLE((port + 1),1))))
    {
      nl_socket_free(my_sock);
      nl_cache_free(link_cache);
      nl_cache_free(all_qdiscs);
      throw configuration_exception("rtnl_qdisc_get failed");
    }

  if (rtnl_qdisc_delete(my_sock, qdisc) != 0)
    {
      nl_socket_free(my_sock);
      nl_cache_free(link_cache);
      nl_cache_free(all_qdiscs);
      throw configuration_exception("rtnl_qdisc_delete failed");
    }

  nl_socket_free(my_sock);
  nl_cache_free(link_cache);
  nl_cache_free(all_qdiscs);
}

void HostConfiguration::set_username(std::string un) throw()
{
  username = un;
  write_log("set_username: got user name " + username);
}

std::string HostConfiguration::get_username() throw()
{
  return username;
}

std::string HostConfiguration::addr_int2str(uint32_t addr)
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
HostConfiguration::read_stats_pkts(int port, int qid, bool use_class) throw(monitor_exception)
{
  uint64_t pkts;
  bool retry = false;
  try
    {
      if (!use_class) pkts = read_stats(port, RTNL_TC_PACKETS, qid);
      else pkts = read_stats_class(port, RTNL_TC_PACKETS, qid);
    }
  catch(std::exception& e)
    {
      std::string msg = e.what();
      retry = true;
      write_log("HostConfiguration::read_stats_pkts: retry got exception:" + msg);
    }
  if (retry)
    {
      sleep(3);
      if (!use_class) pkts = read_stats(port, RTNL_TC_PACKETS, qid);
      else pkts = read_stats_class(port, RTNL_TC_PACKETS, qid);
    }
  //write_log("HostConfiguration::read_stats_pkts for port(" + int2str(port) + ") packets(" + int2str(qid) + "): " + int2str(pkts));
  return pkts;
}

uint64_t 
HostConfiguration::read_stats_bytes(int port, int qid, bool use_class) throw(monitor_exception)
{  
  uint64_t bytes;  
  bool retry = false;
  try
    {
      if (!use_class) bytes = read_stats(port, RTNL_TC_BYTES, qid);
      else bytes = read_stats_class(port, RTNL_TC_BYTES, qid);
    }
  catch(std::exception& e)
    {
      std::string msg = e.what();
      retry = true;
      write_log("HostConfiguration::read_stats_bytes: retry got exception:" + msg);
    }
  if (retry)
    {
      sleep(3);
      if (!use_class) bytes = read_stats(port, RTNL_TC_BYTES, qid);
      else bytes = read_stats_class(port, RTNL_TC_BYTES, qid);
    }
  //write_log("HostConfiguration::read_stats_bytes for port(" + int2str(port) + ") bytes(" + int2str(qid) + "): " + int2str(bytes));
  return bytes;
}

uint64_t 
HostConfiguration::read_stats_qlength(int port, int qid, bool use_class) throw(monitor_exception)
{  
  uint64_t rtn; 
  bool retry = false;
  try
    {
      if (!use_class) rtn = read_stats(port, RTNL_TC_QLEN, qid);
      else rtn = read_stats_class(port, RTNL_TC_QLEN, qid);
      //uint64_t rtn = read_stats(port, RTNL_TC_BACKLOG, qid);
    }
  catch(std::exception& e)
    {
      std::string msg = e.what();
      retry = true;
      write_log("HostConfiguration::read_stats_qlength: retry got exception:" + msg);
    }
  if (retry)
    {
      sleep(3);
      if (!use_class) rtn = read_stats(port, RTNL_TC_QLEN, qid);
      else rtn = read_stats_class(port, RTNL_TC_QLEN, qid);
    }
  //write_log("HostConfiguration::read_stats_qlength for port(" + int2str(port) + ") qlength(" + int2str(qid) + "): " + int2str(rtn));
  return rtn;
}

uint64_t 
HostConfiguration::read_stats_drops(int port, int qid, bool use_class) throw(monitor_exception)
{  
  uint64_t rtn;
  bool retry = false;
  try
    {
      if (!use_class) rtn = read_stats(port, RTNL_TC_DROPS, qid); 
      else rtn = read_stats_class(port, RTNL_TC_DROPS, qid);
    }
  catch(std::exception& e)
    {
      std::string msg = e.what();
      retry = true;
      write_log("HostConfiguration::read_stats_drops: retry got exception:" + msg);
    }
  if (retry)
    {
      sleep(3);
      if (!use_class) rtn = read_stats(port, RTNL_TC_DROPS, qid); 
      else rtn = read_stats_class(port, RTNL_TC_DROPS, qid);
    }
  //write_log("HostConfiguration::read_stats_drops for port(" + int2str(port) + ") drops(" + int2str(qid) + "): " + int2str(rtn));
  return rtn;
}

uint64_t 
HostConfiguration::read_stats_backlog(int port, int qid, bool use_class) throw(monitor_exception)
{  
  uint64_t rtn = 0;
  if (!use_class) rtn = read_stats(port, RTNL_TC_BACKLOG, qid);
  else rtn = read_stats_class(port, RTNL_TC_BACKLOG, qid);
  //write_log("HostConfiguration::read_stats_backlog for port(" + int2str(port) + ") backlog(" + int2str(qid) + "): " + int2str(rtn));
  return rtn;
}

uint64_t 
HostConfiguration::read_stats(int port, enum rtnl_tc_stat sid, int qid) throw(monitor_exception) //enum rtnl_tc_stat
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

  nic = portTable[port].nic;
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

  if (qid == 0)
    {
      
      if (!(qdisc = rtnl_qdisc_get(all_qdiscs, ifindex, TC_HANDLE((port + 1),0))))
	{
	  //sleep and retry
	  //write_log("rtnl_qdisc_get failed: sleep and retry");
	  //sleep(5);
	  //if (!(qdisc = rtnl_qdisc_get(all_qdiscs, ifindex, TC_HANDLE((port + 1),0))))
	  //{
	      write_log("rtnl_qdisc_get failed");
	      nl_socket_free(my_sock);
	      nl_cache_free(link_cache);
	      nl_cache_free(all_qdiscs);
	      throw monitor_exception("rtnl_qdisc_get failed");
	      //}
	}
    }
  else
    {
	/* Iterate on all_qdiscs cache */
      int tmp_p = port + 1;
      int tmp_q = qid;
      if (qid > 9) tmp_q = qid + 6; //this is to work around a bug in libnl 4/21/2020 this may change again
      if (port > 8) tmp_p = (port + 7);//this is to work around a bug in libnl 7/12/17 this may change again
      
      uint32_t tc_handle = TC_HANDLE(tmp_p,tmp_q);
      
      if (!(qdisc  = rtnl_qdisc_get_by_parent(all_qdiscs, ifindex, tc_handle)))
	//if (!(qdisc  = rtnl_qdisc_get(all_qdiscs, ifindex, TC_HANDLE((port + 1),qid))))
	{
	  //sleep and retry
	  //write_log("rtnl_qdisc_get_by_parent failed: sleep and retry");
	  //sleep(5);
	  //if (!(qdisc  = rtnl_qdisc_get_by_parent(all_qdiscs, ifindex, TC_HANDLE((port + 1),qid))))
	  //{
	  qdisc = (struct rtnl_qdisc *)nl_cache_get_first(all_qdiscs);
	  write_log("rtnl_qdisc_get_by_parent failed");
	  write_log("HostConfiguration::read_stats qdiscs for port " + int2str(port) + " looking for parent:" + int2str(tc_handle) + " ifindex:" + int2str(ifindex));
	  while ((qdisc != NULL)) {
	    std::string msg = "qdisc: ";
	    uint32_t tmp_param = rtnl_tc_get_handle(TC_CAST(qdisc));
	    msg = msg + "handle(" + int2str(tmp_param) + ") ";
	    tmp_param = rtnl_tc_get_parent(TC_CAST(qdisc));
	    msg = msg + "parent(" + int2str(tmp_param) + ") ";
	    tmp_param = rtnl_tc_get_ifindex(TC_CAST(qdisc));
	    msg = msg + "ifindex(" + int2str(tmp_param) + ") ";
	    rtn_stat = rtnl_tc_get_stat(TC_CAST(qdisc), sid);
	    msg = msg + "stat(" + int2str(rtn_stat) + ") ";
	    write_log(msg);
	    /* Next link */
	    qdisc = (struct rtnl_qdisc *)nl_cache_get_next((struct nl_object *)qdisc);
	  }
	  nl_socket_free(my_sock);
	  nl_cache_free(link_cache);
	  nl_cache_free(all_qdiscs);
	  throw monitor_exception("rtnl_qdisc_get_by_parent failed");
	  //}
	}
    }

  rtn_stat = rtnl_tc_get_stat(TC_CAST(qdisc), sid);
  rtnl_qdisc_put(qdisc);
  nl_socket_free(my_sock);
  nl_cache_free(link_cache);
  nl_cache_free(all_qdiscs);
  return rtn_stat;
}

uint64_t 
HostConfiguration::read_stats_class(int port, enum rtnl_tc_stat sid, int qid) throw(monitor_exception) //enum rtnl_tc_stat
{  
  struct nl_sock *my_sock = nl_socket_alloc();
  struct nl_cache *link_cache, *all_classes;
  struct rtnl_link *link_ptr;
  struct rtnl_class *tcclass;
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

  nic = portTable[port].nic;
  if (!(link_ptr = rtnl_link_get_by_name(link_cache, nic.c_str())))
    {
      nl_socket_free(my_sock);
      nl_cache_free(link_cache);
      throw monitor_exception("rtnl_link_get_by_name failed");
    }

  int ifindex = rtnl_link_get_ifindex(link_ptr);

  if ((rtn = rtnl_class_alloc_cache(my_sock, ifindex, &all_classes)) < 0)
    {
      nl_socket_free(my_sock);
      nl_cache_free(link_cache);
      throw monitor_exception("rtnl_class_alloc_cache failed ");
    }

  int tmp_p = port + 1;
  int tmp_q = qid;
  if (qid > 9) tmp_q = qid + 6; //this is to work around a bug in libnl 4/21/2020 this may change again
  if (port > 8) tmp_p = (port + 7);//this is to work around a bug in libnl 7/12/17 this may change again
  //if (!(tcclass = rtnl_class_get(all_classes, ifindex, TC_HANDLE((port + 1),qid))))
  if (!(tcclass = rtnl_class_get(all_classes, ifindex, TC_HANDLE(tmp_p, tmp_q))))
    {
      nl_socket_free(my_sock);
      nl_cache_free(link_cache);
      nl_cache_free(all_classes);
      throw monitor_exception("rtnl_class_get failed");
    }
  
  rtn_stat = rtnl_tc_get_stat(TC_CAST(tcclass), sid);
  nl_socket_free(my_sock);
  nl_cache_free(link_cache);
  nl_cache_free(all_classes);
  return rtn_stat;
}


uint64_t 
HostConfiguration::read_link_stats(int port, rtnl_link_stat_id_t sid) throw(monitor_exception) //enum rtnl_tc_stat
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

  nic = portTable[port].nic;

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
HostConfiguration::read_link_stats_rxpkts(int port) throw(monitor_exception)
{
  uint64_t pkts = read_link_stats(port, RTNL_LINK_RX_PACKETS);
  //write_log("HostConfiguration::read_link_stats_rxpkts for port(" + int2str(port) + ") packets: " + int2str(pkts));
  return pkts;
}


uint64_t 
HostConfiguration::read_link_stats_txpkts(int port) throw(monitor_exception)
{
  uint64_t pkts = read_link_stats(port, RTNL_LINK_TX_PACKETS);
  //write_log("HostConfiguration::read_link_stats_txpkts for port(" + int2str(port) + ") packets: " + int2str(pkts));
  return pkts;
}


uint64_t 
HostConfiguration::read_link_stats_rxbytes(int port) throw(monitor_exception)
{
  uint64_t bytes = read_link_stats(port, RTNL_LINK_RX_BYTES);
  //write_log("HostConfiguration::read_link_stats_rxbytes for port(" + int2str(port) + ") bytes: " + int2str(bytes));
  return bytes;
}


uint64_t 
HostConfiguration::read_link_stats_txbytes(int port) throw(monitor_exception)
{
  uint64_t bytes = read_link_stats(port, RTNL_LINK_TX_BYTES);
  //write_log("HostConfiguration::read_link_stats_txbytes for port(" + int2str(port) + ") bytes: " + int2str(bytes));
  return bytes;
}

uint64_t 
HostConfiguration::read_link_stats_rxerrors(int port) throw(monitor_exception)
{
  uint64_t pkts = read_link_stats(port, RTNL_LINK_RX_ERRORS);
  //write_log("HostConfiguration::read_link_stats_rxerrors for port(" + int2str(port) + ") packets: " + int2str(pkts));
  return pkts;
}

uint64_t 
HostConfiguration::read_link_stats_txerrors(int port) throw(monitor_exception)
{
  uint64_t pkts = read_link_stats(port, RTNL_LINK_TX_ERRORS);
  //write_log("HostConfiguration::read_link_stats_txerrors for port(" + int2str(port) + ") packets: " + int2str(pkts));
  return pkts;
}

uint64_t 
HostConfiguration::read_link_stats_rxdrops(int port) throw(monitor_exception)
{
  uint64_t pkts = read_link_stats(port, RTNL_LINK_RX_DROPPED);
  //write_log("HostConfiguration::read_link_stats_rxdrops for port(" + int2str(port) + ") packets: " + int2str(pkts));
  return pkts;
}

uint64_t 
HostConfiguration::read_link_stats_txdrops(int port) throw(monitor_exception)
{
  uint64_t pkts = read_link_stats(port, RTNL_LINK_TX_DROPPED);
  //write_log("HostConfiguration::read_link_stats_txdrops for port(" + int2str(port) + ") packets: " + int2str(pkts));
  return pkts;
}

int
HostConfiguration::get_next_mark()
{
  autoLock cfglock(conf_lock);
  return next_mark++;
}

// int 
// HostConfiguration::get_next_priority()
// {
//   if (next_priority >= 32700) return -1;
//   else return next_priority++; 
// }



int
HostConfiguration::system_cmd(std::string cmd)
{
  int rtn = system(cmd.c_str());
  if(rtn != -1)
    rtn = WEXITSTATUS(rtn);
  write_log("HostConfiguration::system_cmd(" + int2str(rtn) + "): cmd = " + cmd);
  return rtn;
}
