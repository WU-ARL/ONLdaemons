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
#include <fstream>
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

session::session(uint32_t sid, std::string username, nccp_connection* nc)
{
  id = int2str(sid).c_str();
  user = username;
  nccpconn = nc;

  cleared = false;
  
  vlans_created = false;

  pthread_mutex_init(&req_lock, NULL);
  begin_req = NULL;

  user_session_file = session_file_base + "/" + user;

  pthread_mutex_init(&share_lock, NULL);

  mapping_written = false;

  write_log("session::session: added session " + id + " for user " + user);
}

session::~session()
{
  write_log("session::~session: deleting session " + id);

  pthread_mutex_destroy(&req_lock);
  pthread_mutex_destroy(&share_lock);
}

void
session::set_begin_request(begin_session_req* req)
{
  autoLockDebug rlock(req_lock, "session::set_begin_request(): req_lock");
  begin_req = req;
  rlock.unlock();

  pthread_t tid;
  pthread_attr_t tattr;
  if(pthread_attr_init(&tattr) != 0)
  {
    write_log("session::set_begin_request(): pthread_attr_init failed");
    return;
  }
  if(pthread_attr_setdetachstate(&tattr,PTHREAD_CREATE_DETACHED) != 0)
  {
    write_log("session::set_begin_request(): pthread_attr_setdetachstate failed");
    return;
  }
  if(pthread_create(&tid, &tattr, session_send_alerts_wrapper, (void *)this) != 0)
  {
    write_log("session::set_begin_request(): pthread_create failed");
    return;
  }
}

void
session::add_component(session_add_component_req* req)
{
  autoLockDebug rlock(req_lock, "session::add_component(): req_lock");
  component_reqs.push_back(req);
}

void
session::add_cluster(session_add_cluster_req* req)
{
  autoLockDebug rlock(req_lock, "session::add_cluster(): req_lock");
  cluster_reqs.push_back(req);
}

void
session::add_link(session_add_link_req* req)
{
  autoLockDebug rlock(req_lock, "session::add_link(): req_lock");
  link_reqs.push_back(req);
}

