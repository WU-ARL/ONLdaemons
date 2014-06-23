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

#ifndef _ONLVMBASED_SESSION_H
#define _ONLVMBASED_SESSION_H

namespace onlvmbased
{

  struct _vminterface_info;

  typedef struct _vm_info
  { 
    std::string name;
    std::string ctladdr;
    component comp;
    std::list< boost::shared_ptr<_vminterface_info> > interfaces;
    std::string expaddr;
    uint32_t cores;
    uint32_t memory;
  } vm_info;

  typedef boost::shared_ptr<vm_info> vm_ptr;

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

  class session
  {
    friend class session_manager;
    public:
      session(experiment_info& ei) throw(std::runtime_error);
      ~session() throw();   

      experiment_info& getExpInfo() { return expInfo;}
      vm_ptr addVM(component& c, std::string eaddr, uint32_t crs, uint32_t mem);
      bool removeVM(component& c);
      bool removeVM(vm_ptr vmp);

      bool configureVM(component& c, node_info& ninfo);
      vm_ptr getVM(component& c);
      
      //bool addToVlan(uint32_t vlan, component& c);
      void clear();
      vlan_ptr getVLan(uint32_t vlan);//adds vlan if not already there
      
    private:
      experiment_info expInfo;
      std::list<vm_ptr> vms;
      std::list<vlan_ptr> vlans;
  };

  typedef boost::shared_ptr<session> session_ptr;
};

#endif // _ONLVMBASED_SESSION_H
