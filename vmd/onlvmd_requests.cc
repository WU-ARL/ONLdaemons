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
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <netinet/in.h>

#include <boost/shared_ptr.hpp>

#include "shared.h"

#include "onlvmd_session.h"
#include "onlvmd_session_manager.h"
#include "onlvmd_globals.h"
#include "onlvmd_requests.h"

using namespace onlvmd;

start_experiment_req::start_experiment_req(uint8_t *mbuf, uint32_t size): start_experiment(mbuf, size)
{
}

start_experiment_req::~start_experiment_req()
{
}

bool
start_experiment_req::handle()
{
  write_log("start_experiment_req::handle()");
  NCCP_StatusType status = NCCP_Status_Fine;

  session_ptr sess_ptr = the_session_manager->addSession(exp.getExpInfo());
  
  if (sess_ptr)
    {
      std::list<param>::iterator pit;
      pit = (++init_params.begin());
      std::string pwd = pit->getString();
      ++pit;
      std::string nm = pit->getString();
      
      std::string ip_str = ipaddr.getCString();
      if (!sess_ptr->addVM(comp, ip_str, cores, memory, pwd, nm))
	{
	  status = NCCP_Status_Failed;		
	  write_log("start_experiment_req::handle failed to add VM to session");
	}
    }
  else 
    {
      status = NCCP_Status_Failed;		
      write_log("start_experiment_req::handle failed to get session ptr for experiment");
    }
  user = exp.getExpInfo().getUserName();
  
  //std::string cmd = "/bin/mkdir -p /users";
  // write_log("start_experiment_req::handle(): request vm");
  
  
  crd_response* resp = new crd_response(this, status);
  resp->send();
  delete resp;
  
  return true;
}

int
start_experiment_req::system_cmd(std::string cmd)
{
  write_log("start_experiment_req::system_cmd(): cmd = " + cmd);
  int rtn = system(cmd.c_str());
  if(rtn == -1) return rtn;
  return WEXITSTATUS(rtn);
}

refresh_req::refresh_req(uint8_t *mbuf, uint32_t size): refresh(mbuf, size)
{
  write_log("refresh_req::refresh_req: got refresh message");
}

refresh_req::~refresh_req()
{
}

bool
refresh_req::handle()
{
  write_log("refresh_req::handle: about to send Fine response removing vm");
  session_ptr sptr = the_session_manager->getSession(exp.getExpInfo());
  std::string nm = "";
  if (!sptr)
    { 
      write_log("refresh_req::handle failed to get session ptr for experiment");
    }
  else 
    {
      vm_ptr vptr = sptr->getVM(comp);
      if (vptr)
	{
	  nm = vptr->name;
	  if (!the_session_manager->deleteVM(comp, exp.getExpInfo()))
	    {	
	      write_log("refresh_req::handle failed to deleteVM for experiment");
	    }
	}
    }
  
  crd_endconfig_response* resp = new crd_endconfig_response(this, NCCP_Status_Fine, nm);
  resp->send();
  delete resp;
  
  return true;
}

configure_node_req::configure_node_req(uint8_t *mbuf, uint32_t size): configure_node(mbuf, size)
{
}

configure_node_req::~configure_node_req()
{
}

bool
configure_node_req::handle()
{
  write_log("configure_node_req::handle()");
  
  NCCP_StatusType status = NCCP_Status_Fine;
  session_ptr sess_ptr = the_session_manager->getSession(exp.getExpInfo());
  
  if (sess_ptr)
    {
      if (!sess_ptr->configureVM(comp, node_conf))
	{
	  status = NCCP_Status_Failed;
	  write_log("configure_node_req::handle() failed to configure vm");
	}
    }
  else
    {
      status = NCCP_Status_Failed;
      write_log("configure_node_req::handle() failed to get session pointer");
    }
  //conf->set_port_info(node_conf.getPort(), node_conf.getIPAddr(), node_conf.getSubnet(), node_conf.getNHIPAddr());

  crd_response* resp = new crd_response(this, status);
  resp->send();
  delete resp;
  
  return true;
}

end_configure_node_req::end_configure_node_req(uint8_t *mbuf, uint32_t size): end_configure_node(mbuf, size)
{
}

end_configure_node_req::~end_configure_node_req()
{
}

bool
end_configure_node_req::handle()
{
  write_log("end_configure_node_req::handle()");
  
  session_ptr sess_ptr = the_session_manager->getSession(exp.getExpInfo());
  
  NCCP_StatusType status = NCCP_Status_Fine;
  vm_ptr vmp = sess_ptr->getVM(comp);
  if (sess_ptr)
    {
      if (!the_session_manager->startVM(sess_ptr, vmp))
	{
	  status = NCCP_Status_Failed;
	  write_log("end_configure_node_req::handle() failed to start vm");
	}
    }
  else
    {
      status = NCCP_Status_Failed;
      write_log("end_configure_node_req::handle() failed to get session pointer");
    }
  //response fills in vm name
  crd_endconfig_response* resp = new crd_endconfig_response(this, status, vmp->name);
  resp->send();
  delete resp;
  
  return true;
}