bool
session::commit(session_ptr sess)
{
  autoLockDebug rlock(req_lock, "session::commit(): req_lock");
  if(cleared) return false;

  uint32_t num_reqs = begin_req->getNumRequests();
  uint32_t num_reqs_rxed = component_reqs.size() + cluster_reqs.size() + link_reqs.size();
  uint32_t num_attempts = 0;
  while(num_reqs_rxed < num_reqs)
  { 
    // check every 50ms until all reqs received or it's been 1 minute
    if(num_attempts > 1200) { return false; }
    num_attempts++;
    rlock.unlock();
    struct timespec ts;
    ts.tv_sec = 0;
    ts.tv_nsec = 50000000;
    nanosleep(&ts, NULL);
    rlock.lock();
    num_reqs_rxed = component_reqs.size() + cluster_reqs.size() + link_reqs.size();
  }

  // first have to build up the topology object

  //process cluster requests
  std::list<session_add_cluster_req *>::iterator cl_it;
  for(cl_it = cluster_reqs.begin(); cl_it != cluster_reqs.end(); )
  {
    session_add_cluster_req* req = (session_add_cluster_req*) *cl_it;
    component c = req->getComponent();
    onl::onldb_resp r = topology.add_node(c.getType(), c.getID(), c.getParent());
    if(r.result() < 1)
    {
      cl_it = cluster_reqs.erase(cl_it);

      cluster_response* resp = new cluster_response(req, NCCP_Status_AllocFailed);
      resp->send();
      delete resp;
      delete req;
    }
    else
    {
      ++cl_it;
    }
  }

  //process component requests
  std::list<session_add_component_req *>::iterator hw_it;
  for(hw_it = component_reqs.begin(); hw_it != component_reqs.end(); )
  {
    session_add_component_req* req = (session_add_component_req*) *hw_it;
    component c = req->getComponent();
    onl::onldb_resp r = topology.add_node(c.getType(), c.getID(), c.getParent(), the_session_manager->has_virtual_port(c.getType()), req->getCores(), req->getMemory(), req->getNumInterfaces(), req->getInterfaceBW(), req->getExtTag());
    if(r.result() < 1)
    {
      hw_it = component_reqs.erase(hw_it);

      component_response* resp = new component_response(req, NCCP_Status_AllocFailed);
      resp->send();
      delete resp;
      delete req;
    }
    else
    {
      ++hw_it;
    }
  }

  //process link requests
  std::list<session_add_link_req *>::iterator lnk_it;
  for(lnk_it = link_reqs.begin(); lnk_it != link_reqs.end(); )
  {
    session_add_link_req* req = (session_add_link_req*) *lnk_it;
    component c = req->getComponent();

    std::string n1type = topology.get_type(req->getFromComponent().getID());
    int n1port = req->getFromPort();
    int n1cap = req->getFromCapacity();
    std::string n2type = topology.get_type(req->getToComponent().getID());
    int n2port = req->getToPort();
    int n2cap = req->getToCapacity();

    bool hascap = true;
    if (req->get_version() <= 0x75) hascap = false;

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
	}
      n1cap = n2cap;
    }
    else if(n2type == "vgige")
    {
      if (!hascap)
	{
	  n1cap = the_session_manager->get_capacity(n1type,n1port);
	}
      n2cap = n1cap;
    }
    else if (!hascap)
    {
      n1cap = the_session_manager->get_capacity(n1type,n1port);
      n2cap = the_session_manager->get_capacity(n2type,n2port);
    }


    if(n1cap == -1 || n2cap == -1 || (!hascap && n1cap != n2cap))
    {
      lnk_it = link_reqs.erase(lnk_it);

      write_log("session::commit link failed because capacity error. session " + getID() + " user " + getUser() + " link " + c.getLabel() + " n1cap " + int2str(n1cap) + " n2cap " + int2str(n2cap));
      rlicrd_response* resp = new rlicrd_response(req, NCCP_Status_Failed);
      resp->send();
      delete resp;
      delete req;
    }
    else
    {
      onl::onldb_resp r = topology.add_cap_link(c.getID(), n1cap, req->getFromComponent().getID(), n1port, n1cap, req->getToComponent().getID(), n2port, n2cap);
      if(r.result() < 1)
      {
        lnk_it = link_reqs.erase(lnk_it);
      write_log("session::commit link failed because topology.add_link failed. session " + getID() + " user " + getUser() + " link " + c.getLabel() + " error msg:" + r.msg());

        rlicrd_response* resp = new rlicrd_response(req, NCCP_Status_Failed);
        resp->send();
        delete resp;
        delete req;
      }
      else
      {
        ++lnk_it;
      }
    }
  }

  // now assign the physical resources to the virtual topology
  if(!the_session_manager->assign_resources(sess, user, &topology))
  {
    rlock.unlock();
    return false;
  }

  topology.print_resources();

  bool is_admin = the_session_manager->is_admin(user);

  // go through the assigned components and links to initialize everything
  while(!cluster_reqs.empty())
  {
    session_add_cluster_req* req = (session_add_cluster_req*)cluster_reqs.front();
    cluster_reqs.pop_front();

    unsigned int compid = req->getComponent().getID();

    NCCP_StatusType stat = NCCP_Status_Fine;
    if (testing) stat = NCCP_Status_Testing;
    cluster_response* resp = new cluster_response(req, stat, compid);
    resp->send();
    delete resp;
    delete req;
  }

  while(!component_reqs.empty())
  {
    session_add_component_req* req = (session_add_component_req*)component_reqs.front();
    component_reqs.pop_front();

    unsigned int compid = req->getComponent().getID();
    std::string compname = topology.get_component(compid);
    unsigned int vmid = topology.get_vmid(compid);
    crd_component_ptr comp = the_session_manager->get_component(compname, vmid);
    comp->set_component(req->getComponent());

    comp->add_reboot_params(req->getRebootParams());
    comp->add_init_params(req->getInitParams());
    comp->set_ip_addr(req->getIPAddr());
    std::string extip = topology.get_extip(compid);
    if (extip.length() > 0)
      {
	comp->setDevIP(extip);
      }
    else comp->setDevIP(compname);

    comp->setCores(req->getCores());
    comp->setMemory(req->getMemory());
    
    components.push_back(comp);
    if(comp->is_admin_mode() && !is_admin)
    {
      comp->set_add_request(NULL);
      component_response* resp = new component_response(req, NCCP_Status_Failed);
      resp->send();
      delete resp;
      delete req;
    }
    else
    {
      the_session_manager->add_component(comp, sess);
      comp->set_add_request(req);
    }
  }

  set_clusters();

  while(!link_reqs.empty())
  {
    session_add_link_req* req = (session_add_link_req*)link_reqs.front();
    link_reqs.pop_front();

    unsigned int fromcompid = req->getFromComponent().getID();
    unsigned int tocompid = req->getToComponent().getID();
    crd_component_ptr fromcomp;
    crd_component_ptr tocomp;
    std::list<crd_component_ptr>::iterator i;
    for(i=components.begin(); i!=components.end(); ++i)
    {
      if((fromcompid == tocompid) && ((*i)->get_component().getID() == fromcompid))
      {
        fromcomp = *i;
        tocomp = *i;
      }
      else if((*i)->get_component().getID() == fromcompid)
      {
        fromcomp = *i;
      }
      else if((*i)->get_component().getID() == tocompid)
      {
        tocomp = *i;
      }
    }

    crd_link_ptr link(new crd_link(fromcomp, req->getFromPort(), tocomp, req->getToPort()));
    link->set_component(req->getComponent());
    std::list<int> clist;
    topology.get_conns(req->getComponent().getID(), clist);

    //set the real physical ports of the link
    int rp1 = topology.get_realport(req->getComponent().getID(), 1);
    int rp2 = topology.get_realport(req->getComponent().getID(), 2);  
    link->set_rports(rp1, rp2);

    //set the capacity of the link
    int cap1 = topology.get_capacity(req->getComponent().getID(), 1);
    int cap2 = topology.get_capacity(req->getComponent().getID(), 2);
    link->set_capacity(cap1, cap2);

    link->set_switch_ports(clist);

    //set macaddr
    std::string mac1 = topology.get_macaddr(req->getComponent().getID(), 1);
    std::string mac2 = topology.get_macaddr(req->getComponent().getID(), 1);
    link->set_mac(mac1, mac2);

    link->set_session(sess);
    link->set_add_request(req);
    fromcomp->add_link(link);
    tocomp->add_link(link);
    links.push_back(link);
    unreported_links.push_back(req->getComponent().getID());
  }

  // this only removes any sessions that are still running past their end time 
  // if there is not enough interswitch bandwidth to support the new session
  // and the sessions that have run over
  the_session_manager->check_for_expired_sessions();

  // have to allocate vlans for all virtual switch sets before we start to ensure that
  // everything on one set of connected vswitches gets the same vlan
  std::list<crd_component_ptr>::iterator i;
  for(i=components.begin(); i!=components.end(); ++i)
  {
    if((*i)->marked()) { continue; }
    if((*i)->get_type() != "vgige") { continue; }

    vlan_ptr new_vlan = the_session_manager->add_vlan(sess->getID());
    write_log("session::commit(): component " + (*i)->getName() + " adding vlan " + int2str(new_vlan->vlanid));
    if(new_vlan->vlanid == 0)
    {
      write_log("session::commit(): warning: add_vlan failed");
      continue;
    }
    (*i)->set_vlan_allocated();//this will make sure this is deleted when vgige is destroyed
    set_vlans(*i, new_vlan);
  }

  //JP VIRTUALPORTS
  //set vlans for other links here too
  std::list<crd_link_ptr>::iterator li;
  for (li = links.begin(); li != links.end(); ++li)
    {
      if ((*li)->marked()) { continue;}
      //switch_vlan new_vlan = the_session_manager->add_vlan();
      if(!(*li)->allocate_vlan())//new_vlan == 0)
	{
	  write_log("session::commit(): warning: add_vlan failed");
	  continue;
	}

      //VLAN PROBLEM
      //(*li)->set_vlan(new_vlan);
      //set_vlans((*lit)->get_node1(), new_vlan);
    }
 
  std::string usage = "USAGE_LOG: COMMIT user " + user + " session " + id + " components"; 
  for(i=components.begin(); i!=components.end(); ++i)
  {
    if(!((*i)->ignore()))
    {
      (*i)->initialize();
    }
    usage.append(" " + (*i)->getName());
  }

  write_usage_log(usage);
  write_mapping();
  return true;
}

