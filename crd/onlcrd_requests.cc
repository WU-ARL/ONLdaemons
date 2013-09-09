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
#include <set>
#include <exception>
#include <stdexcept>

#include <boost/shared_ptr.hpp>

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
#include "onl_database.h"

#include "onlcrd_reservation.h"
#include "onlcrd_session.h"
#include "onlcrd_component.h"
#include "onlcrd_session_manager.h"
#include "onlcrd_globals.h"
#include "onlcrd_requests.h"
#include "onlcrd_responses.h"
#include "onlcrd_session_sharing.h"

using namespace onlcrd;

ping_req::ping_req(uint8_t *mbuf, uint32_t size): request(mbuf, size)
{
}

ping_req::~ping_req()
{
}

bool
ping_req::handle()
{
  response *resp = new response(this);
  resp->send();
  delete resp;

  return true;
}

i_am_up_req::i_am_up_req(uint8_t *mbuf, uint32_t size): i_am_up(mbuf, size)
{
}

i_am_up_req::~i_am_up_req()
{
}

bool
i_am_up_req::handle()
{
  the_session_manager->received_up_msg(nccpconn->get_remote_hostname());
  return true;
}

rlicrd_request::rlicrd_request(uint8_t* mbuf, uint32_t size): request(mbuf, size)
{
}

rlicrd_request::~rlicrd_request()
{
}

void
rlicrd_request::parse()
{
  request::parse();

  nccp_string tmp;
  buf >> tmp;
  expinfo.setID(tmp.getString());
  buf >> comp;
}

begin_session_req::begin_session_req(uint8_t *mbuf, uint32_t size): request(mbuf, size)
{
}

begin_session_req::~begin_session_req()
{
  the_dispatcher->clear_periodic_rendezvous(this);
}

bool
begin_session_req::handle()
{
  std::string username = expinfo.getUserName();

  if(version < MIN_RLI_VERSION)
  {
    session_response* resp = new session_response(this, "Your RLI is out of date.  Please go to http://onl.wustl.edu to download a new version.", Session_Version_Err);
    resp->send();
    delete resp;
    return true;
  }

  if (the_session_manager->is_initializing())
  {
    session_response* resp = NULL;
    if (version <= 0x73 )
        resp = new session_response(this, "ONL is currently rebooting. Please try again in 10 minutes.", Session_Alert);
    else
        resp = new session_response(this, "ONL is currently rebooting. Please try again in 10 minutes.", Session_Alloc_Err);
        
    if (resp) resp->send();
    delete resp;
    return true;
  }

  if(!the_session_manager->authenticate(username, password.getString()))
  {
    session_response* resp = new session_response(this, "Authentication failed.", Session_Auth_Err);
    resp->send();
    delete resp;
    return true;
  }

  if(the_session_manager->has_session(username))
  {
    session_response* resp = new session_response(this, "You already have a running session.", Session_Alloc_Err);
    resp->send();
    delete resp;
    return true;
  }

  if(the_session_manager->reservation_time_left(username) <= 0)
  {
    session_response* resp = new session_response(this, "Reservation not found.", Session_No_Res_Err);
    resp->send();
    delete resp;
    return true;
  }

  session_ptr sess = the_session_manager->add_session(username, nccpconn, this);
  if(sess == NULL)
  {
    session_response* resp = new session_response(this, "Allocation failed.", Session_Alloc_Err);
    resp->send();
    delete resp;
    return true;
  }

  //sess->set_begin_request(this);

  session_response* resp = new session_response(this, sess->getID());
  resp->send();
  delete resp;
  
  return false;
}

void
begin_session_req::parse()
{
  request::parse();

  buf >> expinfo;
  buf >> password;
  buf >> num_topology_reqs;
}

session_add_component_req::session_add_component_req(uint8_t *mbuf, uint32_t size): rlicrd_request(mbuf, size)
{
}

session_add_component_req::~session_add_component_req()
{
}

bool
session_add_component_req::handle()
{
  session_ptr sess = the_session_manager->get_session(nccpconn, expinfo.getID());
  if(!sess)
  {
    component_response* resp = new component_response(this, NCCP_Status_OperationIncomplete);
    resp->send();
    delete resp;
    return true;
  }
  
  sess->add_component(this);

  return false;
}

