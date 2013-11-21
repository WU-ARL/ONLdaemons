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

#ifndef _NMD_TYPES_H
#define _NMD_TYPES_H

#define NO_FREE_VLANS (uint32_t)-1
#define VLAN_NOT_FOUND (uint32_t)-1

typedef list<switch_port> port_list;
typedef list<switch_port>::iterator port_iter;

class vlan {
  public:
    vlan();
    ~vlan();

    void set_vlan_id(switch_vlan vlan_id);
    switch_vlan vlan_id() {return vlanId;}
    void set_in_use(bool in_use) {inUse = in_use;}
    bool is_in_use() {return inUse;}
    bool add_port(switch_port p);
    bool delete_port(switch_port p);
    bool clear_vlan();
    void print_ports();
    void initialize();

  private:
    port_list get_switch_ports(string switch_id);
    bool inUse;
    switch_vlan vlanId;
    port_list ports;
    pthread_mutex_t lock;
};

class vlan_set {
  public:
    vlan_set(uint32_t num_vlans_);
    ~vlan_set();

    vlan *get_vlan(switch_vlan vlan_id);
    uint32_t get_vlan_index(switch_vlan vlan_id);
    vlan * get_vlan_by_index(uint32_t index);
    uint32_t get_num_vlans() {return num_vlans;} 

    bool add_vlan(switch_vlan &vlan_id);
    bool delete_vlan(switch_vlan vlan_id);
    bool add_to_vlan(switch_vlan vlan_id, switch_port port);
    bool delete_from_vlan(switch_vlan vlan_id, switch_port port);
    void initialize_vlans();

  private:
    switch_vlan alloc_vlan();

    uint32_t num_vlans;
    vlan *vlans;
    pthread_mutex_t lock; 
};


// the set of switches
class switch_info_set {
  public:
    switch_info_set();
    ~switch_info_set();

    // set the active switches the nmd is managing
    void set_switches(list<switch_info> switches_);
    // return switch info for switch named switch_id
    switch_info get_switch(string switch_id);
    // return number of ports on specified switch
    uint32_t get_num_ports(string switch_id);
    // return the management port # on the specified switch
    uint32_t get_management_port(string switch_id);

  private:
    list<switch_info> switches;
};

#endif

