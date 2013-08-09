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

#include <algorithm>

#include "nmd_includes.h"

vlan::vlan()
{
  // initialize lock
  pthread_mutex_init(&lock, NULL);
}

vlan::~vlan()
{
}

void vlan::set_vlan_id(switch_vlan vlan_id)
{
  vlanId = vlan_id;
}


port_list vlan::get_switch_ports(string switch_id)
{
  // unfortunately when we configure a vlan on a switch
  // we have to specify all ports at time
  // Here we simply iterate through all ports on this vlan
  // and extract those that match the switch for
  // the port we are adding
  port_list switch_ports;
  for (port_iter iter = ports.begin();
       iter != ports.end(); iter++) {
    if (iter->getSwitchId() == switch_id) {
      switch_ports.push_back(*iter);
    }
  }

  return switch_ports;
}

bool vlan::add_port(switch_port p)
{
  bool status = true;

  ports.push_back(p);
  
  // for this switch, get the list of ports on this vlan
  port_list switch_ports = get_switch_ports(p.getSwitchId());

  // for this switch, update the set of ports on this vlan
  status &= set_switch_vlan_membership(switch_ports, p.getSwitchId(), vlanId);
 
  // set the PVID so that untagged packets will be tagged
  // with this VLAN ID
  status &= set_switch_pvid(p, p.getSwitchId(), vlanId);

  return status;
}

bool vlan::delete_port(switch_port p)
{
  bool status = true;
 
  ports.remove(p);

  // for this switch, get the list of ports on this vlan
  port_list switch_ports = get_switch_ports(p.getSwitchId());

  // for this switch, update the set of ports on this vlan
  status &= set_switch_vlan_membership(switch_ports, p.getSwitchId(), vlanId);

  // Set the PVID back to the default VLAN for no experiment
  // This shouldn't be necessary but we do it to be safe
  status &= set_switch_pvid(p, p.getSwitchId(), default_vlan);
 

  return status;
}

bool vlan::clear_vlan()
{
  // iterate through ports and build list of switches to clear vlan state from
  bool status = true;
  list<string> switch_list;
  list<string>::iterator siter;
  for (port_iter iter = ports.begin();
       iter != ports.end(); iter++) {
    siter = find(switch_list.begin(), switch_list.end(), iter->getSwitchId());
    if (siter == switch_list.end()) {
      switch_list.push_back(iter->getSwitchId());
    } 
  }
 
  port_list no_ports; // create empty list of ports
  for (siter = switch_list.begin();
       siter != switch_list.end(); siter++) {
    // for this switch, the ports on this vlan need to have their
    // PVIDs set back to the default vlan for no experiment
    port_list pvidPorts = get_switch_ports(*siter);
    status &= set_switch_pvids(pvidPorts, *siter, default_vlan);
    // no ports on this switch should be a member of this vlan
    status &= set_switch_vlan_membership(no_ports, *siter, vlanId);
  }

  ports.clear();

  inUse = false;

  return status;
}

void vlan::print_ports()
{
  ostringstream msg;
  msg << "vlan " << vlanId << " has ports:" << endl;
  port_iter iter;
  for (iter = ports.begin(); iter != ports.end(); iter++)
  {
    msg << *iter << " ";
  }
  write_log(msg.str());
}

vlan_set::vlan_set(uint32_t num_vlans_) : num_vlans(num_vlans_)
{
  uint32_t i;
  switch_vlan vlan_num = 2;
  vlans = new vlan[num_vlans_];
  for (i = 0; i < num_vlans; i++) {
    vlans[i].set_vlan_id(vlan_num + i);
    vlans[i].set_in_use(false);   
  }

  // initialize lock
  pthread_mutex_init(&lock, NULL);
}

vlan_set::~vlan_set()
{
  // is this right?
  delete vlans;
}


