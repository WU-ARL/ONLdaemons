/*
 * Copyright (c) 2014 Jyoti Parwatikar 
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

#ifndef _VMD_SESSION_H
#define _VMD_SESSION_H

/* For the first pass, test with print statements and check log files 
 * to make sure everything syncs up. Also, take regular base daemon
 * as a template and change it a little bit to only spawn one of my 
 * vmd's. Put something to give the names in the base daemon to the
 * specialization daemon.
 *
 * For the base daemon, make sure to only spawn one per experiment
 * and also put my messaging scheme (for the names) in.
 */

namespace vmd
{
  struct _vminterface_info;

  class vm_node 
  {
  public:
    vm_node() {}
    ~vm_node() {}

    bool add_iface( boost::shared_ptr<_vminterface_info> iface);
    
    component get_comp() { return comp; }
    std::string get_name() { return name; }
    std::string get_ctladdr() { return ctladdr; }
    std::string get_expaddr() { return expaddr; }
    uint32_t get_cores(){ return cores; }
    uint32_t get_memory() { return memory; }

    void set_comp(component& c){ comp = c; }
    void set_name(std::string n){ name = n; }
    void set_ctladdr(std::string ctl) { ctladdr = ctl; }
    void set_expaddr(std::string exp){ expaddr = exp; }
    void set_cores(uint32_t crs){ cores = crs; }
    void set_memory(uint32_t mem){ memory = mem; }

  private:
    std::string name;
    std::string ctladdr;
    component comp;
    std::list< boost::shared_ptr<_vminterface_info> > interfaces;
    std::string expaddr;
    uint32_t cores;
    uint32_t memory;
  };

  typedef boost::shared_ptr<vm_node> vm_ptr;

  struct _vlan_info;

  typedef struct _vminterface_info
  {
    node_info ninfo;
    boost::shared_ptr<_vlan_info> vlan;
    vm_ptr vm;
  } vminterface_info;

  typedef boost::shared_ptr<vminterface_info> vminterface_ptr;

  typedef struct _vlan_info
  {
    uint32_t id;
    uint32_t bridge_id;
    std::list<vminterface_ptr> interfaces;
  } vlan_info;

  typedef boost::shared_ptr<vlan_info> vlan_ptr;

  class session_manager
  {
  public:
    session_manager();
    session_manager(experiment_info& ei) throw(std::runtime_error);
    ~session_manager();   
    
    experiment_info& getExpInfo() { return expInfo;}
    vm_ptr addVM(component& c, std::string eaddr, uint32_t crs, 
                 uint32_t mem, std::string pwd, std::string nm);
    bool removeVM(vm_ptr vmp);
    
    bool configureVM(component& c, node_info& ninfo);
    vm_ptr getVM(component& c);
    
    bool startVM(vm_ptr vm);
    
    //bool addToVlan(uint32_t vlan, component& c);
    void clear();
    vlan_ptr getVLan(uint32_t vlan);//adds vlan if not already there
    
  private:
    experiment_info expInfo;
    std::list<vm_ptr> vms;
    std::list<vlan_ptr> vlans;
  };

  typedef boost::shared_ptr<session_manager> session_ptr;
};

#endif // _VMD_SESSION_H