void
session::link_vlanports_added(uint32_t lid)
{
//lock links? may have to be careful vlan_status may change as checking
  autoLock slock(share_lock);
  if (std::find(unreported_links.begin(), unreported_links.end(), lid) != unreported_links.end())
    unreported_links.remove(lid);
//if all links have reported
//send start session to nmd
//have all links return response;
  if (unreported_links.empty() && !vlans_created)
    {
      vlans_created = true;
      slock.unlock();
      NCCP_StatusType status = NCCP_Status_Fine;
      if (!the_session_manager->start_session_vlans(id))
	status = NCCP_Status_Failed;
      //have links send response
      std::list<crd_link_ptr>::iterator li;
      for(li=links.begin(); li!=links.end(); ++li)
	{
	  (*li)->complete_initialization(status);
	}
    }
}

void
session::set_clusters()
{
  std::list<crd_component_ptr>::iterator cit;
  std::list<crd_component_ptr>::reverse_iterator cit2;
  for(cit = components.begin(); cit != components.end(); ++cit)
    {
      if (!(*cit)->getCluster().empty())
	{
	  write_log("session::set_clusters comp:" + (*cit)->getName() + " cluster:" + (*cit)->getCluster());
	  for(cit2 = components.rbegin(); cit2 != components.rend(); ++cit2)
	    {
	      if ((*cit)==(*cit2)) break;
	      if ((*cit2)->getCluster() == (*cit)->getCluster())
		{
		  (*cit)->setDependentComp((*cit2)->getName());
		  (*cit2)->setDependentComp((*cit)->getName());
		  break;
		}
	    }
	}
    }
  
}

