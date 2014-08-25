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

#include "vmd_session.h"
#include "vmd_configuration.h"
#include "vmd_globals.h"
#include "vmd_requests.h"

using namespace vmd;


start_experiment_req::start_experiment_req(uint8_t *mbuf, uint32_t size) : 
  start_experiment(mbuf, size)
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
  
  static int count = 0;
  if (count == 0)
  {
    global_session->setExpInfo(exp.getExpInfo());
  }
  ++count;

  if (global_session)
  {
    std::list<param>::iterator param_itr;
    param_itr = (++init_params.begin());
    std::string vm_pwd = param_itr->getString();
    ++param_itr;
    std::string vm_name = param_itr->getString();
    std::string ip_str = ipaddr.getString();

    if (!global_session->addVM(comp, ip_str, cores, memory, vm_pwd, vm_name))
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

  crd_response* resp = new crd_response(this, status);
  resp->send();
  delete resp;

  return true;
}

configure_node_req::configure_node_req(uint8_t *mbuf, uint32_t size): 
  configure_node(mbuf, size)
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

  if (global_session)
  {
    if (!global_session->configureVM(comp, node_conf))
    {
      status = NCCP_Status_Failed;
      write_log("configure_node_req::handle() failed to configure vm");
    }
  }
  else
  {
    status = NCCP_Status_Failed;
    write_log("configure_node_req::handle() could not get global session pointer");
  }

  vm_ptr vm = global_session->getVM(comp);
  if (!vm)
  {
    status = NCCP_Status_Failed;
    write_log("configure_node_req::handle() failed to get the VM from its comp");
  }

  crd_response* resp = new crd_response(this, status);
  resp->send();
  delete resp;

  return true;
}

end_configure_node_req::end_configure_node_req(uint8_t *mbuf, uint32_t size) : 
  end_configure_node(mbuf, size)
{
}

end_configure_node_req::~end_configure_node_req()
{
}

bool
end_configure_node_req::handle()
{
  write_log("end_configure_node_req::handle()");

  NCCP_StatusType status = NCCP_Status_Fine;
  std::string vm_name="";
  if (global_session)
  {
    vm_ptr vmp = global_session->getVM(comp);
    if (vmp)
    {
      vm_name = vmp->name;
      if (!global_session->startVM(vmp))
      {
        status = NCCP_Status_Failed;
        write_log("end_configure_node_req::handle() failed to start vm");
	global_session->removeVM(vmp);//jp added
      }
    }
    else
    {
      status = NCCP_Status_Failed;
      write_log("end_configure_node_req::handle() could not get vm pointer");
    }
  }
  else
  {
    status = NCCP_Status_Failed;
    write_log("end_configure_node_req::handle() could not get global session pointer");
  }

  //response fills in vm name
  crd_endconfig_response* resp = new crd_endconfig_response(this, status, vm_name);
  resp->send();
  delete resp;

  return true;
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
  write_log("refresh_req::handle() about to send Fine response removing vm");
  NCCP_StatusType status = NCCP_Status_Fine;
  std::string vm_name="";

  if (global_session)
  {
    vm_ptr vmp = global_session->getVM(comp);
    if (vmp)
    {
      vm_name = vmp->name;
      if (!global_session->removeVM(vmp))
      {
        status = NCCP_Status_Failed;
        write_log("refresh_req::handle() failed to remove vm");
      }
    }
  }
  else
  {
    status = NCCP_Status_Failed;
    write_log("refresh_req::handle() could not get global session pointer");
  }

  //response fills in vm name
  crd_endconfig_response* resp = new crd_endconfig_response(this, status, vm_name);
  resp->send();
  delete resp;

  if (global_session->empty())
  {
    write_log("refresh_req::handle() no state left, last refresh req, exiting...");
    sleep(1);
    exit(0);
  }

  return true;
}

end_experiment_req::end_experiment_req(uint8_t *mbuf, uint32_t size): 
  end_experiment(mbuf, size)
{
  write_log("end_experiment_req::end_experiment_req: got end message");
}

end_experiment_req::~end_experiment_req()
{
}

bool
end_experiment_req::handle()
{
  write_log("end_experiment_req::handle() about to clean up everything");
  NCCP_StatusType status = NCCP_Status_Fine;

  if (global_session)
  {
    global_session->clear();
    if (!global_session->empty())
    {
      status = NCCP_Status_Failed;
      write_log("session_manager::clear() failed to remove all vms");
    }
  }

  crd_response* resp = new crd_response(this, status);
  resp->send();
  delete resp;

  sleep(1); // make sure response is sent before exit
  exit(0);

  return true;
}

