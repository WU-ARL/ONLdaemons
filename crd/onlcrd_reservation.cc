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

reservation::reservation(uint32_t rid, std::string username, nccp_connection* nc)
{
  id = int2str(rid).c_str();
  user = username;
  nccpconn = nc;

  cleared = false;

  pthread_mutex_init(&req_lock, NULL);
}

reservation::~reservation()
{
  clear();

  pthread_mutex_destroy(&req_lock);
}

void
reservation::set_begin_request(begin_reservation_req* req)
{
  autoLockDebug rlock(req_lock, "reservation::set_begin_request(): req_lock");
  write_log("reservation::set_begin_request user:" + user + " id:" + id);
  begin_req = req;
}

void
reservation::add_component(reservation_add_component_req* req)
{
  autoLockDebug rlock(req_lock, "reservation::add_component(): req_lock");
  write_log("reservation::add_component user:" + user + " id:" + id + " component:" + req->getComponent().getLabel());
  component_reqs.push_back(req);
}

void
reservation::add_cluster(reservation_add_cluster_req* req)
{
  autoLockDebug rlock(req_lock, "reservation::add_cluster(): req_lock");
  write_log("reservation::add_cluster user:" + user + " id:" + id + " component:" + req->getComponent().getLabel());
  cluster_reqs.push_back(req);
}

void
reservation::add_link(reservation_add_link_req* req)
{
  autoLockDebug rlock(req_lock, "reservation::add_link(): req_lock");
  write_log("reservation::add_link user:" + user + " id:" + id + " component:" + req->getComponent().getLabel());
  link_reqs.push_back(req);
}

