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
#include <fstream>
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

session_manager::session_manager() throw()
{
  //int id = 1;

  pthread_mutex_init(&devname_lock, NULL);
  pthread_mutex_init(&session_lock, NULL);
  char* myhostname = new char[64+1];
  gethostname(myhostname, 64);
  std::string id((myhostname+8), 2);
  delete[] myhostname;
  //initialize dev names
  int i = 0;
  /*NOT NEEDED*/ 
  for (i = 1; i < 25; ++i)
    {
      if (i < 10)
	{
	  std::string str = "dev12c" + id + "v0" + int2str(i);
	  devnames[str].in_use = false;
	  devnames[str].name = str;
	  devnames[str].index = i;
	}
      else
	{
	  std::string str = "dev12c" + id + "v" + int2str(i);
	  devnames[str].in_use = false;
	  devnames[str].name = str;
	  devnames[str].index = i;
	}
    }
  /*END NOT NEEDED*/
}

session_manager::~session_manager()
{
  //need to clear any existing sessions
  std::list<session_ptr>::iterator sit;
  while (!active_sessions.empty())
    {
      active_sessions.front()->clear();
      active_sessions.pop_front();
    }
  pthread_mutex_destroy(&devname_lock);

  pthread_mutex_destroy(&session_lock);
}

session_ptr 
session_manager::getSession(experiment_info& einfo)
{
  std::list<session_ptr>::iterator sit;
  session_ptr  no_ptr;
  autoLockDebug slock(session_lock, "session_manager::getSession(): session_lock");
  for (sit = active_sessions.begin(); sit != active_sessions.end(); ++sit)
    {
      if ((*sit)->expInfo == einfo) return (*sit);
    }
  return no_ptr;
}

session_ptr 
session_manager::addSession(experiment_info& einfo)
{
  session_ptr sptr = getSession(einfo);
  if (!sptr)
    {
      autoLockDebug slock(session_lock, "session_manager::addSession(): session_lock");
      session_ptr tmp_ptr(new session(einfo));
      sptr = tmp_ptr;
      active_sessions.push_back(sptr);
    }
  return sptr;
}

bool //this is where we do the actual setting up of interfaces and vlan stuff
session_manager::startDev(session_ptr sptr, dev_ptr devp)
{
  write_log("session_manager::startDev experiment:(" + sptr->getExpInfo().getUserName() + ", " + sptr->getExpInfo().getID() + ") component:(" 
	    + devp->comp.getLabel() + ", " + int2str(devp->comp.getID()) + ") dev:(" + devp->name + ",cores" + int2str(devp->cores) + ",memory" + int2str(devp->memory) 
	    + ",interfaces" + int2str(devp->interfaces.size()) + ")");
  /*
  std::list<devinterface_ptr>::iterator devi_it;
  //write vlans to file /KVM_Images/scripts/lists/<devp->name>_vlan.txt
  std::string vnm_str = "/KVM_Images/scripts/lists/" + devp->name + "_vlan.txt";
  std::ofstream vlan_fs(vnm_str.c_str(), std::ofstream::out|std::ofstream::trunc);
  //write data_ips to file /KVM_Images/scripts/lists/<devp->name>_dataip.txt
  std::string dipnm_str = "/KVM_Images/scripts/lists/" + devp->name + "_dataip.txt";
  std::ofstream dip_fs(dipnm_str.c_str(), std::ofstream::out|std::ofstream::trunc);
  //std::string cmd = "/KVM_Images/scripts/setup_networking.sh";
  //write_log("session_manager::startDev: system(" + cmd + ")");
  //if(system(cmd.c_str()) != 0)
  //{
  //  write_log("session_manager::startDev: setup_networking.sh failed");
  //  return false;
  //}
  for (devi_it = devp->interfaces.begin(); devi_it != devp->interfaces.end(); ++devi_it)
    {
      vlan_ptr vlan = sptr->getVLan((*devi_it)->ninfo.getVLan());
      vlan->interfaces.push_back((*devi_it));
      write_log("    add interface:" + int2str((*devi_it)->ninfo.getPort()) + " with ipaddr:" + (*devi_it)->ninfo.getIPAddr() + " to physical port:" + int2str((*devi_it)->ninfo.getRealPort()) 
		+  " and vlan:" + int2str((*devi_it)->ninfo.getVLan()));
      //cmd = "/KVM_Images/scripts/setup_vlan.sh " + int2str((*devi_it)->ninfo.getVLan()) + " " + int2str((*devi_it)->ninfo.getRealPort());
      //write_log("session_manager::startDev: system(" + cmd + ")");
      //if(system(cmd.c_str()) != 0)
      //{
      //  write_log("session_manager::startDev: setup_vlan.sh failed");
      //  return false;
      //}
      dip_fs << (*devi_it)->ninfo.getIPAddr() << std::endl;
      vlan_fs << (*devi_it)->ninfo.getVLan() << " " << (*devi_it)->ninfo.getRealPort() << std::endl;
    }
  dip_fs.close();
  vlan_fs.close();
  //int dev_ndx = getDevIndex(devp->name);
  //run start Dev script
  //std::string cmd = "/KVM_Images/scripts/start_new_dev.sh " + sptr->getExpInfo().getUserName() + " /KVM_Images/img/ubuntu_12.04_template.img " + int2str(devp->cores) + " " + int2str(devp->memory) + " " + int2str(devp->interfaces.size()) + " " + vnm_str + " " + dipnm_str + " " + devp->name + " " + devp->passwd;
  std::string cmd = "/KVM_Images/scripts/start_new_dev.sh " + sptr->getExpInfo().getUserName() + " /KVM_Images/img/" +  devp->img + " "  + int2str(devp->cores) + " " + int2str(devp->memory) + " " + int2str(devp->interfaces.size()) + " " + vnm_str + " " + dipnm_str + " " + devp->name + " " + devp->passwd;

  write_log("session_manager::startDev: system(" + cmd + ")");
  if(system(cmd.c_str()) != 0)
  {
    write_log("session_manager::startDev: start script failed");
    //may need to clean up something here
    return false;
  }
  //configure ports for bandwidth restriction
  for (devi_it = devp->interfaces.begin(); devi_it != devp->interfaces.end(); ++devi_it)
    {
      uint32_t bw_kbits = (*devi_it)->ninfo.getBandwidth() * 1000;//rates are given in Mbits/s need to convert to kbits/s
      //#define IFACE_DEFAULT_MIN 1000 //is 1000 arg given to script
      cmd = "/KVM_Images/scripts/dev_configure_port.sh " + int2str((*devi_it)->ninfo.getPort()) + " data" + int2str((*devi_it)->ninfo.getRealPort()) + " " + 
	int2str((*devi_it)->ninfo.getVLan()) + " " + (*devi_it)->ninfo.getIPAddr() + " " + (*devi_it)->ninfo.getSubnet() + " " + int2str(bw_kbits) + " 1000 "  + 
	int2str((*devi_it)->ninfo.getVLan()) + " " + devp->name ;
      write_log("session_manager::startDev: system(" + cmd + ")");
      if(system(cmd.c_str()) != 0)
      {
        write_log("session_manager::startDev: dev_configure_port.sh script failed");
      }
    }

  //make sure we can ping the new dev
  cmd = "ping -c 1 " + devp->name;
  for (int i = 0; i < 15 ; ++i)
    {
      sleep(20);
      if (system(cmd.c_str()) == 0)
	{
	  write_log("session_manager::startDev: system(" + cmd + ") succeeded");
	  return true;
	}
      else
	write_log("session_manager::startDev: system(" + cmd + ") failed");
    }
  return false;
  */
  return true;
}