void
session::set_vlans(crd_component_ptr comp, vlan_ptr vlan)
{
  std::list<crd_link_ptr> clinks;
  std::list<crd_link_ptr>::iterator l;

  comp->set_marked(true);
  comp->set_vlan(vlan);
    
  comp->get_links(clinks);
  for(l = clinks.begin(); l != clinks.end(); ++l)
  {
    if((*l)->marked()) { continue; }

    (*l)->set_marked(true);
    (*l)->set_vlan(vlan);
    
    crd_component_ptr other_end = (*l)->get_node1();
    if(other_end->getName() == comp->getName())
    {
      other_end = (*l)->get_node2();
    }  

    if(other_end->marked()) { continue; }

    if(other_end->get_type() == "vgige")
    {
      set_vlans(other_end, vlan);
    }
    else
    {
      other_end->set_marked(true);
    }
  }
}

void
session::clear()
{
  autoLockDebug rlock(req_lock, "session::clear(): req_lock");
  if(cleared) return;
  cleared = true;

  write_log("session::clear(): clearing session " + id);

  while(!cluster_reqs.empty())
  {
    session_add_cluster_req* req = cluster_reqs.front();
    cluster_reqs.pop_front();
    
    cluster_response* resp = new cluster_response(req, NCCP_Status_Failed);
    resp->set_more_message(true);
    resp->send();
    delete resp;
    delete req;
  } 

  while(!component_reqs.empty())
  {
    session_add_component_req* req = component_reqs.front();
    component_reqs.pop_front();
    
    component_response* resp = new component_response(req, NCCP_Status_Failed);
    resp->set_more_message(true);
    resp->send();
    delete resp;
    delete req;
  } 

  while(!link_reqs.empty())
  {
    session_add_link_req* req = link_reqs.front();
    link_reqs.pop_front();
    
    rlicrd_response* resp = new rlicrd_response(req, NCCP_Status_Failed);
    resp->set_more_message(true);
    resp->send();
    delete resp;
    delete req;
  } 

  // clear out the topology
  the_session_manager->return_resources(user, &topology);

  // clear vlans
  if (!the_session_manager->clear_session_vlans(id))
    write_log("session(" + id + ")::clear() session_manager clear vlans failed");

  // remove all active links
  std::list<crd_link_ptr>::iterator li;
  for(li=links.begin(); li!=links.end(); ++li)
  {
    crd_link_ptr lnk = *li;

    unsigned int id = lnk->get_component().getID();
    lnk->refresh();
    //need to make sure vlans get deleted here VLAN PROBLEM
    topology.remove_link(id);
  }

  // remove all active components
  std::list<crd_component_ptr>::iterator i;
  for(i=components.begin(); i!=components.end(); ++i)
  {
    crd_component_ptr comp = *i;

    unsigned int id = comp->get_component().getID();
    comp->reset_params();
    the_session_manager->remove_component(comp);
    if(!comp->ignore())
    {
      comp->refresh();
    }
    topology.remove_node(id);
  }

  // remove pending observation requests
  while(!observe_reqs.empty())
  {
    observe_session_req* req = (observe_session_req *)observe_reqs.front();
    observe_reqs.pop_front();

    observe_response* resp = new observe_response(req, "Experiment closing", Session_No_Err);
    resp->send();
    delete resp;
    delete req;
  }

  write_usage_log("USAGE_LOG: CLOSE user " + user + " session " + id);
  //write (empty) component mapping to file
  //write_mapping();
  clear_mapping();

  
  std::string cmd = "/usr/testbed/scripts/clear_exp.sh " + user + " " + id;
  int ret = system(cmd.c_str());

  if(ret < 0 || WEXITSTATUS(ret) != 1) write_log("session::clear() cmd failed: " + cmd);

  if(begin_req != NULL) { delete begin_req; }
  begin_req = NULL;
}

