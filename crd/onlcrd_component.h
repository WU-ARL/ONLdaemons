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

#ifndef _ONLCRD_COMPONENT_H
#define _ONLCRD_COMPONENT_H

namespace onlcrd
{
  class session;
  class session_add_component_req;
  class session_add_link_req;
  class crd_link;

  class crd_component
  {
    public:
      crd_component(std::string n, std::string c, unsigned short p, bool do_k, bool is_d=false);
      virtual ~crd_component();

      std::string getName() { return name; }
      bool hasCP() { return (cp.length() != 0); }
      std::string getCP() { return cp; }
      unsigned short getCPPort() { return cp_port; }
      bool isKeeboot() { return keeboot; }
     
      bool operator<(const crd_component& c) const;
      bool operator==(const crd_component& c);

      virtual void set_vlan(switch_vlan) {}

      void set_session(session_ptr s);
      void clear_session();
      session_ptr get_session() { return sess; }
      void set_component(component& c) { comp = c; }
      component& get_component() { return comp; }
      void set_ip_addr(std::string addr) { ip_addr = addr; }

      unsigned int getVMID() { return vmid;}

      bool is_admin_mode();
      virtual std::string get_type();

      bool ignore();
      void add_reboot_params(std::list<param>& params);
      void add_init_params(std::list<param>& params);
      void reset_params();

      void set_add_request(session_add_component_req* req) { compreq = req; }
      virtual void add_link(boost::shared_ptr<crd_link> l);
      void get_links(std::list< boost::shared_ptr<crd_link> >& ls) { ls = links; }
      nccp_connection* get_connection() { return nccpconn; }

      void wait_for_last_init();

      virtual void initialize();
      virtual void do_initialize();
      virtual void refresh();
      virtual void do_refresh();

      void got_up_msg();

      bool marked() { return mark; }
      void set_marked(bool m) { mark = m; }
      void setCores(unsigned int c) { cores = c;}
      unsigned int getCores() { return cores;}
      void setMemory(unsigned int m) { memory = m;}
      unsigned int getMemory() { return memory;}


    protected:
      std::string name;
      std::string cp;
      unsigned short cp_port;
      bool keeboot;
      bool is_dependent;

      unsigned int vmid;
      unsigned int cores;
      unsigned int memory;

      //pthread_mutex_t db_lock;
      onl::onldb* database;

      session_ptr sess;
      component comp;
      std::string ip_addr;

      std::list<param> reboot_params;
      std::list<param> init_params;

      session_add_component_req* compreq;
      std::list< boost::shared_ptr<crd_link> > links;
      nccp_connection* nccpconn;

      pthread_mutex_t state_lock;
      std::string internal_state;
      bool needs_refresh;
      std::string local_state;
      bool keebooting_now;
      pthread_mutex_t up_msg_mutex;
      pthread_cond_t up_msg_cond;
      bool received_up_msg;

      bool mark; 

      virtual std::string get_state();
      void set_state(std::string s);

      bool wait_for_up_msg(int timeout);
      void cleanup_reqs(bool failed);
      void cleanup_links(bool do_init);
  }; //class crd_component

  typedef boost::shared_ptr<crd_component> crd_component_ptr;

  void* crd_component_do_initialize_wrapper(void* obj);
  void* crd_component_do_refresh_wrapper(void* obj);

  class crd_virtual_switch : public crd_component
  {
    public:
      crd_virtual_switch(std::string n);
      virtual ~crd_virtual_switch();
  
      virtual void set_vlan(switch_vlan v) { vlan = v; }

      virtual std::string get_type();

      virtual void do_initialize();
      virtual void refresh();
      virtual void do_refresh();

    protected:
      virtual std::string get_state();
  
      switch_vlan vlan;
  }; //class crd_virtual_switch

  class crd_virtual_machine : public crd_component
  {
    public:
      crd_virtual_machine(std::string n, std::string c, unsigned short p, bool do_k, bool is_d=false);
      virtual ~crd_virtual_machine();
  

      virtual std::string get_type();

      //virtual void do_initialize();
      //virtual void refresh();
      //virtual void do_refresh();

    protected:
      virtual std::string get_state();
  
  }; //class crd_virtual_machine

  class crd_link
  {
    public:
      crd_link(crd_component_ptr e1, unsigned short e1p, crd_component_ptr e2, unsigned short e2p);
      ~crd_link();

      void set_component(component& c) { comp = c; }
      component& get_component() { return comp; }

      void set_vlan(switch_vlan v) { link_vlan = v; alloc_vlan = false; }
      bool allocate_vlan();
      void set_rports(unsigned short p1, unsigned short p2) { endpoint1_rport = p1; endpoint2_rport = p2;}

      crd_component_ptr get_node1() { return endpoint1; }
      crd_component_ptr get_node2() { return endpoint2; }

      unsigned int get_node1_capacity() { return endpoint1_cap;}
      unsigned int get_node2_capacity() { return endpoint2_cap;}
      void set_capacity(int c1, int c2) { endpoint1_cap = c1; endpoint2_cap = c2;}

      void set_switch_ports(std::list<int>& connlist);

      void set_add_request(session_add_link_req* req) { linkreq = req; }
      session_add_link_req* get_add_request() { return linkreq; }
      void set_session(session_ptr s);
      void clear_session();
      bool has_failed();

      bool send_port_configuration(crd_component* c, bool use2=false);

      bool both_comps_done();
      void initialize();
      void do_initialize();
      void refresh();
      void do_refresh();

      bool marked() { return mark; }
      void set_marked(bool m) { mark = m; }
      bool is_loopback(); //ADDED 9/2/2010 jp

    protected:
      crd_component_ptr endpoint1;
      unsigned short endpoint1_port;
      unsigned short endpoint1_rport;
      unsigned int endpoint1_cap;
      crd_component_ptr endpoint2;
      unsigned short endpoint2_port;
      unsigned short endpoint2_rport;
      unsigned int endpoint2_cap;
      component comp;

      std::list<switch_port> switch_ports;

      pthread_mutex_t state_lock;
      std::string state;
      bool needs_refresh;

      session_ptr sess;

      session_add_link_req* linkreq;
      pthread_mutex_t done_lock;
      int num_comps_done;

      switch_vlan link_vlan;
      bool alloc_vlan;

      bool mark; 

      void cleanup_reqs();
      bool failed;
  }; //class crd_link

  typedef boost::shared_ptr<crd_link> crd_link_ptr;

  void* crd_link_do_initialize_wrapper(void* obj);
  void* crd_link_do_refresh_wrapper(void* obj);
};

#endif // _ONLCRD_COMPONENT_H
