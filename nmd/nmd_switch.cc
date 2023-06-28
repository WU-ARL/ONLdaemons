//
// Copyright (c) 2009-2013 Mart Haitjema, Charlie Wiseman, Jyoti Parwatikar, John DeHart 
// and Washington University in St. Louis
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
//    limitations under the License.
//
//

#include <boost/shared_ptr.hpp>
#include "nmd_includes.h"
#include <math.h>
#include <sys/wait.h>
#include <stdlib.h>

#define user "admin"
#define authentication "MD5"
#define password "netgearSwPW"
#define seclevel "authNoPriv"

bool set_switch_vlan_membership_arista64(port_list switch_ports, string switch_id, switch_vlan vlan_id, ostringstream& cmd);
bool set_switch_vlan_membership_arista(port_list switch_ports, string switch_id, switch_vlan vlan_id, ostringstream& cmd);
bool set_switch_vlan_membership_cisco(port_list switch_ports, string switch_id, switch_vlan vlan_id, ostringstream& cmd);
bool set_switch_vlan_membership_snmp(port_list switch_ports, string switch_id, switch_vlan vlan_id);
bool set_switch_pvids_arista64(port_list host_ports, string switch_id, switch_vlan vlan_id, ostringstream& cmd);
bool set_switch_pvids_arista(port_list host_ports, string switch_id, switch_vlan vlan_id, ostringstream& cmd);
bool set_switch_pvids_cisco(port_list host_ports, string switch_id, switch_vlan vlan_id, ostringstream& cmd);
bool set_switch_pvids_snmp(port_list host_ports, string switch_id, switch_vlan vlan_id);
bool initialize_switch_arista64(string switch_id);
bool initialize_switch_arista(string switch_id);
bool initialize_switch_cisco(string switch_id);
bool initialize_switch_snmp(string switch_id);
bool is_arista64_switch(string switch_id);
bool is_arista_switch(string switch_id);
bool is_cisco_switch(string switch_id);
bool exec_snmp(string cmd);
bool exec_ssh(string cmd);
void getHexString(unsigned char bval, char* str);
string encodeEgressPorts(port_list ports, int numBytes, bool is_complement);
string get_arista64_port(int port);

static std::map<std::string, switch_ssh_lock> ssh_locks;



switch_ssh_lock::switch_ssh_lock()
{
  switch_id = "";
  outstanding = 0; 
  // initialize lock
  pthread_mutex_init(&lock, NULL);
}


switch_ssh_lock::switch_ssh_lock(string sid)
{
  switch_id = sid;
  outstanding = 0; 
  // initialize lock
  pthread_mutex_init(&lock, NULL);
  ports_used = 0;
}


switch_ssh_lock::~switch_ssh_lock()
{
}

bool
switch_ssh_lock::start_ssh()
{
  port_list ps;
  return (start_ssh(ps));
}

bool
switch_ssh_lock::start_ssh(port_list ports)
{
  //uint64_t port_map = get_port_map(ports);
  autoLock slock(lock);
  write_log("switch_ssh_lock::start_ssh switch " + switch_id + " outstanding " + int2str(outstanding));
  if (outstanding < 10)// && ((port_map & ports_used) == 0))
    {
      //ports_used = ports_used | port_map;
      outstanding += 1;
      return true;
    }
  slock.unlock();
  for (int i = 0; i < 300; ++i)
    { 
      slock.lock();
      if (outstanding < 10 )//&& ((port_map & ports_used) == 0))
	{
	  //ports_used = ports_used | port_map;
	  outstanding += 1;
	  return true;
	}
      slock.unlock();
      sleep(1);
    }
  //write_log("ERROR: " + switch_id + " timed out trying to set switch ssh lock");
  return false;
}

void
switch_ssh_lock::finish_ssh()
{
  port_list ps;
  return (finish_ssh(ps));
}

void
switch_ssh_lock::finish_ssh(port_list ports)
{
  //uint64_t port_map = get_port_map(ports);
  autoLock slock(lock);
  outstanding -= 1;
  //ports_used = (ports_used & ~port_map);
  write_log("switch_ssh_lock::finish_ssh switch " + switch_id + " outstanding " + int2str(outstanding));
  if (outstanding < 0) 
    {
      write_log("WARNING: " + switch_id + " outstanding counter is negative");
      outstanding = 0;
    }
}


