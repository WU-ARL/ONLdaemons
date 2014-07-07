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


#include <iostream>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <list>
#include <vector>
#include <map>
#include <exception>
#include <stdexcept>
#include <pwd.h>

#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <memory.h>
//#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>

#include <boost/shared_ptr.hpp>

#include "shared.h"

#include "onlvmbased_session.h"
#include "onlvmbased_session_manager.h"
#include "onlvmbased_globals.h"

using namespace onlvmbased;


vminterface_ptr
getInterface(vm_ptr vm, node_info& ninfo)
{
  std::list<vminterface_ptr>::iterator vmiit;
  vminterface_ptr noptr;
  for (vmiit = vm->interfaces.begin(); vmiit != vm->interfaces.end(); ++vmiit)
    {
      if ((*vmiit)->ninfo.getPort() == ninfo.getPort()) return (*vmiit);
    }
  return noptr;
}

session::session(experiment_info& ei) throw(std::runtime_error)
{
  expInfo = ei;
  write_log("session::session: added session " + ei.getID() + " for user " + ei.getUserName());
}

session::~session() throw()
{
  write_log("session::session: deleting session " + expInfo.getID() + " for user " + expInfo.getUserName());
  clear();
}
     

vm_ptr
session::addVM(component& c, std::string eaddr, uint32_t crs, uint32_t mem)
{
  vm_ptr rtn_vm = getVM(c);
  if (!rtn_vm)
    {
      vm_ptr vm(new vm_info());
      vm->comp = c;
      vm->expaddr = eaddr;
      vm->cores = crs;
      int tmp_mem = (int)(mem/1000) * 1024;
      vm->memory = tmp_mem;
      if (the_session_manager->assignVM(vm)) //assigns control addr and vm name for vm
	{
	  rtn_vm = vm;
	  vms.push_back(vm);
	}
    }
  return rtn_vm;
}
      

bool 
session::removeVM(component& c)
{
  vm_ptr vmp = getVM(c);
  return (removeVM(vmp));
}
      

bool 
session::removeVM(vm_ptr vmp)
{
  if (vmp)
    {
      write_log("session::removeVM experiment:(" + expInfo.getUserName() + ", " + expInfo.getID() + ") component:(" 
		+ vmp->comp.getLabel() + ", " + int2str(vmp->comp.getID()) + ") vm:(" + vmp->name + ",cores" + int2str(vmp->cores) + ",memory" + int2str(vmp->memory) 
		+ ",interfaces" + int2str(vmp->interfaces.size()) + ")");
      
      std::list<vminterface_ptr>::iterator vmi_it;
      //remove from bridges
      for (vmi_it = vmp->interfaces.begin(); vmi_it != vmp->interfaces.end(); ++vmi_it)
	{
	  vlan_ptr vlan = getVLan((*vmi_it)->ninfo.getVLan());
	  vlan->interfaces.remove((*vmi_it));
	  write_log("    remove interface:" + int2str((*vmi_it)->ninfo.getPort()) + " with ipaddr:" + (*vmi_it)->ninfo.getIPAddr() + " to physical port:" + int2str((*vmi_it)->ninfo.getRealPort()) 
		    +  " and vlan:" + int2str((*vmi_it)->ninfo.getVLan()));
	  if (vlan->interfaces.empty())
	    {
	      write_log("     remove vlan:" + int2str(vlan->id));
	      vlans.remove(vlan);
	    }
	}
      //kill vm and release name for reuse
      //run kill script
      std::string cmd = "/KVM_Images/scripts/undefine_vm.sh " + vmp->name;
      write_log("session::removeVM: system(" + cmd + ")");
      if(system(cmd.c_str()) != 0)
      {
	write_log("session::removeVM: start script failed");
	//may need to clean up something here
	return false;
      }
      
      if (!the_session_manager->releaseVMname(vmp->name))
	{
	  write_log("session::removeVM failed vm " + vmp->name + " comp " + int2str(vmp->comp.getID()) );
	  return false;
	}
      write_log("session::removeVM vm " + vmp->name + " comp " + int2str(vmp->comp.getID()) );
      vms.remove(vmp);
    }
  return true;
}

bool 
session::configureVM(component& c, node_info& ninfo)
{
  vm_ptr vm = getVM(c);
  if (!vm)
    {
      write_log("session::configureVM failed to find vm for comp " + int2str(c.getID()) );
      return false;
    }
  vminterface_ptr tmp_vmi = getInterface(vm, ninfo);
  if (!tmp_vmi)
    {
      vminterface_ptr vmi(new vminterface_info());
      vmi->ninfo = ninfo;
      vmi->vm = vm;
      vm->interfaces.push_back(vmi);
    }
  return true;
}
      
vm_ptr 
session::getVM(component& c)
{
  std::list<vm_ptr>::iterator vmit;
  
  vm_ptr noptr;
  for (vmit = vms.begin(); vmit != vms.end(); ++vmit)
    {
      if ((*vmit)->comp.getID() == c.getID()) 
	return (*vmit);
    }
  return noptr;
}
      
//bool addToVlan(uint32_t vlan, component& c);
void 
session::clear()
{
  //remove vms 
  while (!vms.empty())
    {
      removeVM(vms.front());//this is only called from the session manager so it should get removed from the active_sessions
      vms.pop_front();
    }
  //clear vlans?
}


vlan_ptr 
session::getVLan(uint32_t vlan)//adds vlan if not already there
{
  std::list<vlan_ptr>::iterator vit;
  for (vit = vlans.begin(); vit != vlans.end(); ++vit)
    {
      if ((*vit)->id == vlan) return (*vit);
    }
  vlan_ptr new_vlan(new vlan_info());
  new_vlan->id = vlan;
  new_vlan->bridge_id = vlan;
  vlans.push_back(new_vlan);
  return new_vlan;
}