void
session::expired()
{
  if(begin_req == NULL) { return; }
  
  session_response* resp = new session_response(begin_req, "Reservation Expired", Session_Expired);
  resp->send();
  delete resp;
}

void
session::write_mapping()
{
  //write_log("session::write_mapping " + id + " for user " + user);
  if (mapping_written) return;
  std::list<crd_component_ptr>::iterator i;
  //check that all components are active and then write mapping
  for(i=components.begin(); i!=components.end(); ++i)
  {
    if((*i)->getVMID() > 0 && (*i)->getVMName().empty() && !((*i)->get_component().isExtDev()))
    {
      //write_log("session::write_mapping " + id + " for user " + user + " vm" + int2str((*i)->getVMID()) + " not done");
      return; 
    }
  }
  std::ofstream e_file(user_session_file.c_str(), std::ofstream::out);
  for(i=components.begin(); i!=components.end(); ++i)
  {
    crd_component_ptr comp = *i;
    std::string label = comp->get_component().getLabel();
    if(label != "")
    {
      size_t found;
      found = label.find_first_of(".");
      while(found != std::string::npos)
      {
        label.erase(found,1);
        found = label.find_first_of(".",found);
      }
      if (comp->getVMID() > 0)
	{
	  if (!((*i)->get_component().isExtDev()))
	    {
	      write_log("session::write_mapping " + id + " for user " + user + " writing: " + label + " " + comp->getVMName());
	      e_file << label << " " << comp->getVMName() << std::endl;
	    }
	}
      else
	{
	  write_log("session::write_mapping " + id + " for user " + user + " writing: " + label + " " + comp->getName());
	  e_file << label << " " << comp->getName() << std::endl;
	}
    }
  }
  e_file.close();
  mapping_written = true;
}

void
session::clear_mapping()
{
  std::ofstream e_file(user_session_file.c_str(), std::ofstream::out);
  e_file.close();
  mapping_written = false;
}

void *
onlcrd::session_send_alerts_wrapper(void* obj)
{
  session* sess = (session *)obj;
  sess->send_alerts();
  the_session_manager->remove_session(sess);
  return NULL;
}