uint64_t
switch_ssh_lock::get_port_map(port_list ports)
{
  uint64_t rtn = 0;
  for (port_iter iter = ports.begin();
       iter != ports.end(); iter++) {
    rtn = (rtn | (1 << (iter->getPortNum())));
  }
  return rtn;
}


void start_command(string switch_id, ostringstream& cmd)
{
  if (is_arista_switch(switch_id) || is_arista64_switch(switch_id)) {
      cmd << "ssh -n " << user << "@" << switch_id << " \"enable" << endl;
      cmd << "configure" << endl;
     }
  else if (is_cisco_switch(switch_id)) {
      cmd << "ssh -i /root/.ssh/cisco_id_rsa " << user << "@" << switch_id << " \"configure ; " << endl;
  }
}


void between_command(string switch_id, ostringstream& cmd)
{
  //if (is_cisco_switch(switch_id)) {
  //  cmd << " ; " ;
  //}
}


bool send_command(string switch_id, ostringstream& cmd)
{
  if (is_arista_switch(switch_id) || is_arista64_switch(switch_id) || is_cisco_switch(switch_id)) {
      cmd << "\"" << endl;
      ssh_locks[switch_id].start_ssh();
      bool rtn =  exec_ssh(cmd.str());
      ssh_locks[switch_id].finish_ssh();
      return rtn;
     }
  return true;
}


bool set_switch_vlan_membership(port_list switch_ports, string switch_id, switch_vlan vlan_id, ostringstream& cmd)
{
  // HARDCODED: Arista switches don't support SNMP-writes so we have to use the
  // Arista CLI. 
  if (is_arista_switch(switch_id)) {
    return set_switch_vlan_membership_arista(switch_ports, switch_id, vlan_id, cmd);
  } else if (is_arista64_switch(switch_id)){
    return set_switch_vlan_membership_arista64(switch_ports, switch_id, vlan_id, cmd);
  } else if (is_cisco_switch(switch_id)){
    return set_switch_vlan_membership_cisco(switch_ports, switch_id, vlan_id, cmd);
  } else {
    return set_switch_vlan_membership_snmp(switch_ports, switch_id, vlan_id);
  }
}

bool set_switch_pvids(port_list switch_ports, string switch_id, switch_vlan vlan_id, ostringstream& cmd)
{
  // find out which hosts are connected to hosts
  uint32_t management_port = eth_switches->get_management_port(switch_id);
  port_list host_ports;
  for (port_iter iter = switch_ports.begin();
    iter != switch_ports.end(); iter++) {
    if (!iter->isInterSwitchPort() && 
        (iter->getPortNum() != management_port)) {
      host_ports.push_back(*iter);
    }
  } 
  
  // HARDCODED: Arista switches don't support SNMP-writes so we have to use the
  // Arista CLI. 
  if (is_arista_switch(switch_id)) {
    return set_switch_pvids_arista(host_ports, switch_id, vlan_id, cmd);
  } 
  else if (is_arista64_switch(switch_id))
    {
      return set_switch_pvids_arista64(host_ports, switch_id, vlan_id, cmd);
    } 
  else if (is_cisco_switch(switch_id)) {
    return set_switch_pvids_cisco(host_ports, switch_id, vlan_id, cmd);
  }
  else  {
    return set_switch_pvids_snmp(host_ports, switch_id, vlan_id);
  }
}

bool set_switch_pvid(switch_port port, string switch_id, switch_vlan vlan_id, ostringstream& cmd)
{
  port_list host_ports;
  host_ports.push_back(port);
  return set_switch_pvids(host_ports, switch_id, vlan_id, cmd);
}

bool initialize_switch(string switch_id)
{
  // HARDCODED: Arista switches don't support SNMP-writes so we have to use the
  // Arista CLI. 
  if (is_arista_switch(switch_id)) {
    return initialize_switch_arista(switch_id);
  } 
  else if (is_arista64_switch(switch_id)) 
    {
      return initialize_switch_arista64(switch_id);
    }
  else if (is_cisco_switch(switch_id)) 
    {
      return initialize_switch_cisco(switch_id);
    }
  else {
    return initialize_switch_snmp(switch_id);
  }
}