void
session_add_component_req::parse()
{
  rlicrd_request::parse();
 
  buf >> ipaddr;
  buf >> reboot_params;
  buf >> init_params;
}

session_add_cluster_req::session_add_cluster_req(uint8_t *mbuf, uint32_t size): rlicrd_request(mbuf, size)
{
}

session_add_cluster_req::~session_add_cluster_req()
{
}

bool
session_add_cluster_req::handle()
{
  session_ptr sess = the_session_manager->get_session(nccpconn, expinfo.getID());
  if(!sess)
  {
    cluster_response* resp = new cluster_response(this, NCCP_Status_Failed);
    resp->send();
    delete resp;
    return true;
  }

  sess->add_cluster(this);

  return false;
}

void
session_add_cluster_req::parse()
{
  rlicrd_request::parse();

  clus.setType(comp.getType());
  clus.setID(comp.getID());
  clus.setParent(comp.getParent());
  clus.setLabel(comp.getLabel());
  clus.setCP(comp.getCP());

  //clus.parseExtra(buf);
}

session_add_link_req::session_add_link_req(uint8_t *mbuf, uint32_t size): rlicrd_request(mbuf, size)
{
}

session_add_link_req::~session_add_link_req()
{
}

bool
session_add_link_req::handle()
{
  session_ptr sess = the_session_manager->get_session(nccpconn, expinfo.getID());
  if(!sess)
  {
    write_log("session_add_link_req::handle():failed to find session " + expinfo.getID() + " user " + expinfo.getUserName() + " link comp " + int2str(getFromComponent().getID()) + "," + getFromComponent().getType() + " port " + int2str(getFromPort()) + " ip " + getFromIP() + " subnet " + getFromSubnet() + " nexthop " + getFromNHIP() + " to comp " + int2str(getToComponent().getID()) + "," + getToComponent().getType() + " port " + int2str(getToPort()) + " ip " + getToIP() + " subnet " + getToSubnet() + " nexthop " + getToNHIP());
    rlicrd_response* resp = new rlicrd_response(this, NCCP_Status_Failed);
    resp->send();
    delete resp;
    return true;
  }

  write_log("session_add_link_req::handle(): link comp " + int2str(getFromComponent().getID()) + "," + getFromComponent().getType() + " port " + int2str(getFromPort()) + " ip " + getFromIP() + " subnet " + getFromSubnet() + " nexthop " + getFromNHIP() + " to comp " + int2str(getToComponent().getID()) + "," + getToComponent().getType() + " port " + int2str(getToPort()) + " ip " + getToIP() + " subnet " + getToSubnet() + " nexthop " + getToNHIP());

  sess->add_link(this);

  return false;
}

void
session_add_link_req::parse()
{
  rlicrd_request::parse();

  buf >> from_comp;
  buf >> from_port;
  buf >> from_ip;
  buf >> from_subnet;
  buf >> from_nhip;
  if (version > 0x75)
    buf >> from_cap;
  else from_cap = 0;
  buf >> to_comp;
  buf >> to_port;
  buf >> to_ip;
  buf >> to_subnet;
  buf >> to_nhip;
  if (version > 0x75)
    buf >> to_cap;
  else to_cap = 0;
  buf >> bandwidth;
}

commit_session_req::commit_session_req(uint8_t *mbuf, uint32_t size): request(mbuf, size)
{
}

commit_session_req::~commit_session_req()
{
}

bool
commit_session_req::handle()
{
  session_ptr sess = the_session_manager->get_session(nccpconn, eid.getString());
  if(!sess)
  {
    session_response* resp = new session_response(this, eid, "Session not found.", Session_Alloc_Err);
    resp->send();
    delete resp;
    return true;
  }

  session_response* resp;
  if(sess->commit(sess) == false)
  {
    resp = new session_response(this, eid, sess->getErrorMsg(), Session_Alloc_Err);
  }
  else
  {
    resp = new session_response(this, eid);
  }

  resp->send();
  delete resp;
  return true;
}

void
commit_session_req::parse()
{
  request::parse();
  
  buf >> eid;
}

clear_session_req::clear_session_req(uint8_t *mbuf, uint32_t size): request(mbuf, size)
{
}

clear_session_req::~clear_session_req()
{
}

