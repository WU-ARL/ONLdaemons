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

#include "host_configuration.h"
#include "host_globals.h"
#include "host_requests.h"

using namespace host;

configure_node_req::configure_node_req(uint8_t *mbuf, uint32_t size): configure_node(mbuf, size)
{
}

configure_node_req::~configure_node_req()
{
}

bool
configure_node_req::handle()
{
  write_log("configure_node_req::handle() + port:" + int2str(node_conf.getPort()) + " vlan:" + int2str(node_conf.getVLan()));

  conf->set_port_info(node_conf.getPort(), node_conf.getIPAddr(), node_conf.getSubnet(), node_conf.getNHIPAddr());

  crd_response* resp = new crd_response(this, NCCP_Status_Fine);
  resp->send();
  delete resp;

  return true;
}

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
