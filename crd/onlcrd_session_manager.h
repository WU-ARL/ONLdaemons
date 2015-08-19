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
 *
 */

#ifndef _ONLCRD_SESSION_MANAGER_H
#define _ONLCRD_SESSION_MANAGER_H

namespace onlcrd
{
  #define MIN_RLI_VERSION 0x54


  class session_manager
  { 
    public:
      session_manager() throw();
      ~session_manager();
  
      void initialize();

      bool authenticate(std::string username, std::string password);
      bool has_session(std::string username);
      int reservation_time_left(std::string username);
      bool is_admin(std::string username);
      bool extend_reservation(std::string username, uint32_t minutes);
      bool cancel_reservation(std::string username);

      int get_capacity(std::string type, int port);
      bool has_virtual_port(std::string type);
      void get_switch_ports(unsigned int cid, switch_port& p1, switch_port& p2);
      void fix_component(onl::topology* top, uint32_t id, std::string cp);
      bool make_reservation(reservation* res, std::string username, std::string early_start, std::string late_start, uint32_t duration, onl::topology* top);
      bool assign_resources(session_ptr sess, std::string username, onl::topology* top);
      void return_resources(std::string username, onl::topology* top);
      void check_for_expired_sessions();

      void received_up_msg(std::string name);
      void set_keeboot_param(std::string name, std::string param);
      bool check_keeboot_param(std::string name);
      std::string get_keeboot_param(std::string name);

      vlan_ptr add_vlan(std::string sid);
      bool add_port_to_vlan(vlan_ptr vlan, switch_port port, std::string sid);
      bool remove_port_from_vlan(vlan_ptr vlan, switch_port port, std::string sid);
      bool delete_vlan(vlan_ptr vlan, std::string sid);
      bool add_port_to_outstanding_list(switch_port port);
      void remove_port_from_outstanding_list(switch_port port);

      crd_component_ptr get_component(std::string name, unsigned int vmid);
      void add_component(crd_component_ptr comp, session_ptr sess);
      void remove_component(crd_component_ptr comp);
      void clear_component(crd_component* comp);

      session_ptr add_session(std::string username, nccp_connection* conn, begin_session_req* beg_req=NULL);
      session_ptr get_session(nccp_connection* conn);
      session_ptr get_session(nccp_connection* conn, std::string sid);
      session_ptr get_session(std::string username);
      void remove_session(session* sess);

      reservation* add_reservation(std::string username, nccp_connection* conn, begin_reservation_req* beg_req=NULL);
      reservation* get_reservation(nccp_connection* conn);
      reservation* get_reservation(nccp_connection* conn, std::string rid);
      void remove_reservation(reservation* res);

      void get_observable_sessions(std::string username, std::list<session_ptr>& list);

      bool is_initializing() { return initializing;}

      bool start_session_vlans(std::string sid);

    private: 
      pthread_mutex_t vlan_lock;
      std::map<switch_vlan,int> num_ports_on_vlan;
      
      pthread_mutex_t port_lock;
      std::set<switch_port> outstanding_ports;

      pthread_mutex_t comp_lock;
      std::list<crd_component_ptr> active_components;
      std::list<crd_component_ptr> refreshing_components;
      std::list<crd_component *> keeboot_components;
      std::map<std::string,std::string> keeboot_params;

      pthread_mutex_t session_lock;
      std::list<session_ptr> active_sessions;
      uint32_t next_session_id;

      // if we never get a reservation commit, then the memory never gets cleaned up..
      pthread_mutex_t reservation_lock;
      std::list<reservation *> active_reservations;
      uint32_t next_reservation_id;

      pthread_mutex_t db_lock;
      onl::onldb* database;

      //initialization flag set true while sending refresh and waiting for all nodes to come up or go into repair //JP added 11/13/12
      bool initializing;
  }; // class session_manager
};

#endif // _ONLCRD_SESSION_MANAGER_H