void initialize_ssh_locks(list<switch_info>& switches)
{
  list<switch_info>::iterator iter;
  std::string switch_id;
  for (iter = switches.begin(); iter != switches.end(); ++iter)
    {
      switch_id = iter->getSwitchId();
      if (is_arista_switch(switch_id) ||
	  is_arista64_switch(switch_id) ||
	  is_cisco_switch(switch_id)) 
	ssh_locks[switch_id].set_switch_id(switch_id);
    }
}


bool set_switch_vlan_membership_arista(port_list switch_ports, string switch_id, switch_vlan vlan_id, ostringstream& cmd)
{
  ostringstream msg;
      int num_ports = switch_ports.size();
      int num_switch_ports = eth_switches->get_num_ports(switch_id);
      if (num_switch_ports == 0) 
	{
	  return false;
	}
      
      // first remove all ports from this vlan
      cmd << "interface ethernet 1-" << num_switch_ports << endl;
      cmd << "switchport trunk allowed vlan remove " << vlan_id << endl;
      cmd << "exit" << endl;
      if (num_ports > 0)
	{
	  // now add the specified ports to the vlan
	  cmd << "interface ethernet ";
	  for (port_iter iter = switch_ports.begin();
	       iter != switch_ports.end(); iter++) {
	    cmd << iter->getPortNum();
	    if (--num_ports > 0) cmd << ",";
	  }
	  cmd << endl << "switchport trunk allowed vlan add " << vlan_id << endl;
	  cmd << "show vlan " << vlan_id << endl;
   	  cmd << "exit" << endl;
	}
    return true;
}


bool set_switch_vlan_membership_arista64(port_list switch_ports, string switch_id, switch_vlan vlan_id, ostringstream& cmd)
{
  ostringstream msg;
      int num_ports = switch_ports.size();
      int num_switch_ports = eth_switches->get_num_ports(switch_id);
      std::string last_port = get_arista64_port(num_switch_ports);
      if (num_switch_ports == 0) 
	{
	  return false;
	}
      
      // first remove all ports from this vlan
      cmd << "interface ethernet 1-" << last_port << endl;
      cmd << "switchport trunk allowed vlan remove " << vlan_id << endl;
      cmd << "exit" << endl;
      if (num_ports > 0)
	{
	  // now add the specified ports to the vlan
	  cmd << "interface ethernet ";
	  for (port_iter iter = switch_ports.begin();
	       iter != switch_ports.end(); iter++) {
	    cmd << get_arista64_port(iter->getPortNum());
	    if (--num_ports > 0) cmd << ",";
	  }
	  cmd << endl << "switchport trunk allowed vlan add " << vlan_id << endl;
          cmd << "exit" << endl;
	}
      
      return true;
}




bool set_switch_vlan_membership_cisco(port_list switch_ports, string switch_id, switch_vlan vlan_id, ostringstream& cmd)
{
  ostringstream msg;
      int num_ports = switch_ports.size();
      int num_switch_ports = eth_switches->get_num_ports(switch_id);
      if (num_switch_ports == 0) 
	{
	  return false;
	}
      
      // first remove all ports from this vlan
      cmd << "interface ethernet 1/1-" << num_switch_ports << " ; ";
      cmd << "switchport trunk allowed vlan remove " << vlan_id << " ; ";
      cmd << "exit ; " << endl;
      if (num_ports > 0)
	{
	  // now add the specified ports to the vlan
	  cmd << "interface ";
	  for (port_iter iter = switch_ports.begin();
	       iter != switch_ports.end(); iter++) {
	    cmd << "ethernet 1/" << iter->getPortNum();
	    if (--num_ports > 0) cmd << ",";
	  }
	  cmd << " ; " << endl << "switchport trunk allowed vlan add " << vlan_id << " ; " << endl;
	  cmd << "show vlan id " << vlan_id << " ; " << endl;
   	  cmd << "exit ; " << endl;
	}
    return true;
}


