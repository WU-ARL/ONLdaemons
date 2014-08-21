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
#include <fstream>
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

#include "vmd_session.h"
#include "vmd_configuration.h"
#include "vmd_globals.h"

using namespace vmd;

session_manager::session_manager()
{
  write_log("session_manager::session_manager: added empty session_manager");
}
session_manager::session_manager(experiment_info& ei) throw(std::runtime_error)
{
  expInfo = ei;
  write_log("session_manager::session_manager: added session_manager " + 
            ei.getID() + " for user " + ei.getUserName());
}

session_manager::~session_manager()
{
  write_log("session_manager::session_manager: deleting session_manager " + 
            expInfo.getID() + " for user " + expInfo.getUserName());
  clear();
}

bool 
session_manager::startVM(vm_ptr vmp)
{
  write_log("session_manager::startVM experiment:(" + expInfo.getUserName() 
            + ", " + expInfo.getID() + ") component:(" 
            + vmp->comp.getLabel() + ", " + int2str(vmp->comp.getID()) 
            + ") vm:(" + vmp->name + ",cores" + int2str(vmp->cores) + ",memory" 
            + int2str(vmp->memory) + ",interfaces" + 
            int2str(vmp->interfaces.size()) + ")");
  
  
  std::list<vminterface_ptr>::iterator vmi_it;

  //write vlans to file 
  std::string vnm_str = "/KVM_Images/scripts/lists/" + vmp->name + "_vlan.txt";
  std::ofstream vlan_fs(vnm_str.c_str(), std::ofstream::out|std::ofstream::trunc);

  //write data_ips to file
  std::string dipnm_str = "/KVM_Images/scripts/lists/" + vmp->name + "_dataip.txt";
  std::ofstream dip_fs(dipnm_str.c_str(), std::ofstream::out|std::ofstream::trunc);

  for (vmi_it = vmp->interfaces.begin(); vmi_it != vmp->interfaces.end(); ++vmi_it)
  {
    vlan_ptr vlan = getVLan((*vmi_it)->ninfo.getVLan());
    vlan->interfaces.push_back((*vmi_it));

    write_log("    add interface:" + int2str((*vmi_it)->ninfo.getPort()) 
              + " with ipaddr:" + (*vmi_it)->ninfo.getIPAddr() + 
              " to physical port:" + int2str((*vmi_it)->ninfo.getRealPort()) 
              +  " and vlan:" + int2str((*vmi_it)->ninfo.getVLan()));

    dip_fs << (*vmi_it)->ninfo.getIPAddr() << std::endl;
    vlan_fs << (*vmi_it)->ninfo.getVLan() << " " << 
      (*vmi_it)->ninfo.getRealPort() << std::endl;
  }

  dip_fs.close();
  vlan_fs.close();

  //run start VM script
  std::string cmd = "/KVM_Images/scripts/start_new_vm.sh " + 
    expInfo.getUserName() + " /KVM_Images/img/ubuntu_12.04_template.img " +
    int2str(vmp->cores) + " " + int2str(vmp->memory) + " " + 
    int2str(vmp->interfaces.size()) + " " + vnm_str + " " + dipnm_str + " " + 
    vmp->name + " " + vmp->passwd;
  write_log("session_manager::startVM: system(" + cmd + ")");

  /*
  if(system_cmd(cmd) != 0)
  {
    write_log("session_manager::startVM: start script failed");
    //may need to clean up something here
    return false;
  }


  cmd = "ping -c 1 " + vmp->name;
  for (int i = 0; i < 10; ++i)
  {
    if (system_cmd(cmd) == 0)
    {
      write_log("session_manager::startVM: vm is successfuly up");
      return true;
    }
    sleep(15);
  }
  */

  write_log("session_manager::startVM: vm did not start after 150 seconds");  
  return false;
}

vminterface_ptr
getInterface(vm_ptr vm, node_info& ninfo)
{
  std::list<vminterface_ptr>::iterator vmiit;
  vminterface_ptr noptr;
  for (vmiit = vm->interfaces.begin(); 
       vmiit != vm->interfaces.end(); 
       ++vmiit)
  {
    if ((*vmiit)->ninfo.getPort() == ninfo.getPort()) return (*vmiit);
  }
  return noptr;
}     