bool
clear_session_req::handle()
{
  session_ptr sess = the_session_manager->get_session(nccpconn, eid.getString());
  if(!sess)
  {
    return true;
  }

  sess->clear();

  return true;
} 
  
void
clear_session_req::parse()
{
  request::parse();

  buf >> eid;
}

rli_ping::rli_ping(request* req): periodic()
{
  op = NCCP_Operation_AckPeriodic;
  rend = req->get_rendezvous();
  return_rend = req->get_return_rendezvous();
  nccpconn = req->get_connection();
}

rli_ping::~rli_ping()
{
}

begin_reservation_req::begin_reservation_req(uint8_t *mbuf, uint32_t size): request(mbuf, size)
{
}

begin_reservation_req::~begin_reservation_req()
{
}

bool
begin_reservation_req::handle()
{
  if(version < MIN_RLI_VERSION)
  {
    reservation_response* resp = new reservation_response(this, "Your RLI is out of date.  Please go to http://onl.wustl.edu to download a new version.", Session_Version_Err);
    resp->send();
    delete resp;
    return true;
  }

  if(!the_session_manager->authenticate(username.getString(), password.getString()))
  {
    reservation_response* resp = new reservation_response(this, "Authentication failed.", Session_Auth_Err);
    resp->send();
    delete resp;
    return true;
  }

  reservation* res = the_session_manager->add_reservation(username.getString(), nccpconn, this);
  if(res == NULL)
  {
    reservation_response* resp = new reservation_response(this, "Allocation failed.", Session_Alloc_Err); 
    resp->send();
    delete resp;
    return true;
  }

  //res->set_begin_request(this);

  reservation_response* resp = new reservation_response(this, res->getID(), Session_Ack);
  resp->send();
  delete resp;

  return false;
}

void
begin_reservation_req::commit_done(bool success, std::string msg, std::string start_time)
{
  if(success)
  {
    reservation_response* resp = new reservation_response(this, msg, start_time);
    resp->send();
    delete resp;
  }
  else
  {
    reservation_response* resp = new reservation_response(this, msg);
    resp->send();
    delete resp;
  }
}

void
begin_reservation_req::parse()
{
  request::parse();

  buf >> username;
  buf >> password;
  buf >> earlyStart;
  buf >> lateStart;
  buf >> duration;
  buf >> num_topology_reqs;
}

reservation_add_component_req::reservation_add_component_req(uint8_t *mbuf, uint32_t size): rlicrd_request(mbuf, size)
{
}

reservation_add_component_req::~reservation_add_component_req()
{
}

bool
reservation_add_component_req::handle()
{
  reservation* res = the_session_manager->get_reservation(nccpconn, expinfo.getID());
  if(res == NULL)
  {
    component_response* resp = new component_response(this, NCCP_Status_OperationIncomplete);
    resp->send();
    delete resp;
    return true;
  }

  res->add_component(this);

  return false;
}

void
reservation_add_component_req::parse()
{
  rlicrd_request::parse();

  buf >> ipaddr;
  buf >> reboot_params;
  buf >> init_params;
}

reservation_add_cluster_req::reservation_add_cluster_req(uint8_t *mbuf, uint32_t size): rlicrd_request(mbuf, size)
{
}

reservation_add_cluster_req::~reservation_add_cluster_req()
{
}

bool
reservation_add_cluster_req::handle()
{
  //reservation* res = the_session_manager->get_reservation(nccpconn);
  reservation* res = the_session_manager->get_reservation(nccpconn, expinfo.getID());
  if(res == NULL)
  {
    cluster_response* resp = new cluster_response(this, NCCP_Status_Failed);
    resp->send();
    delete resp;
    return true;
  }

  res->add_cluster(this);

  return false;
}

void
reservation_add_cluster_req::parse()
{
  rlicrd_request::parse();

  clus.setType(comp.getType());
  clus.setID(comp.getID());
  clus.setParent(comp.getParent());
  clus.setLabel(comp.getLabel());
  clus.setCP(comp.getCP());

  //clus.parseExtra(buf);
}

reservation_add_link_req::reservation_add_link_req(uint8_t *mbuf, uint32_t size): rlicrd_request(mbuf, size)
{
}

reservation_add_link_req::~reservation_add_link_req()
{
}