int
session_manager::getDevIndex(std::string devnm)
{
  /*
  autoLockDebug nmlock(devname_lock, "session_manager::getDevIndex(): devname_lock");
  return (devnames[devnm].index);
  */
  return 1;
}

//Here's where we do any device wise assigning, vpn ip addr, but this is just state actuall configuration
//is done later
bool 
session_manager::assignDev(dev_ptr devp)//assigns control addr and dev name for dev
{
  return true;
  /*
  autoLockDebug nmlock(devname_lock, "session_manager::assignDev(): devname_lock");
  std::map<std::string, devname_info>::iterator vnmit;
  for (vnmit = devnames.begin(); vnmit != devnames.end(); ++vnmit)
    {
       if (!vnmit->second.in_use)
	{
	  devp->name = vnmit->first;
	  vnmit->second.in_use = true;
	  return true;
	}
    }
  devp->name = "";
  return false;
  */
}

bool //this should initiate any cleanup code should reside in session::removeDev
session_manager::deleteDev(component& c, experiment_info& einfo)
{
  session_ptr sptr = getSession(einfo);
  if (!sptr) 
    {
      write_log("session_manager::deleteDev session (" + einfo.getUserName() + ", " + einfo.getID() + ") session already gone");
      return true;
    }
  autoLockDebug slock(session_lock, "session_manager::deleteDev(): session_lock");
  if (sptr->removeDev(c))
    {
      write_log("session_manager::deleteDev session (" + einfo.getUserName() + ", " + einfo.getID() + ") succeeded to remove Dev " + c.getLabel());
      if (sptr->devs.empty())
	{
	  sptr->clear();
	  active_sessions.remove(sptr);
	}
      return true;
    }
  else
    {
      write_log("session_manager::deleteDev session (" + einfo.getUserName() + ", " + einfo.getID() + ") failed to remove Dev " + c.getLabel());
      return false;
    }
}

bool
session_manager::releaseDevName(std::string nm)
{
  autoLockDebug nmlock(devname_lock, "session_manager::releaseDevName(): devname_lock");
  //should check if name exists
  devnames[nm].in_use = false;
  return true;
}
