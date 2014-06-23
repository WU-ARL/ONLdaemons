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

session_manager::session_manager() throw()
{
  //int id = 1;

  char* myhostname = new char[64+1];
  gethostname(myhostname, 64);
  std::string id((myhostname+8), 2);
  delete[] myhostname;
  //initialize vm names
  int i = 0;
 
  for (i = 1; i < 25; ++i)
    {
      if (i < 10)
	{
	  std::string str = "vm12c" + id + "v0" + int2str(i);
	  vmnames[str] = false;
	}
      else
	{
	  std::string str = "vm12c" + id + "v" + int2str(i);
	  vmnames[str] = false;
	}
    }
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
}

session_ptr 
session_manager::getSession(experiment_info& einfo)
{
  std::list<session_ptr>::iterator sit;
  session_ptr  no_ptr;
  for (sit = active_sessions.begin(); sit != active_sessions.end(); ++sit)
    {
      if ((*sit)->expInfo == einfo) return (*sit);
    }
  return no_ptr;
}

bool 
session_manager::startVM(session_ptr sptr, vm_ptr vmp)
{
  write_log("session_manager::startVM experiment:(" + sptr->getExpInfo().getUserName() + ", " + sptr->getExpInfo().getID() + ") component:(" 
	    + vmp->comp.getLabel() + ", " + int2str(vmp->comp.getID()) + ") vm:(" + vmp->name + ",cores" + int2str(vmp->cores) + ",memory" + int2str(vmp->memory) 
	    + ",interfaces" + int2str(vmp->interfaces.size()) + ")");
  
  std::list<vminterface_ptr>::iterator vmi_it;
  for (vmi_it = vmp->interfaces.begin(); vmi_it != vmp->interfaces.end(); ++vmi_it)
    {
      vlan_ptr vlan = sptr->getVLan((*vmi_it)->ninfo.getVLan());
      vlan->interfaces.push_back((*vmi_it));
      write_log("    add interface:" + int2str((*vmi_it)->ninfo.getPort()) + " with ipaddr:" + (*vmi_it)->ninfo.getIPAddr() + " to physical port:" + int2str((*vmi_it)->ninfo.getRealPort()) 
		+  " and vlan:" + int2str((*vmi_it)->ninfo.getVLan()));
    }
  return true;
}
      
bool 
session_manager::assignVM(vm_ptr vmp)//assigns control addr and vm name for vm
{
  std::map<std::string, bool>::iterator vnmit;
  for (vnmit = vmnames.begin(); vnmit != vmnames.end(); ++vnmit)
    {
       if (!vnmit->second)
	{
	  vmp->name = vnmit->first;
	  return true;
	}
    }
  vmp->name = "";
  return false;
}

bool 
session_manager::deleteVM(component& c, experiment_info& einfo)
{
  session_ptr sptr = getSession(einfo);
  if (!sptr) 
    {
      write_log("session_manager::deleteVM session (" + einfo.getUserName() + ", " + einfo.getID() + ") session already gone");
      return true;
    }
  if (sptr->removeVM(c))
    {
      write_log("session_manager::deleteVM session (" + einfo.getUserName() + ", " + einfo.getID() + ") succeeded to remove VM " + c.getLabel());
      if (sptr->vms.empty())
	{
	  sptr->clear();
	  active_sessions.remove(sptr);
	}
      return true;
    }
  else
    {
      write_log("session_manager::deleteVM session (" + einfo.getUserName() + ", " + einfo.getID() + ") failed to remove VM " + c.getLabel());
      return false;
    }
}

bool
session_manager::releaseVMname(std::string nm)
{
  //should check if name exists
  vmnames[nm] = false;
  return true;
}