vlan *vlan_set::get_vlan(switch_vlan vlan_id)
{
  uint32_t i = 0;
  for (i = 0; i < num_vlans; i++) {
    if (vlans[i].vlan_id() == vlan_id) {
      return &vlans[i];
    }
  }
  return NULL;
}

uint32_t vlan_set::get_vlan_index(switch_vlan vlan_id)
{
  uint32_t i = 0;
  for (i = 0; i < num_vlans; i++) {
    if (vlans[i].vlan_id() == vlan_id) {
      return i;
    }
  }
  return VLAN_NOT_FOUND;
}

vlan *vlan_set::get_vlan_by_index(uint32_t index)
{
  if (index >= num_vlans) return NULL;
  return &vlans[index];
}


bool vlan_set::add_vlan(switch_vlan &vlan_id)
{
  autoLock slock(lock);
  vlan_id = alloc_vlan();
  if (vlan_id == NO_FREE_VLANS) {
    write_log("add_link_req::handle() no free vlans!");
    return false;
  }
 return true;
}


bool vlan_set::delete_vlan(switch_vlan vlan_id)
{
  autoLock slock(lock);
  uint32_t vlan_index = get_vlan_index(vlan_id);
  vlans[vlan_index].clear_vlan(); 
  return true;
}

bool vlan_set::add_to_vlan(switch_vlan vlan_id, switch_port port)
{
  autoLock slock(lock);
  ostringstream msg;
  vlan *curVlan = get_vlan(vlan_id);
  if (!curVlan->is_in_use()) 
  { 
    msg << "add_to_vlan() could not find vlan " << vlan_id;
    write_log(msg.str());
    return false;
  }
  curVlan->add_port(port);
  curVlan->print_ports();

  return true;
}

bool vlan_set::delete_from_vlan(switch_vlan vlan_id, switch_port port)
{
  autoLock slock(lock);
  ostringstream msg;
  vlan *curVlan = get_vlan(vlan_id);
  if (!curVlan->is_in_use()) 
  { 
    msg << "delete_from_vlan() could not find vlan " << vlan_id;
    write_log(msg.str());
    return false;
  }
  curVlan->delete_port(port);
  curVlan->print_ports();

  return true;
}


// Private members

switch_vlan vlan_set::alloc_vlan()
{
  uint32_t i = 0;
  for (i = 0; i < num_vlans; i++) {
    if (!vlans[i].is_in_use()) {
      vlans[i].set_in_use(true);
      return vlans[i].vlan_id();
    }
  }
  return NO_FREE_VLANS;
}


switch_info_set::switch_info_set()
{
}

switch_info_set::~switch_info_set()
{
}

void switch_info_set::set_switches(list<switch_info> switches_)
{
  switches = switches_;
}

switch_info switch_info_set::get_switch(string switch_id)
{
  // return a copy of the switch_info object specified by 
  // switch_id
  // if it doesn't exist, return a switch_info object
  // with switch_info not found
  list<switch_info>::iterator iter;
  for (iter = switches.begin();
       iter != switches.end(); iter++) {
    if (iter->getSwitchId() == switch_id) return *iter;
  }
  return switch_info("not_found", 0);
}

uint32_t switch_info_set::get_num_ports(string switch_id)
{
  ostringstream msg;
  switch_info eth_switch = get_switch(switch_id);
  if (eth_switch.getSwitchId() != switch_id) {
    msg << endl << "ERROR: Could not find switch "
        << switch_id << "!" << endl;
    write_log(msg.str());
    return 0 ;
  }
  return eth_switch.getNumPorts();
}

uint32_t switch_info_set::get_management_port(string switch_id)
{
  ostringstream msg;
  switch_info eth_switch = get_switch(switch_id);
  if (eth_switch.getSwitchId() != switch_id) {
    msg << endl << "ERROR: Could not find switch "
        << switch_id << "!" << endl;
    write_log(msg.str());
    return 0 ;
  }
  return eth_switch.getManagementPort();
}