bool
reservation_add_link_req::handle()
{
  //reservation* res = the_session_manager->get_reservation(nccpconn);
  reservation* res = the_session_manager->get_reservation(nccpconn, expinfo.getID());
  if(res == NULL)
  {
    rlicrd_response* resp = new rlicrd_response(this, NCCP_Status_Failed);
    resp->send();
    delete resp;
    return true;
  } 
  
  res->add_link(this);
  
  return false;
}   

void
reservation_add_link_req::parse()
{   
  rlicrd_request::parse();

  buf >> from_comp; 
  buf >> from_port;
  buf >> from_ip;
  buf >> from_subnet;
  buf >> from_nhip;
  if (version > 0x75)
    buf >> from_cap;
  else from_cap = 0;
  buf >> to_comp;
  buf >> to_port;
  buf >> to_ip;
  buf >> to_subnet;
  buf >> to_nhip;
  if (version > 0x75)
    buf >> to_cap;
  else to_cap = 0;
  buf >> bandwidth;
}

commit_reservation_req::commit_reservation_req(uint8_t *mbuf, uint32_t size): request(mbuf, size)
{
}

commit_reservation_req::~commit_reservation_req()
{
}

bool
commit_reservation_req::handle()
{
  //reservation* res = the_session_manager->get_reservation(nccpconn);
  reservation* res = the_session_manager->get_reservation(nccpconn, eid.getString());
  if(res == NULL)
  {
    reservation_response* resp = new reservation_response(this, "Reservation not found.");
    resp->send();
    delete resp;
    return true;
  }

  res->commit(); 
  the_session_manager->remove_reservation(res);
  return true;
}

void
commit_reservation_req::parse()
{
  request::parse();

  buf >> eid;
}

extend_reservation_req::extend_reservation_req(uint8_t *mbuf, uint32_t size): request(mbuf, size)
{
}

extend_reservation_req::~extend_reservation_req()
{
}

bool
extend_reservation_req::handle()
{
  if(!the_session_manager->authenticate(username.getString(), password.getString()))
  {
    reservation_response* resp = new reservation_response(this, "Authentication failed.", Session_Auth_Err);
    resp->send();
    delete resp;
    return true;
  }
  
  if(the_session_manager->reservation_time_left(username.getString()) <= 0)
  {
    reservation_response* resp = new reservation_response(this, "You don't have a reservation now to extend.", Session_No_Res_Err);
    resp->send();
    delete resp; 
    return true;
  }

  if(the_session_manager->extend_reservation(username.getString(), extend_mins) == false)
  {
    reservation_response* resp = new reservation_response(this, "No room to extend your reservation.", Session_No_Res_Err);
    resp->send();
    delete resp; 
    return true;
  }

  reservation_response* resp = new reservation_response(this, NCCP_Status_Fine, "Success", Session_No_Err);
  resp->send();
  delete resp; 
  return true;
}

void
extend_reservation_req::parse()
{
  request::parse();

  buf >> username;
  buf >> password;
  buf >> extend_mins;
}

cancel_reservation_req::cancel_reservation_req(uint8_t *mbuf, uint32_t size): request(mbuf, size)
{
}

cancel_reservation_req::~cancel_reservation_req()
{
}

bool
cancel_reservation_req::handle()
{
  if(!the_session_manager->authenticate(username.getString(), password.getString()))
  {
    reservation_response* resp = new reservation_response(this, "Authentication failed.", Session_Auth_Err);
    resp->send();
    delete resp;
    return true;
  }
  if(the_session_manager->has_session(username.getString()))
  {
    reservation_response* resp = new reservation_response(this, "Can't cancel reservation during running session.", Session_No_Res_Err);
    resp->send();
    delete resp;
    return true;
  }

  bool cancelled = the_session_manager->cancel_reservation(username.getString());
  if(!cancelled)
  {
    sleep(3);
    cancelled = the_session_manager->cancel_reservation(username.getString());
  }

  if(!cancelled)
  {
    reservation_response* resp = new reservation_response(this, "Could not cancel reservation.", Session_No_Res_Err);
    resp->send();
    delete resp;
    return true;
  }

  reservation_response* resp = new reservation_response(this, NCCP_Status_Fine, "Success", Session_No_Err);
  resp->send();
  delete resp;
  return true;
}

void
cancel_reservation_req::parse()
{
  request::parse();

  buf >> username;
  buf >> password;
}