bool set_switch_pvids_arista64(port_list host_ports, string switch_id, switch_vlan vlan_id, ostringstream& cmd)
{
  ostringstream msg;
      int num_ports = host_ports.size();
      if (num_ports == 0) 
	{
	  return true; // don't need to do anything
	}
      
      // now add the specified ports to the vlan
      cmd << "interface ethernet ";
      for (port_iter iter = host_ports.begin();
	   iter != host_ports.end(); iter++) {
	cmd << get_arista64_port(iter->getPortNum());
	if (--num_ports > 0) cmd << ",";
      }
      cmd << endl << "switchport trunk native vlan " << vlan_id << endl;
      cmd << "exit" << endl;
      return true;
}


bool set_switch_pvids_arista(port_list host_ports, string switch_id, switch_vlan vlan_id, ostringstream& cmd)
{
  ostringstream msg;
 
      int num_ports = host_ports.size();
      if (num_ports == 0) 
	{
	  return true; // don't need to do anything
	}
      
      // now add the specified ports to the vlan
      cmd << "interface ethernet ";
      for (port_iter iter = host_ports.begin();
	   iter != host_ports.end(); iter++) {
	cmd << iter->getPortNum();
	if (--num_ports > 0) cmd << ",";
      }
      cmd << endl << "switchport trunk native vlan " << vlan_id << endl;
      cmd << "exit" << endl;
      
      return true;
}


bool set_switch_pvids_cisco(port_list host_ports, string switch_id, switch_vlan vlan_id, ostringstream& cmd)
{
  ostringstream msg;
 
      int num_ports = host_ports.size();
      if (num_ports == 0) 
	{
	  return true; // don't need to do anything
	}
      
      // now add the specified ports to the vlan
      cmd << "interface ";
      for (port_iter iter = host_ports.begin();
	   iter != host_ports.end(); iter++) {
	cmd << "ethernet 1/" << iter->getPortNum();
	if (--num_ports > 0) cmd << ",";
      }
      cmd << " ; " << endl << "switchport access vlan " << vlan_id << " ; " << endl;
      cmd << "exit ; " << endl;
      
      return true;
}

bool initialize_switch_arista64(string switch_id)
{
  ostringstream cmd;
  if (ssh_locks[switch_id].start_ssh())
    {  
      switch_info eth_switch = eth_switches->get_switch(switch_id);
      cmd << "ssh -n " << user << "@" << switch_id << " \"enable" << endl;
      cmd << "configure" << endl;
      cmd << "interface ethernet 1-" << get_arista64_port(eth_switch.getNumPorts()) << endl;
      cmd << "switchport trunk allowed vlan none" << endl;
      cmd << "switchport trunk native vlan " << default_vlan << endl;
      cmd << "\"" << endl;
  
      bool rtn =  exec_ssh(cmd.str());
      ssh_locks[switch_id].finish_ssh();
      return rtn;
    }
  else
    {
      write_log("ERROR: initialize_switch_arista64 switch " + switch_id + " ssh timed out for too many outstanding ssh connections");
      return false;
    }
}

bool initialize_switch_arista(string switch_id)
{
  ostringstream cmd;
  if (ssh_locks[switch_id].start_ssh())
    {  
      switch_info eth_switch = eth_switches->get_switch(switch_id);
      cmd << "ssh -n " << user << "@" << switch_id << " \"enable" << endl;
      cmd << "configure" << endl;
      cmd << "interface ethernet 1-" << eth_switch.getNumPorts() << endl;
      cmd << "switchport trunk allowed vlan none" << endl;
      cmd << "switchport trunk native vlan " << default_vlan << endl;
      cmd << "\"" << endl;
      
      bool rtn =  exec_ssh(cmd.str());
      ssh_locks[switch_id].finish_ssh();
      return rtn;
    }
  else
    {
      write_log("ERROR: initialize_switch_arista switch " + switch_id + " ssh timed out for too many outstanding ssh connections");
      return false;
    }
}


bool initialize_switch_cisco(string switch_id)
{
  ostringstream cmd;
  if (ssh_locks[switch_id].start_ssh())
    {  
      switch_info eth_switch = eth_switches->get_switch(switch_id);
      cmd << "ssh -i /root/.ssh/cisco_id_rsa " << user << "@" << switch_id << " \"configure ; " << endl;
      cmd << "interface ethernet 1/1-" << eth_switch.getNumPorts() << " ; " << endl;
      cmd << "switchport trunk allowed vlan none ; " << endl;
      cmd << "switchport trunk native vlan " << default_vlan << " ; exit " << endl;
      cmd << "\"" << endl;
      
      bool rtn =  exec_ssh(cmd.str());
      ssh_locks[switch_id].finish_ssh();
      return rtn;
    }
  else
    {
      write_log("ERROR: initialize_switch_arista switch " + switch_id + " ssh timed out for too many outstanding ssh connections");
      return false;
    }
}


