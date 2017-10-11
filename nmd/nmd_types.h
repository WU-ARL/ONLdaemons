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
    bool set_in_use(bool in_use, std::string sid); //{inUse = in_use;}
    bool is_in_use() {return inUse;}
    bool add_port(switch_port p);
    bool delete_port(switch_port p);
    bool clear_vlan();
    void clear_if_empty();//resets state if no ports
    void print_ports();
    void initialize();
    //void set_session_id(std::string sid) { sessionID = sid;}
    std::string get_session_id() { return sessionID;}

    bool get_add_ports_cmd(string switch_id, ostringstream& cmd);
    //generates clear vlan cmd for the given switch and returns number of ports
    //removed from the vlan
    int get_clear_vlan_cmd(string switch_id, ostringstream& cmd);

  private:
    port_list get_switch_ports(string switch_id);
    pthread_mutex_t& get_switch_lock(string switch_id);
    bool inUse;
    switch_vlan vlanId;
    port_list ports;
    pthread_mutex_t lock;
    std::string sessionID;
    std::map<std::string, pthread_mutex_t> switch_locks;
};

typedef struct _switch_cmd
{
  string switch_id;
  ostringstream cmd;
  int state;
  string debug_name;
} switch_cmd;//struct switch_cmd

class session {
 public:
  session(std::string str);
  ~session();

  //bool add_request(nmd::add_to_vlan_req* req);//adds request to switch list
  bool add_switch(std::string swid);
  bool create_vlans();//sends vlan requests for all vlans
  void add_vlan(switch_vlan vlan_id);//adds id to active list
  bool remove_vlan(switch_vlan vlan_id); //removes vlan from active to removal when the active list is empty it clears the session.
  bool is_cleared() { return cleared;} //called to check if it should be removed
  std::string get_session_id() { return sessionID;}
  bool clear_session(); //called when last vlan is cleared sends vlan clears at once 

 private:
  bool create_vlans_unthreaded();//sends vlan requests for all vlans
  bool create_vlans_threaded();//sends vlan requests for all vlans
  bool clear_session_unthreaded(); //called when last vlan is cleared sends vlan clears at once 
  bool clear_session_threaded(); //called when last vlan is cleared sends vlan clears at once 
  std::string sessionID;
  std::list<switch_cmd* > switch_commands;
  std::list<std::string> switches;
  pthread_mutex_t lock; 
  std::list<switch_vlan> active_vlans;
  std::list<switch_vlan> removed_vlans;
  bool cleared;
};

typedef boost::shared_ptr<session> session_ptr;

class vlan_set {
  public:
    vlan_set(uint32_t num_vlans_);
    ~vlan_set();

    vlan *get_vlan(switch_vlan vlan_id);
    void free_vlan(switch_vlan vlanid);
    uint32_t get_vlan_index(switch_vlan vlan_id);
    vlan * get_vlan_by_index(uint32_t index);
    uint32_t get_num_vlans() {return num_vlans;} 

    bool add_vlan(switch_vlan &vlan_id, std::string sid);
    bool delete_vlan(switch_vlan vlan_id, std::string sid);
    bool add_to_vlan(switch_vlan vlan_id, switch_port port, std::string sid);
    //bool add_to_vlan(nmd::add_to_vlan_req* req, std::string sid);
    bool delete_from_vlan(switch_vlan vlan_id, switch_port port, std::string sid);
    void initialize_vlans();
    bool create_session_vlans(std::string sid);
    bool clear_session(std::string sid);

  private:
    switch_vlan alloc_vlan(std::string sid);

    std::list<session_ptr> sessions;

    session_ptr get_session(std::string sid);
    void remove_session(session_ptr sptr);

    uint32_t num_vlans;
    vlan *vlans;
    std::list<switch_vlan> free_vlans;
    pthread_mutex_t lock; 
    pthread_mutex_t free_vlanslock; 
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

