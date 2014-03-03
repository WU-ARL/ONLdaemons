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

#include "nmd_includes.h"
#include <math.h>
#include <sys/wait.h>
#include <stdlib.h>

#define user "admin"

bool set_switch_vlan_membership_arista64(port_list switch_ports, string switch_id, switch_vlan vlan_id);
bool set_switch_vlan_membership_arista(port_list switch_ports, string switch_id, switch_vlan vlan_id);
bool set_switch_vlan_membership_snmp(port_list switch_ports, string switch_id, switch_vlan vlan_id);
bool set_switch_pvids_arista64(port_list host_ports, string switch_id, switch_vlan vlan_id);
bool set_switch_pvids_arista(port_list host_ports, string switch_id, switch_vlan vlan_id);
bool set_switch_pvids_snmp(port_list host_ports, string switch_id, switch_vlan vlan_id);
bool initialize_switch_arista64(string switch_id);
bool initialize_switch_arista(string switch_id);
bool initialize_switch_snmp(string switch_id);
bool is_arista64_switch(string switch_id);
bool is_arista_switch(string switch_id);
bool exec_snmp(string cmd);
bool exec_ssh(string cmd);
void getHexString(unsigned char bval, char* str);
string encodeEgressPorts(port_list ports, int numBytes, bool is_complement);
string get_arista64_port(int port);


bool set_switch_vlan_membership(port_list switch_ports, string switch_id, switch_vlan vlan_id)
{
  // HARDCODED: Arista switches don't support SNMP-writes so we have to use the
  // Arista CLI. 
  if (is_arista_switch(switch_id)) {
    return set_switch_vlan_membership_arista(switch_ports, switch_id, vlan_id);
  } else{
    if (is_arista64_switch(switch_id)){
      return set_switch_vlan_membership_arista64(switch_ports, switch_id, vlan_id);
    } else{
      return set_switch_vlan_membership_snmp(switch_ports, switch_id, vlan_id);
    }
  }
}

bool set_switch_pvids(port_list switch_ports, string switch_id, switch_vlan vlan_id)
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
    return set_switch_pvids_arista(host_ports, switch_id, vlan_id);
  } 
  else if (is_arista64_switch(switch_id))
    {
      return set_switch_pvids_arista64(host_ports, switch_id, vlan_id);
    } 
  else {
    return set_switch_pvids_snmp(host_ports, switch_id, vlan_id);
  }
}

bool set_switch_pvid(switch_port port, string switch_id, switch_vlan vlan_id)
{
  port_list host_ports;
  host_ports.push_back(port);
  return set_switch_pvids(host_ports, switch_id, vlan_id);
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
  else {
    return initialize_switch_snmp(switch_id);
  }
}


bool set_switch_vlan_membership_arista(port_list switch_ports, string switch_id, switch_vlan vlan_id)
{
  ostringstream cmd, msg;
  int num_ports = switch_ports.size();
  int num_switch_ports = eth_switches->get_num_ports(switch_id);
  if (num_switch_ports == 0) return false;

  cmd << "ssh -n " << user << "@" << switch_id << " \"enable" << endl;
  cmd << "configure" << endl;
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
  }
  cmd << "\"" << endl;

  return exec_ssh(cmd.str());
}


bool set_switch_vlan_membership_arista64(port_list switch_ports, string switch_id, switch_vlan vlan_id)
{
  ostringstream cmd, msg;
  int num_ports = switch_ports.size();
  int num_switch_ports = eth_switches->get_num_ports(switch_id);
  std::string last_port = get_arista64_port(num_switch_ports);
  if (num_switch_ports == 0) return false;

  cmd << "ssh -n " << user << "@" << switch_id << " \"enable" << endl;
  cmd << "configure" << endl;
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
  }
  cmd << "\"" << endl;

  return exec_ssh(cmd.str());
}