bool set_switch_vlan_membership_snmp(port_list switch_ports, string switch_id, switch_vlan vlan_id)
{
  // all ports in switch_ports should have same switch_id
  ostringstream cmd1, cmd2;
  string portStr;
  bool status = true;
  int numBytes = (int)ceil(((double)eth_switches->get_num_ports(switch_id))/8);
  if (numBytes == 0) return false;

  // set all ports not in this list to be explicitly excluded from vlan
  portStr = encodeEgressPorts(switch_ports, numBytes, true);    
  //if (switch_id == "onlsw5") // onlsw5 currently can't be logged in to to change its settings
  //  cmd1 << "snmpset -v3 " 
  //       << "-u " << user << " " 
  //       << switch_id
  //       << " Q-BRIDGE-MIB::dot1qVlanForbiddenEgressPorts." << vlan_id
  //       << " x " << portStr;
  //else
    cmd1 << "snmpset -v3 " 
         << "-u " << user << " " 
         << "-l " << seclevel << " " 
         << "-a " << authentication << " " 
         << "-A " << password << " " 
         << switch_id
         << " Q-BRIDGE-MIB::dot1qVlanForbiddenEgressPorts." << vlan_id
         << " x " << portStr;
  status &= exec_snmp(cmd1.str());

  // set all ports in this list to be included in this vlan
  portStr = encodeEgressPorts(switch_ports, numBytes, false);    
  //if (switch_id == "onlsw5") // onlsw5 currently can't be logged in to to change its settings
  //  cmd2 <<  "snmpset -v3 "
  //       << "-u " << user << " " 
  //       << switch_id
  //       << " Q-BRIDGE-MIB::dot1qVlanStaticEgressPorts." << vlan_id
  //       << " x " << portStr;
  //else
    cmd2 <<  "snmpset -v3 "
         << "-u " << user << " " 
         << "-l " << seclevel << " " 
         << "-a " << authentication << " " 
         << "-A " << password << " " 
         << switch_id
         << " Q-BRIDGE-MIB::dot1qVlanStaticEgressPorts." << vlan_id
         << " x " << portStr;

  status &= exec_snmp(cmd2.str());

  return status;
}

bool set_switch_pvids_snmp(port_list host_ports, string switch_id, switch_vlan vlan_id)
{
  // all ports in host_ports should have same switch_id
  // and should be inter-switch ports
  
  bool status = true;

  for (port_iter iter = host_ports.begin();
       iter != host_ports.end(); iter++) {
    ostringstream cmd;
    //if (switch_id == "onlsw5") // onlsw5 currently can't be logged in to to change its settings
    //  cmd << "snmpset -v3 "
    //      << "-u " << user << " " 
    //      << switch_id
    //      << " Q-BRIDGE-MIB::dot1qPvid." << iter->getPortNum()
    //      << " u " << vlan_id;
    //else
      cmd << "snmpset -v3 "
          << "-u " << user << " " 
          << "-l " << seclevel << " " 
          << "-a " << authentication << " " 
          << "-A " << password << " " 
          << switch_id
          << " Q-BRIDGE-MIB::dot1qPvid." << iter->getPortNum()
          << " u " << vlan_id;
    exec_snmp(cmd.str());
  }
  
  return status;
}

bool initialize_switch_snmp(string switch_id) 
{
  bool status = true;
  
  // set all vlans to have no ports
  port_list no_ports;
  ostringstream cmd;
  for (uint32_t i = 0; i < vlans->get_num_vlans(); i++) {
    vlan *curVlan = vlans->get_vlan_by_index(i);
    status &= set_switch_vlan_membership(no_ports, switch_id, curVlan->vlan_id(), cmd);
  }

  uint32_t num_ports = eth_switches->get_num_ports(switch_id);
  port_list all_ports;
  for (uint32_t i = 1; i <= num_ports; i++) {
    all_ports.push_back(switch_port(switch_id, i));
  }
  // function below will exclude management port
  status &= set_switch_pvids(all_ports, switch_id, default_vlan, cmd);


  return status;
}


