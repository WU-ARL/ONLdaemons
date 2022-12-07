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

#include "onlvpnbased_session.h"
#include "onlvpnbased_session_manager.h"
#include "onlvpnbased_globals.h"

using namespace onlvpnbased;


devinterface_ptr
getInterface(dev_ptr dev, node_info& ninfo)
{
  std::list<devinterface_ptr>::iterator deviit;
  devinterface_ptr noptr;
  for (deviit = dev->interfaces.begin(); deviit != dev->interfaces.end(); ++deviit)
    {
      if ((*deviit)->ninfo.getPort() == ninfo.getPort()) return (*deviit);
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
     

dev_ptr
session::addDev(component& c, std::string eaddr, uint32_t crs, uint32_t mem)
{
  dev_ptr rtn_dev = getDev(c);
  if (!rtn_dev)
    {
      dev_ptr dev(new dev_info());
      dev->comp = c;
      dev->expaddr = eaddr;
      dev->cores = crs;
      int tmp_mem = (int)(mem/1000) * 1024;
      dev->memory = tmp_mem;
      if (the_session_manager->assignDev(dev)) //assigns control addr and dev name for dev
	{
	  rtn_dev = dev;
	  devs.push_back(dev);
	}
    }
  return rtn_dev;
}
      

bool 
session::removeDev(component& c)
{
  dev_ptr devp = getDev(c);
  return (removeDev(devp));
}
      

bool //cleanup code for removing device from experiment
session::removeDev(dev_ptr devp)
{
  if (devp)
    {
      write_log("session::removeDev experiment:(" + expInfo.getUserName() + ", " + expInfo.getID() + ") component:(" 
		+ devp->comp.getLabel() + ", " + int2str(devp->comp.getID()) + ") dev:(" + devp->name + ",cores" + int2str(devp->cores) + ",memory" + int2str(devp->memory) 
		+ ",interfaces" + int2str(devp->interfaces.size()) + ")");
      /*
      std::list<devinterface_ptr>::iterator devi_it;
      std::string cmd;
      //remove from bridges
      for (devi_it = devp->interfaces.begin(); devi_it != devp->interfaces.end(); ++devi_it)
	{
	  vlan_ptr vlan = getVLan((*devi_it)->ninfo.getVLan());
	  vlan->interfaces.remove((*devi_it));
	  write_log("    remove interface:" + int2str((*devi_it)->ninfo.getPort()) + " with ipaddr:" + (*devi_it)->ninfo.getIPAddr() + " to physical port:" + int2str((*devi_it)->ninfo.getRealPort()) 
		    +  " and vlan:" + int2str((*devi_it)->ninfo.getVLan()));
	  if (vlan->interfaces.empty())
	    {
	      cmd = "/KVM_Images/scripts/cleanup_vlan.sh " + int2str(vlan->id);
	      if(system(cmd.c_str()) != 0)
		{
		  write_log("session::removeDev: cleanup_vlan script failed");
		  //may need to clean up something here
		}
	      write_log("     remove vlan:" + int2str(vlan->id));
	      vlans.remove(vlan);
	    }
	}
      //kill dev and release name for reuse
      //run kill script
      cmd = "/KVM_Images/scripts/undefine_dev.sh " + devp->name;
      write_log("session::removeDev: system(" + cmd + ")");
      if(system(cmd.c_str()) != 0)
      {
	write_log("session::removeDev: undefine script failed");
	//may need to clean up something here
	return false;
      }
      
      if (!the_session_manager->releaseDevName(devp->name))
	{
	  write_log("session::removeDev failed dev " + devp->name + " comp " + int2str(devp->comp.getID()) );
	  return false;
	}
      */
      write_log("session::removeDev dev " + devp->name + " comp " + int2str(devp->comp.getID()) );
      devs.remove(devp);
      
    }
  return true;
}

bool //This is when we get the vlan info
session::configureDev(component& c, node_info& ninfo)
{
  dev_ptr dev = getDev(c);

  write_log("session::configureDev interface " + int2str(ninfo.getPort()) + " with ipaddr:" + ninfo.getIPAddr() + " to physical port:" + int2str(ninfo.getRealPort()) + " vlan:" + int2str(ninfo.getVLan()));

  if (!dev)
    {
      write_log("session::configureDev failed to find dev for comp " + int2str(c.getID()) );
      return false;
    }
  devinterface_ptr tmp_devi = getInterface(dev, ninfo);
  if (!tmp_devi)
    {
      devinterface_ptr devi(new devinterface_info());
      devi->ninfo = ninfo;
      devi->dev = dev;
      dev->interfaces.push_back(devi);
    }
  return true;
}
      
dev_ptr 
session::getDev(component& c)
{
  std::list<dev_ptr>::iterator devit;
  
  dev_ptr noptr;
  for (devit = devs.begin(); devit != devs.end(); ++devit)
    {
      if ((*devit)->comp.getID() == c.getID()) 
	return (*devit);
    }
  return noptr;
}
      
//bool addToVlan(uint32_t vlan, component& c);
void 
session::clear()
{
  //remove devs 
  while (!devs.empty())
    {
      removeDev(devs.front());//this is only called from the session manager so it should get removed from the active_sessions
      devs.pop_front();
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