void
session::send_alerts()
{
  struct timespec sleep_time, remaining_time;
  int ns_ret;

  bool sent_reservation_alert = false;

  the_dispatcher->register_periodic_rendezvous(begin_req);
  rli_ping* ping = new rli_ping(begin_req);

  while(!cleared)
  {
    int min_left = the_session_manager->reservation_time_left(user);
    if((!sent_reservation_alert) && (min_left <= reservation_alert_time))
    {
      session_response* alert = new session_response(begin_req, "Reservation Alert - less than " + int2str(min_left) + " minutes left", Session_Alert);
      alert->send();
      delete alert;
      sent_reservation_alert = true;
    }
    else if((sent_reservation_alert) && (min_left > reservation_alert_time))
    {
      sent_reservation_alert = false;
    }
    
    begin_req->clear_ack();
    ping->send();
    
    sleep_time.tv_sec = 60;
    sleep_time.tv_nsec = 0;
    while(((ns_ret = nanosleep(&sleep_time, &remaining_time)) != 0) && (errno == EINTR))
    {
      sleep_time.tv_sec = remaining_time.tv_sec;
      sleep_time.tv_nsec = remaining_time.tv_nsec;
    }
    if(ns_ret == -1)
    {
      write_log("nccp_connection::process_message(): warning: nanosleep failed");
    }

    if(cleared) { break; }

    if(!begin_req->acked())
    {
      write_log("session::send_alerts(): failed to get ACK from RLI");
      clear();
      break;
    }

    sleep_time.tv_sec = (rli_ping_period - 1)*60;
    sleep_time.tv_nsec = 0;
    while(((ns_ret = nanosleep(&sleep_time, &remaining_time)) != 0) && (errno == EINTR))
    {
      sleep_time.tv_sec = remaining_time.tv_sec;
      sleep_time.tv_nsec = remaining_time.tv_nsec;
    }
    if(ns_ret == -1)
    {
      write_log("nccp_connection::process_message(): warning: nanosleep failed");
    }
  }

  delete ping;
}

void
session::add_observer(std::string username)
{
  autoLockDebug slock(share_lock, "session::add_observer(): share_lock");
  std::list<std::string>::iterator it;
  for(it = allowed_observers.begin(); it != allowed_observers.end(); ++it)
  {
    if(*it == username) return;
  }
  allowed_observers.push_back(username);
}

bool
session::observable_by(std::string username)
{
  autoLockDebug slock(share_lock, "session::observable_by(): share_lock");
  std::list<std::string>::iterator it;
  for(it = allowed_observers.begin(); it != allowed_observers.end(); ++it)
  {
    if(*it == username) return true;
  }
  return false;
}

bool
session::observe(observe_session_req* req)
{
  if(!observable_by(req->getObserveData().getObserver()))
  {
    return false;
  }
 
  autoLockDebug slock(share_lock, "session::observe(): share_lock");
  std::list<observe_session_req *>::iterator it;
  for(it = observe_reqs.begin(); it != observe_reqs.end(); ++it)
  {
    if((*it)->getObserveData().getObserver() == req->getObserveData().getObserver())
    {
      return true;
    }
  }
  observe_reqs.push_back(req);
  slock.unlock();

  observe_response* resp = new observe_response(begin_req, "Observation Request", Session_Observe_Req, NCCP_Status_StillRemaining, req->getObserveData());
  resp->send();
  delete resp;
  return true;
}

void
session::grant_observe(grant_observe_req* req)
{
  autoLockDebug slock(share_lock, "session::grant_observe(): share_lock");
  std::list<observe_session_req *>::iterator it;
  for(it = observe_reqs.begin(); it != observe_reqs.end(); ++it)
  {
    if((*it)->getObserveData().getObserver() == req->getObserveData().getObserver())
    {
      break;
    }
  }
  if(it == observe_reqs.end())
  {
    return;
  }
  observe_session_req* oreq = *it;
  observe_reqs.erase(it);

  observe_response* resp = new observe_response(oreq, "", Session_No_Err, NCCP_Status_Fine, req->getObserveData());
  resp->send();
  delete resp;
  delete oreq;

  resp = new observe_response(req, "", Session_No_Err, NCCP_Status_Fine, req->getObserveData());
  resp->send();
  delete resp;
  
  return;
}