bool is_arista_switch(string switch_id)
{
  // Here we assume swithces with switchid = onslw7 and onlsw8 
  // are Arista switches
  // 8/10/10 we switched from 2-48 port aristas to 4-24 port aristas
  // so assume onlsw9 and onlsw10 are also arista switches
  return (switch_id == "onlsw7" || switch_id == "onlsw8" ||
  	  switch_id == "onlsw9" || switch_id == "onlsw10"); 
}


bool is_arista64_switch(string switch_id)
{
  // Here we assume swithces with switchid = onslw7 and onlsw8 
  // are Arista switches
  // 8/10/10 we switched from 2-48 port aristas to 4-24 port aristas
  // so assume onlsw9 and onlsw10 are also arista switches
  return (switch_id == "onlsw11" || switch_id == "onlsw12"); 
}


bool is_cisco_switch(string switch_id)
{
  // Here we assume swithces with switchid = onslw7 and onlsw8 
  // are Arista switches
  // 8/10/10 we switched from 2-48 port aristas to 4-24 port aristas
  // so assume onlsw9 and onlsw10 are also arista switches
  return (switch_id == "onlsw1" || switch_id == "onlsw2" || switch_id == "onlsw3" || switch_id == "onlsw4"); 
}

bool exec_snmp(string cmd)
{
  int rtn = 0;

#ifdef DEBUG
  // in debug mode we write the command to the log 
  // file rather than execute it
  write_log(cmd);
#else
  write_log(cmd);
  rtn = system(cmd.c_str());
  rtn = WEXITSTATUS(rtn);
#endif

  // Mart: 6/17/10
  // on error lets print out what command failed
  if (rtn != 0) {
    write_log("ERROR! The following snmp command failed:");
    write_log(cmd.c_str());
  }


  return (rtn == 0);
}

bool exec_ssh(string cmd)
{
  int rtn = 0;

#ifdef DEBUG
  // in debug mode we write the command to the log 
  // file rather than execute it
  write_log(cmd);
#else
  write_log(cmd);
  rtn = system(cmd.c_str());
  rtn = WEXITSTATUS(rtn);
#endif

  // Mart: 6/17/10
  // on error lets print out what command failed
  if (rtn != 0) {
    write_log("ERROR! The following ssh command failed:");
    write_log(cmd.c_str());
  }

  return (rtn == 0);
}

void getHexString(unsigned char bval, char* str)
{
  int up = bval >> 4;
  int down = bval & 0x0f;
  sprintf(str, "%x%x", up, down);
}

string encodeEgressPorts(port_list ports, int numBytes, bool is_complement)
{
  string retVal;
  int i, p, byte_ndx, bit_shift;
  unsigned char bytes[numBytes]; 

  for (i = 0; i < numBytes; ++i) bytes[i] = 0;
  
  for (port_iter iter = ports.begin();
       iter != ports.end(); iter++)
  {
    p = iter->getPortNum() - 1;
    byte_ndx = (int)floor(p/8);
    bit_shift = p%8;
    if (byte_ndx < numBytes)
    {
      bytes[byte_ndx] = bytes[byte_ndx] | (0x80 >> bit_shift);
    }
  }

  if (is_complement) {
    for (i = 0; i < numBytes; ++i) bytes[i] = ~(bytes[i]);
  }

  char tmp_string[3];
  for (i = 0; i < numBytes; ++i)
  {
    getHexString(bytes[i], tmp_string);
    retVal.append(tmp_string);
  }
  return retVal;
}


string get_arista64_port(int port)
{
  std::string rtn_str;
  char tmp_port[10];
  if (port < 49) 
    {
      sprintf(tmp_port, "%d", port);
      rtn_str.assign(tmp_port);
    }
  else
    {
      int base = 49;
      int remainder = port - 49;
      while (remainder > 3)
	{
	  remainder -= 4;
	  ++base;
	}
      ++remainder;
      sprintf(tmp_port, "%d", base);
      rtn_str.assign(tmp_port);
      rtn_str.append("/");
      sprintf(tmp_port, "%d", remainder);
      rtn_str.append(tmp_port);
    }
  return rtn_str; 
}

