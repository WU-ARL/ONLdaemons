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

#include "shared.h"

#include "vmd_configuration.h"
#include "vmd_globals.h"
#include "vmd_requests.h"
#include "vmd_session.h"

using namespace vmd;

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
  session_ptr sess_ptr = the_session_manager->getSession(exp.getExpInfo());

  if (sess_ptr)
	{
		// ard: need to do the proper configuration information here.

		/*
		if (!sess_ptr->configureVM(comp, node_conf))
		{
			status = NCCP_Status_Failed;
			write_log("configure_node_req::handle() failed to configure vm");
		}
		*/
	}
  else
	{
		status = NCCP_Status_Failed;
		write_log("configure_node_req::handle() failed to get session pointer");
	}

  //ard: also probably need to store this somewhere globally
  // equal operator for node info defined
	
	/* Where does the experiment info come from? Couldn't find it in 
	 * session_ptr or experiment_info...
	 */

  crd_response* resp = new crd_response(this, status);
  resp->send();
  delete resp;

  return true;
}

end_configure_node_req::end_configure_node_req(uint8_t *mbuf, uint32_t size) : 
  rli_request(mbuf, size)
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
    //ard: start vm here, not sure if we'll actually use session manager 
    //or just plug in Jason's scripts right here.
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

start_vm_req::start_vm_req(uint8_t *mbuf, uint32_t size) : 
  rli_request(mbuf, size)
{
}

start_vm_req::~start_vm_req()
{
}

bool
start_vm_req::handle()
{
  write_log("start_vm_req::handle() + name:" + name);

  crd_response* resp = new crd_response(this, NCCP_Status_Fine);
  resp->send();
  delete resp;

  return true;
}



/*
add_route_req::add_route_req(uint8_t *mbuf, uint32_t size): rli_request(mbuf, size)
{
}

add_route_req::~add_route_req()
{
}

bool
add_route_req::handle()
{
  write_log("add_route_req::handle()");

  std::string prefixstr = conf->addr_int2str(prefix);
  if(prefixstr == "")
  {
    write_log("add_route_req: handle(): got bad prefix");
    rli_response* rliresp = new rli_response(this, NCCP_Status_Failed);
    rliresp->send();
    delete rliresp;
    return true;
  }
  
  std::string nhstr = conf->addr_int2str(nexthop_ip);
  if(nhstr == "")
  {
    write_log("add_route_req: handle(): got bad next hop");
    rli_response* rliresp = new rli_response(this, NCCP_Status_Failed);
    rliresp->send();
    delete rliresp;
    return true;
  }
  
  NCCP_StatusType stat = NCCP_Status_Fine;
  if(!conf->add_route(port, prefixstr, mask, nhstr))
  {
    stat = NCCP_Status_Failed;
  }

  rli_response* rliresp = new rli_response(this, stat);
  rliresp->send();
  delete rliresp;

  return true;
}

void
add_route_req::parse()
{
  rli_request::parse();

  prefix = params[0].getInt();
  mask = params[1].getInt();
  output_port = params[2].getInt();
  nexthop_ip = params[3].getInt();
  stats_index = params[4].getInt();
}

delete_route_req::delete_route_req(uint8_t *mbuf, uint32_t size): rli_request(mbuf, size)
{
}

delete_route_req::~delete_route_req()
{
}

bool
delete_route_req::handle()
{
  write_log("delete_route_req::handle()");

  std::string prefixstr = conf->addr_int2str(prefix);
  if(prefixstr == "")
  {
    write_log("delete_route_req: handle(): got bad prefix");
    rli_response* rliresp = new rli_response(this, NCCP_Status_Failed);
    rliresp->send();
    delete rliresp;
    return true;
  }
  
  NCCP_StatusType stat = NCCP_Status_Fine;
  if(!conf->delete_route(prefixstr, mask))
  {
    stat = NCCP_Status_Failed;
  }

  rli_response* rliresp = new rli_response(this, stat);
  rliresp->send();
  delete rliresp;

  return true;
}

void
delete_route_req::parse()
{
  rli_request::parse();

  prefix = params[0].getInt();
  mask = params[1].getInt();
}
*/