void
reservation::commit()
{
  autoLockDebug rlock(req_lock, "reservation::commit(): req_lock");
  write_log("reservation::commit user:" + user + " id:" + id);
  if(cleared) return;

  uint32_t num_reqs = begin_req->getNumRequests();
  uint32_t num_reqs_rxed = component_reqs.size() + cluster_reqs.size() + link_reqs.size();
  write_log("reservation::commit(): user:" + user + " id:" + id + " expecting " + int2str(num_reqs) + " requests rxed:" + int2str(num_reqs_rxed));
  uint32_t num_attempts = 0;
  while(num_reqs_rxed < num_reqs)
  {
    // check every 50ms until all reqs received or it's been 1 minute
    if(num_attempts > 1200) { return; }
    num_attempts++;
    rlock.unlock();
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 50000000;
    nanosleep(&ts, NULL);
    rlock.lock();
    num_reqs_rxed = component_reqs.size() + cluster_reqs.size() + link_reqs.size();  }

  while(!cluster_reqs.empty())
  {
    session_add_cluster_req* req = (session_add_cluster_req*)cluster_reqs.front();
    cluster_reqs.pop_front();

    component c = req->getComponent();
    NCCP_StatusType stat = NCCP_Status_Fine;
    onl::onldb_resp r = topology.add_node(c.getType(), c.getID(), c.getParent());
    if(r.result() < 1)
    {
      stat = NCCP_Status_AllocFailed;
    }

    cluster_response* resp = new cluster_response(req, stat);
    resp->send();
    delete resp;
    delete req;
  }

  while(!component_reqs.empty())
  {
    session_add_component_req* req = (session_add_component_req*)component_reqs.front();
    component_reqs.pop_front();
  
    component c = req->getComponent();
    NCCP_StatusType stat = NCCP_Status_Fine;
    onl::onldb_resp r = topology.add_node(c.getType(), c.getID(), c.getParent());
    if(r.result() < 1)
    {
      stat = NCCP_Status_AllocFailed;
    }
    else if(c.getCP() != "")
    {
      the_session_manager->fix_component(&topology, c.getID(), c.getCP());
    }

    component_response* resp = new component_response(req, stat);
    resp->send();
    delete resp;
    delete req;
  }

  while(!link_reqs.empty())
  {
    session_add_link_req* req = (session_add_link_req*)link_reqs.front();
    link_reqs.pop_front();
  
    component c = req->getComponent();
    NCCP_StatusType stat = NCCP_Status_Fine;

    std::string n1type = topology.get_type(req->getFromComponent().getID());
    int n1port = req->getFromPort();
    int n1cap = req->getFromCapacity();
    std::string n2type = topology.get_type(req->getToComponent().getID());
    int n2port = req->getToPort();
    int n2cap = req->getToCapacity();

    bool hascap = true;
    if (req->getVersion() <= 0x75) hascap = false;
  
    if(n1type == "vgige" && n2type == "vgige")
    {
      // we could try to calculate the max capacity here instead..
      n1cap = MAX_INTERCLUSTER_CAPACITY;
      n2cap = MAX_INTERCLUSTER_CAPACITY;
    }
    else if(n1type == "vgige")
    {
      if (!hascap)
	{
	  n2cap = the_session_manager->get_capacity(n2type,n2port);
	  if (topology.has_virtual_port(req->getToComponent().getID()))
	    {
	      if (n2cap < req->getCapacity()) n2cap = -1;
	      else n2cap = req->getCapacity();
	    }
	}
      n1cap = n2cap;
    }
    else if(n2type == "vgige")
    {
      if (!hascap)
	{
	  n1cap = the_session_manager->get_capacity(n1type,n1port);
	  if (topology.has_virtual_port(req->getFromComponent().getID()))
	    {
	      if (n1cap < req->getCapacity()) n1cap = -1;
	      else n1cap = req->getCapacity();
	    }
	}
      n2cap = n1cap;
    }
    else if (!hascap)
    {
      n1cap = the_session_manager->get_capacity(n1type,n1port);
      if (the_session_manager->has_virtual_port(n1type))
	{
	  if (n1cap < req->getCapacity()) n1cap = -1;
	  else n1cap = req->getCapacity();
	}
      n2cap = the_session_manager->get_capacity(n2type,n2port);
      if (the_session_manager->has_virtual_port(n2type))
	{
	  if (n2cap < req->getCapacity()) n2cap = -1;
	  else n2cap = req->getCapacity();
	}
    }

    if(n1cap == -1 || n2cap == -1 || (!hascap && n1cap != n2cap))
    {
      stat = NCCP_Status_AllocFailed;
    }
    else
    {
      onl::onldb_resp r = topology.add_cap_link(c.getID(), n1cap, req->getFromComponent().getID(), n1port, n1cap, req->getToComponent().getID(), n2port, n2cap);
      if(r.result() < 1)
      {
        stat = NCCP_Status_AllocFailed;
      }
    }

    rlicrd_response* resp = new rlicrd_response(req, stat);
    resp->send();
    delete resp;
    delete req;
  }

  // now assign the physical resources to the virtual topology
  if(!the_session_manager->make_reservation(this, user, begin_req->getEarlyStart(), begin_req->getLateStart(), begin_req->getDuration(), &topology))
  {
    begin_req->commit_done(false, errmsg, "");
  }
  else
  {
    begin_req->commit_done(true, "Reservation Request Succeeded", errmsg);
  }

  delete begin_req;
  begin_req = NULL;
}

void
reservation::clear()
{
  autoLockDebug rlock(req_lock, "reservation::clear(): req_lock");
  if(cleared) return;
  cleared = true;

  write_log("reservation::clear(): clearing reservation " + user + "," + id);

  while(!cluster_reqs.empty())
  {
    reservation_add_cluster_req* req = cluster_reqs.front();
    cluster_reqs.pop_front();

    cluster_response* resp = new cluster_response(req, NCCP_Status_Failed);
    resp->set_more_message(true);
    resp->send();
    delete resp;
    delete req;
  }

  while(!component_reqs.empty())
  {
    reservation_add_component_req* req = component_reqs.front();
    component_reqs.pop_front();

    component_response* resp = new component_response(req, NCCP_Status_Failed);
    resp->set_more_message(true);
    resp->send();
    delete resp;
    delete req;
  }

  while(!link_reqs.empty())
  {
    reservation_add_link_req* req = link_reqs.front();
    link_reqs.pop_front();

    rlicrd_response* resp = new rlicrd_response(req, NCCP_Status_Failed);
    resp->set_more_message(true);
    resp->send();
    delete resp;
    delete req;
  }

  if(begin_req != NULL)
  {
    begin_req->commit_done(false, "Reservation failed.", "");
  }
  delete begin_req;
  begin_req = NULL;
}