bool set_switch_pvids_arista64(port_list host_ports, string switch_id, switch_vlan vlan_id)
{
  ostringstream cmd, msg;
 
  int num_ports = host_ports.size();
  if (num_ports == 0) return true; // don't need to do anything

  cmd << "ssh -n " << user << "@" << switch_id << " \"enable" << endl;
  cmd << "configure" << endl;
  // first remove all ports from this vlan
  // now add the specified ports to the vlan
  cmd << "interface ethernet ";
  for (port_iter iter = host_ports.begin();
       iter != host_ports.end(); iter++) {
    cmd << get_arista64_port(iter->getPortNum());
     if (--num_ports > 0) cmd << ",";
  }
  cmd << endl << "switchport trunk native vlan " << vlan_id << endl;
  cmd << "\"" << endl;

  return exec_ssh(cmd.str());
}


bool set_switch_pvids_arista(port_list host_ports, string switch_id, switch_vlan vlan_id)
{
  ostringstream cmd, msg;
 
  int num_ports = host_ports.size();
  if (num_ports == 0) return true; // don't need to do anything

  cmd << "ssh -n " << user << "@" << switch_id << " \"enable" << endl;
  cmd << "configure" << endl;
  // first remove all ports from this vlan
  // now add the specified ports to the vlan
  cmd << "interface ethernet ";
  for (port_iter iter = host_ports.begin();
       iter != host_ports.end(); iter++) {
     cmd << iter->getPortNum();
     if (--num_ports > 0) cmd << ",";
  }
  cmd << endl << "switchport trunk native vlan " << vlan_id << endl;
  cmd << "\"" << endl;

  return exec_ssh(cmd.str());
}

bool initialize_switch_arista64(string switch_id)
{
  ostringstream cmd;
  switch_info eth_switch = eth_switches->get_switch(switch_id);
  cmd << "ssh -n " << user << "@" << switch_id << " \"enable" << endl;
  cmd << "configure" << endl;
  cmd << "interface ethernet 1-" << get_arista64_port(eth_switch.getNumPorts()) << endl;
  cmd << "switchport trunk allowed vlan none" << endl;
  cmd << "switchport trunk native vlan " << default_vlan << endl;
  cmd << "\"" << endl;

  return exec_ssh(cmd.str());
}

bool initialize_switch_arista(string switch_id)
{
  ostringstream cmd;
  switch_info eth_switch = eth_switches->get_switch(switch_id);
  cmd << "ssh -n " << user << "@" << switch_id << " \"enable" << endl;
  cmd << "configure" << endl;
  cmd << "interface ethernet 1-" << eth_switch.getNumPorts() << endl;
  cmd << "switchport trunk allowed vlan none" << endl;
  cmd << "switchport trunk native vlan " << default_vlan << endl;
  cmd << "\"" << endl;

  return exec_ssh(cmd.str());
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
  cmd1 << "snmpset -v3 -u " << user << " " << switch_id
       << " Q-BRIDGE-MIB::dot1qVlanForbiddenEgressPorts." << vlan_id
       << " x " << portStr;
  status &= exec_snmp(cmd1.str());

  // set all ports in this list to be included in this vlan
  portStr = encodeEgressPorts(switch_ports, numBytes, false);    
  cmd2 <<  "snmpset -v3 -u " << user << " " << switch_id
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
    cmd << "snmpset -v3 -u " << user << " " << switch_id
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
  for (uint32_t i = 0; i < vlans->get_num_vlans(); i++) {
    vlan *curVlan = vlans->get_vlan_by_index(i);
    status &= set_switch_vlan_membership(no_ports, switch_id, curVlan->vlan_id());
  }

  uint32_t num_ports = eth_switches->get_num_ports(switch_id);
  port_list all_ports;
  for (uint32_t i = 1; i <= num_ports; i++) {
    all_ports.push_back(switch_port(switch_id, i));
  }
  // function below will exclude management port
  status &= set_switch_pvids(all_ports, switch_id, default_vlan);


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


