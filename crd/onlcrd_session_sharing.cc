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

observe_data::observe_data()
{
  accept = 0;
  proxy_sid = 0;
}

observe_data::observe_data(const observe_data& od)
{
  sid = od.sid;
  observer = od.observer;
  observee = od.observee;
  password = od.password;
  accept = od.accept;
  proxy_sid = od.proxy_sid;
} 
  
observe_data::~observe_data()
{
}

observe_data&
observe_data::operator=(const observe_data& od)
{ 
  sid = od.sid;
  observer = od.observer;
  observee = od.observee;
  password = od.password;
  accept = od.accept;
  proxy_sid = od.proxy_sid;
  return *this;
}

byte_buffer&
onlcrd::operator<<(byte_buffer& buf, observe_data& od)
{ 
  buf << od.sid;
  buf << od.observer;
  buf << od.observee;
  //cgw, would be nice to change this..
  if(od.accept) { buf << (uint8_t)1; }
  else { buf << (uint8_t)0; }
  buf << od.proxy_sid;
  return buf;
} 

byte_buffer&
onlcrd::operator>>(byte_buffer& buf, observe_data& od)
{
  buf >> od.sid;
  buf >> od.observer;
  buf >> od.observee;
  buf >> od.password;
  uint8_t tmp_a; buf >> tmp_a;
  if(tmp_a == 0) { od.accept = false; }
  else { od.accept  = true; }
  buf >> od.proxy_sid;
  return buf; 
} 

add_observer_req::add_observer_req(uint8_t *mbuf, uint32_t size): rlicrd_request(mbuf, size)
{
}

add_observer_req::~add_observer_req()
{   
}
    
bool
add_observer_req::handle()
{ 
  session_ptr sess = the_session_manager->get_session(nccpconn);
  if(!sess)
  {
    rlicrd_response* resp = new rlicrd_response(this, NCCP_Status_Failed);
    resp->send();
    delete resp;
    return true;
  } 
  
  sess->add_observer(username.getString());
  
  rlicrd_response* resp = new rlicrd_response(this, NCCP_Status_Fine);
  resp->send();
  delete resp;
  return true;
}
  
void
add_observer_req::parse()
{
  rlicrd_request::parse();
 
  buf >> username;
}

get_observable_req::get_observable_req(uint8_t *mbuf, uint32_t size): request(mbuf, size)
{
}

get_observable_req::~get_observable_req()
{
}

bool
get_observable_req::handle()
{
  if(!the_session_manager->authenticate(username.getString(), password.getString()))
  {
    observable_response* resp = new observable_response(this, "Authentication failed.", Session_Auth_Err);
    resp->send();
    delete resp;
    return true;
  }

  std::list<session_ptr> sess_list;
  the_session_manager->get_observable_sessions(username.getString(), sess_list);

  observable_response* resp = new observable_response(this, sess_list);
  resp->send();
  delete resp;
  return true;
}

void
get_observable_req::parse()
{
  request::parse();

  buf >> username;
  buf >> password;
}

observable_response::observable_response(request *req, std::string msg, Session_Error err): response(req)
{
  status = NCCP_Status_Failed;
  status_msg = msg.c_str();
  error_code = err;
}

observable_response::observable_response(request *req, std::list<session_ptr>& list): response(req)
{
  status = NCCP_Status_Fine;
  status_msg = "Success.";
  error_code = Session_No_Err;
  sess_list = list;
}

observable_response::~observable_response()
{
}

void
observable_response::write()
{
  response::write();

  buf << status;
  buf << status_msg;
  buf << error_code;
  buf << (uint32_t)sess_list.size();
  while(!sess_list.empty())
  {
    session_ptr sess = (session_ptr)sess_list.front();
    sess_list.pop_front();
 
    nccp_string id(sess->getID().c_str());
    nccp_string user(sess->getUser().c_str());
    buf << id;
    buf << user;
  }
}

observe_req::observe_req(uint8_t *mbuf, uint32_t size): request(mbuf, size)
{
}

observe_req::~observe_req()
{
}

void
observe_req::parse()
{
  request::parse();

  buf >> data;
}

observe_session_req::observe_session_req(uint8_t *mbuf, uint32_t size): observe_req(mbuf, size)
{
}

observe_session_req::~observe_session_req()
{
}

bool
observe_session_req::handle()
{
  if(!the_session_manager->authenticate(data.getObserver(), data.getPassword()))
  {
    observe_response* resp = new observe_response(this, "Authentication failed.", Session_Auth_Err);
    resp->send();
    delete resp;
    return true;
  }

  session_ptr sess = the_session_manager->get_session(data.getObservee());
  if(!sess)
  {
    observe_response* resp = new observe_response(this, "Session not found.", Session_Alloc_Err);
    resp->send();
    delete resp;
    return true;
  }

  if(!sess->observe(this))
  {
    observe_response* resp = new observe_response(this, "Session not found.", Session_Alloc_Err);
    resp->send();
    delete resp;
    return true;
  }

  observe_response* resp = new observe_response(this, "Request has been sent to experiment owner.", Session_No_Err, NCCP_Status_StillRemaining);
  resp->send();
  delete resp;
  return false;
}

grant_observe_req::grant_observe_req(uint8_t *mbuf, uint32_t size): observe_req(mbuf, size)
{
}

grant_observe_req::~grant_observe_req()
{
}

bool
grant_observe_req::handle()
{
  session_ptr sess = the_session_manager->get_session(nccpconn);
  if(!sess)
  {
    observe_response* resp = new observe_response(this, "Authentication failed.", Session_Auth_Err);
    resp->send();
    delete resp;
    return true;
  }

  sess->grant_observe(this);

  observe_response* resp = new observe_response(this, NCCP_Status_Fine);
  resp->send();
  delete resp;
  return true;
}

observe_response::observe_response(observe_req* req, NCCP_StatusType stat): response(req)
{
  status = stat;
  error_code = Session_No_Err;
  data = req->getObserveData();
} 

observe_response::observe_response(observe_req* req, std::string msg, Session_Error err): response(req)
{
  status = NCCP_Status_Failed;
  status_msg = msg.c_str();
  error_code = err;
  data = req->getObserveData();
} 

observe_response::observe_response(observe_req* req, std::string msg, Session_Error err, NCCP_StatusType stat): response(req)
{
  status = stat;
  status_msg = msg.c_str();
  error_code = err;
  data = req->getObserveData();
} 

observe_response::observe_response(request* req, std::string msg, Session_Error err, NCCP_StatusType stat, observe_data& d): response(req)
{
  status = stat;
  status_msg = msg.c_str();
  error_code = err;
  data = d;
} 

observe_response::~observe_response()
{
} 

void
observe_response::write()
{
  response::write();
  
  buf << status;
  buf << status_msg;
  buf << error_code;
  buf << data;
}