vm_ptr
session_manager::addVM(component& c, std::string eaddr, 
                       uint32_t crs, uint32_t mem,
                       std::string pwd, std::string nm)
{
  vm_ptr rtn_vm = getVM(c);
  if (!rtn_vm)
  {
    vm_ptr vm(new vm_node());
    vm->comp = c;
    vm->expaddr = eaddr;
    vm->cores = crs;
    vm->passwd = pwd;
    vm->name = nm;

    int tmp_mem = (int)(mem/1000) * 1024;
    vm->memory = tmp_mem;

    rtn_vm = vm;
    vms.push_back(vm);
  }
  return rtn_vm;
}
      
bool 
session_manager::removeVM(component& c)
{
  vm_ptr vmp = getVM(c);
  return (removeVM(vmp));
}

bool 
session_manager::removeVM(vm_ptr vmp)
{
  if (vmp)
  {
    write_log("session_manager::removeVM experiment:(" + expInfo.getUserName() + 
              ", " + expInfo.getID() + ") component:(" + vmp->comp.getLabel() + 
              ", " + int2str(vmp->comp.getID()) + ") vm:(" + vmp->name + 
              ",cores" + int2str(vmp->cores) + ",memory" + int2str(vmp->memory) + 
              ",interfaces" + int2str(vmp->interfaces.size()) + ")");
      
    std::string cmd;
    std::list<vminterface_ptr>::iterator vmi_it;
    //remove from bridges
    for (vmi_it = vmp->interfaces.begin(); 
         vmi_it != vmp->interfaces.end(); 
         ++vmi_it)
    {
      vlan_ptr vlan = getVLan((*vmi_it)->ninfo.getVLan());
      vlan->interfaces.remove((*vmi_it));

      write_log("\tremove interface:" + int2str((*vmi_it)->ninfo.getPort()) + 
                " with ipaddr:" + (*vmi_it)->ninfo.getIPAddr() + 
                " to physical port:" + int2str((*vmi_it)->ninfo.getRealPort()) + 
                " and vlan:" + int2str((*vmi_it)->ninfo.getVLan()));

      if (vlan->interfaces.empty())
      {
        // clean up vlans
        cmd = "/KVM_Images/scripts/cleanup_vlan.sh " + int2str(vlan->id);
        /*
        if(system_cmd(cmd) != 0) {
          write_log("session::removeVM: cleanup_vlan script failed");
        }
        */
        write_log("\tremove vlan:" + int2str(vlan->id));
        vlans.remove(vlan);
      }
    }

    //kill vm 
    cmd = "/KVM_Images/scripts/undefine_vm.sh " + vmp->name;
    write_log("session_manager::removeVM: system(" + cmd + ")");

    /*
    if(system_cmd(cmd) != 0)
    {
      write_log("session_manager::removeVM: undefine_vm script failed");
      return false;
    }
    */

    write_log("session_manager::removeVM vm " + vmp->name + " comp " + 
              int2str(vmp->comp.getID()) );
    vms.remove(vmp);
  }
  return true;
}

bool 
session_manager::configureVM(component& c, node_info& ninfo)
{
  vm_ptr vm = getVM(c);
  if (!vm)
  {
    write_log("session_manager::configureVM failed to find vm for comp " + 
              int2str(c.getID()) );
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
session_manager::getVM(component& c)
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

/* this is only called from the session manager so it should get 
 * removed from the active_sessions
 */
void 
session_manager::clear()
{
  while (!vms.empty())
  { 
    removeVM(vms.front());
    vms.pop_front();
  }
}

/* adds vlan if not already there */
vlan_ptr 
session_manager::getVLan(uint32_t vlan)
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

int
session_manager::system_cmd(std::string cmd)
{
  int rtn = system(cmd.c_str());
  if(rtn == -1) return rtn;
  return WEXITSTATUS(rtn);
}

