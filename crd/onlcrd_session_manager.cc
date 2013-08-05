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
switch_vlan test_vlan = 0; //used when testing and don't want to connect to NMD

session_manager::session_manager()
{
  pthread_mutex_init(&vlan_lock, NULL);

  pthread_mutex_init(&port_lock, NULL);

  pthread_mutex_init(&comp_lock, NULL);

  pthread_mutex_init(&session_lock, NULL);
  next_session_id = 1;

  pthread_mutex_init(&reservation_lock, NULL);
  next_reservation_id = 1;

  pthread_mutex_init(&db_lock, NULL);
  database = new onl::onldb();

  //JP added 11/13/12 to force users to wait until we know about all testbed nodes
  initializing = true;
}

session_manager::~session_manager()
{
  pthread_mutex_destroy(&vlan_lock);

  pthread_mutex_destroy(&db_lock);

  pthread_mutex_destroy(&comp_lock);

  pthread_mutex_destroy(&session_lock);

  while(!active_reservations.empty())
  {
    delete active_reservations.front();
    active_reservations.pop_front();
  } 
  pthread_mutex_destroy(&reservation_lock);
}

void
session_manager::initialize()
{
  std::string cmd = "/usr/testbed/scripts/send_crd_email.pl";
  int ret = system(cmd.c_str());
  if(ret < 0 || WEXITSTATUS(ret) != 1) write_log("session_manager::initialize(): Warning: starting email was not sent");

  if(!testing)
  {
    database->clear_all_experiments();
  }

  if(!testing)
  {
    onld::initialize* init = new onld::initialize();

    onl::switch_info_list switch_list;
    database->get_switch_list(switch_list);
    while(!switch_list.empty())
    {
      onl::switch_info sw = switch_list.front();
      switch_list.pop_front();

      onld::switch_info nextsw(sw.get_switch(), sw.get_num_ports());
      nextsw.set_management_port(sw.get_mgmt_port());
      init->add_switch(nextsw);
    }

    init->set_connection(the_gige_conn);
    if(!init->send_and_wait())
    {
      write_log("session_manager::initialize(): NMD initialization failed! exiting.");
      exit(1);
    }
    switch_response *swresp = (switch_response*)init->get_response();
    if(swresp->getStatus() != NCCP_Status_Fine)
    {
      write_log("session_manager::initialize(): warning!! NMD initialization returned failure!");
    }
    delete swresp;
    delete init;
  }

  onl::node_info_list node_list;
  database->get_base_node_list(node_list);

  autoLockDebug clock(comp_lock, "session_manager::initialize(): comp_lock");
  while(!node_list.empty())
  {
    onl::node_info node = node_list.front();
    node_list.pop_front();
    
    if(node.state() == "free")
    {
      std::string cp = "";
      unsigned short cp_port = 0;
      if(node.has_cp())
      {
        cp = node.cp();
        cp_port = node.cp_port();
      }

      crd_component_ptr new_comp(new crd_component(node.node(), cp, cp_port, node.do_keeboot(), node.is_dependent()));
      if(!new_comp->ignore())
      {
        refreshing_components.push_back(new_comp);
        new_comp->refresh();
      }
    }
  }
  clock.unlock();
}

bool
session_manager::authenticate(std::string username, std::string password)
{
  write_log("session_manager::authenticate(): user " + username + " with password hash " + password);
  autoLockDebug dlock(db_lock, "session_manager::authenticate(): db_lock");

  onl::onldb_resp res = database->authenticate_user(username,password);
  if(res.result() < 1)
  {
    return false;
  }
  return true;
}

bool
session_manager::has_session(std::string username)
{
  write_log("session_manager::has_session(): user " + username);
  autoLockDebug slock(session_lock, "session_manager::has_session(): session_lock");

  std::list<session_ptr>::iterator it;
  for(it = active_sessions.begin(); it != active_sessions.end(); ++it)
  {
    if(((*it)->getUser() == username) && (!((*it)->is_cleared()))) { return true; }
  }
  return false;
}

int
session_manager::reservation_time_left(std::string username)
{
  autoLockDebug dlock(db_lock, "session_manager::reservation_time_left(): db_lock");

  onl::onldb_resp res = database->has_reservation(username);
  if(res.result() < 1)
  {
    return 0;
  }
  return res.result();
}

bool
session_manager::is_admin(std::string username)
{
  autoLockDebug dlock(db_lock, "session_manager::is_admin(): db_lock");

  onl::onldb_resp res = database->is_admin(username);
  if(res.result() < 1)
  {
    return false;
  }
  return true;
}

bool
session_manager::extend_reservation(std::string username, uint32_t minutes)
{
  write_log("session_manager::extend_reservation(): user " + username + " for " + int2str(minutes));
  autoLockDebug dlock(db_lock, "session_manager::extend_reservation(): db_lock");

  onl::onldb_resp res = database->extend_current_reservation(username, minutes);
  if(res.result() < 1)
  { 
    write_log("session_manager::extend_reservation(): failed: " + res.msg());
    return false;
  }
  return true; 
}

bool
session_manager::cancel_reservation(std::string username)
{
  write_log("session_manager::cancel_reservation(): user " + username);
  autoLockDebug dlock(db_lock, "session_manager::cancel_reservation(): db_lock");

  onl::onldb_resp res = database->cancel_current_reservation(username);
  if(res.result() < 1)
  {
    return false;
  }
  return true;
}

int
session_manager::get_capacity(std::string type, int port)
{
  //write_log("session_manager::get_capacity(): type=" + type + " port=" + int2str(port));
  autoLockDebug dlock(db_lock, "session_manager::get_capacity(): db_lock");

  onl::onldb_resp res = database->get_capacity(type, port);

  return res.result();
}

bool
session_manager::has_virtual_port(std::string type)
{
  //write_log("session_manager::get_capacity(): type=" + type + " port=" + int2str(port));
  autoLockDebug dlock(db_lock, "session_manager::is_vport(): db_lock");

  onl::onldb_resp res = database->has_virtual_port(type);

  if (res.result())
    return true;
  else
    return false;
}

void
session_manager::get_switch_ports(unsigned int cid, switch_port& p1, switch_port& p2)
{
  //write_log("session_manager::get_switch_ports(): cid=" + int2str(cid));
  autoLockDebug dlock(db_lock, "session_manager::get_switch_ports(): db_lock");

  onl::switch_port_info spi1, spi2;

  database->get_switch_ports(cid, spi1, spi2);

  p1 = switch_port(spi1.get_switch(),spi1.get_port(),spi1.is_interswitch_port());
  p2 = switch_port(spi2.get_switch(),spi2.get_port(),spi2.is_interswitch_port());
}

void
session_manager::fix_component(onl::topology* top, uint32_t id, std::string cp)
{
  write_log("session_manager::fix_component(): id=" + int2str(id) + " cp=" + cp);
  autoLockDebug dlock(db_lock, "session_manager::fix_component(): db_lock");

  database->fix_component(top, id, cp);
}

bool
session_manager::make_reservation(reservation* res, std::string username, std::string early_start, std::string late_start, uint32_t duration, onl::topology* top)
{
  write_log("session_manager::make_reservation(): user " + username);
  autoLockDebug dlock(db_lock, "session_manager::make_reservation(): db_lock");

  onl::onldb_resp resp = database->make_reservation(username, early_start, late_start, duration, top);
  res->setErrorMsg(resp.msg());
  if(resp.result() < 1)
  {
    return false;
  }
  return true;
}

bool
session_manager::assign_resources(session_ptr sess, std::string username, onl::topology* top)
{
  write_log("session_manager::assign_resources(): user " + username);
  //write_usage_log("USAGE_LOG: COMMIT user " + username);
  autoLockDebug dlock(db_lock, "session_manager::assign_resources(): db_lock");

  onl::onldb_resp res = database->assign_resources(username, top);
  if(res.result() < 1)
  {
    sess->setErrorMsg(res.msg());
    return false;
  }
  return true;
}

void
session_manager::return_resources(std::string username, onl::topology* top)
{
  write_log("session_manager::return_resources(): user " + username);
  //write_usage_log("USAGE_LOG: CLOSE user " + username);
  autoLockDebug dlock(db_lock, "session_manager::return_resources(): db_lock");

  database->return_resources(username, top);
}

void
session_manager::check_for_expired_sessions()
{
  std::list<std::string> users;

  autoLockDebug dlock(db_lock, "session_manager::check_for_expired_sessions(): db_lock");
  onl::onldb_resp res = database->get_expired_sessions(users);
  dlock.unlock();

  if(res.result() < 1)
  {
    write_log("session_manager::check_for_expired_sessions(): warning: db call failed: " + res.msg());
    return;
  }

  while(!users.empty())
  {
    std::string user = users.front();
    users.pop_front();
 
    session_ptr sess = get_session(user);
    sess->expired();
    sess->clear();
  }
}

void
session_manager::received_up_msg(std::string hostname)
{
  write_log("session_manager::received_up_msg(): host " + hostname);

  autoLockDebug clock(comp_lock, "session_manager::received_up_msg(): comp_lock");

  crd_component_ptr comp;
  std::list<crd_component_ptr>::iterator i;
  for(i=refreshing_components.begin(); i!=refreshing_components.end(); ++i)
  {
    if((*i)->getCP() == hostname)
    {
      comp = *i;
      break;
    }
  }

  if(!comp)
  {
    for(i=active_components.begin(); i!=active_components.end(); ++i)
    {
      if((*i)->getCP() == hostname)
      {
        comp = *i;
        break;
      }
    }
  }
  clock.unlock();

  // if comp was in the list, then let the refresher finish
  if(comp)
  {
    comp->got_up_msg();
    return;
  }

  // if comp wasn't in the list, then it must have been in an unknown state
  // but is now back up
  autoLockDebug dlock(db_lock, "session_manager::received_up_msg(): db_lock");
  onl::onldb_resp r = database->get_node_from_cp(hostname);
  if(r.result() <= 0)
  {
    write_log("session_manager::received_up_msg(): get node failed: " + r.msg());
    return;
  }

  // if we got the node, r.msg() has the node name
  std::string node = r.msg();
  
  // get the base node info
  onl::node_info info;
  r = database->get_node_info(node, false, info);
  if(r.result() < 0)
  {
    write_log("session_manager::received_up_msg(): get node info failed: " + r.msg());
    return;
  }

  // since the node is back up now, free it
  r = database->set_state(node, "free");
  if(r.result() <= 0)
  {
    write_log("session_manager::received_up_msg(): set state to free failed: " + r.msg());
    return;
  }
  dlock.unlock();

  // if the node uses keeboot, then we need to complete the keeboot. the
  // simplest way to ensure a clean state is to refresh the node completely
  if(info.do_keeboot())
  {
    std::string cp = "";
    unsigned short cp_port = 0;
    if(info.has_cp())
    {
      cp = info.cp();
      cp_port = info.cp_port();
    }

    crd_component_ptr new_comp(new crd_component(node, cp, cp_port, info.do_keeboot(), info.is_dependent()));
    if(!new_comp->ignore())
    {
      refreshing_components.push_back(new_comp);
      new_comp->refresh();
    }
  }
}

void
session_manager::set_keeboot_param(std::string name, std::string param)
{
  autoLockDebug clock(comp_lock, "session_manager::set_keeboot_param(): comp_lock");
  keeboot_params[name] = param;
}

bool
session_manager::check_keeboot_param(std::string name)
{
  autoLockDebug clock(comp_lock, "session_manager::check_keeboot_param(): comp_lock");
  std::map<std::string, std::string>::iterator it;
  it = keeboot_params.find(name); 
  if(it == keeboot_params.end())
  {
    return false;
  }
  return true;
}

std::string
session_manager::get_keeboot_param(std::string name)
{
  std::map<std::string, std::string>::iterator it;
  it = keeboot_params.find(name); 
  if(it == keeboot_params.end())
  {
    // if the user hasn't set an fs to boot, check for a file in the 
    // install directory with this nodes name.
    struct stat fb;
    std::string install_file = "/export/keeboot_kernels/install/" + name + ".fsa";
    if(lstat(install_file.c_str(), &fb) == 0)
    {
      if((fb.st_mode & S_IFMT) == S_IFLNK)
      {
        std::string install_fs = "install/" + name;
        return install_fs;
      }
    }
    return "";
    //std::string default_fs = type + "/default";
    //return default_fs;
  }
  std::string fs = it->second;
  keeboot_params.erase(it);
  return fs;
}

switch_vlan
session_manager::add_vlan()
{
  switch_vlan vlan = 0;
  if (!testing)
    {
      onld::add_vlan* addvlan = new onld::add_vlan();
      addvlan->set_connection(the_gige_conn);
      
      if(!addvlan->send_and_wait())
	{
	  delete addvlan;
	  return 0;
	}
      
      add_vlan_response *addresp = (add_vlan_response*)addvlan->get_response();
      if(addresp->getStatus() != NCCP_Status_Fine)
	{
	  delete addresp;
	  delete addvlan;
	  return 0;
	}
      
      vlan = addresp->getVlan();
      
      delete addresp;
      delete addvlan;
    }
  else
    {
      vlan = ++test_vlan;
    }
  autoLockDebug vlock(vlan_lock, "session_manager::add_vlan(): vlan_lock");
  num_ports_on_vlan[vlan] = 0;
  vlock.unlock();

  return vlan;
}

bool
session_manager::add_port_to_vlan(switch_vlan vlan, switch_port port)
{
  if (!testing)
    {
      add_to_vlan* addtovlan = new add_to_vlan(vlan, port);
      addtovlan->set_connection(the_gige_conn);
      
      if(!add_port_to_outstanding_list(port))
	{
	  write_log("session_manager::add_port_to_vlan(): giving up after port was always in outstanding list: " + port.getSwitchId() + " port " + int2str(port.getPortNum()));
	  delete addtovlan;
	  return false;
	}
      
      if(!addtovlan->send_and_wait())
	{
	  remove_port_from_outstanding_list(port);
	  delete addtovlan;
	  return false;
	}

      remove_port_from_outstanding_list(port);
      
      switch_response *swresp = (switch_response*)addtovlan->get_response();
      if(swresp->getStatus() != NCCP_Status_Fine)
	{
	  delete swresp;
	  delete addtovlan;
	  return false;
	}

      delete swresp;
      delete addtovlan;
    }
  else //testing
    remove_port_from_outstanding_list(port);


  autoLockDebug vlock(vlan_lock, "session_manager::add_port_to_vlan(): vlan_lock");
  num_ports_on_vlan[vlan] = num_ports_on_vlan[vlan] + 1;
  vlock.unlock();

  return true;
}

bool
session_manager::remove_port_from_vlan(switch_vlan vlan, switch_port port)
{
  bool last_port = false;

  autoLockDebug vlock(vlan_lock, "session_manager::remove_port_from_vlan(): vlan_lock");
  num_ports_on_vlan[vlan] = num_ports_on_vlan[vlan] - 1;
  if(num_ports_on_vlan[vlan] == 0) { last_port = true; }
  vlock.unlock();

  if (!testing)
    {
      delete_from_vlan* delfromvlan = new delete_from_vlan(vlan, port);
      delfromvlan->set_connection(the_gige_conn);
      
      if(!add_port_to_outstanding_list(port))
	{
	  write_log("session_manager::remove_port_from_vlan(): giving up after port was always in outstanding list: " + port.getSwitchId() + " port " + int2str(port.getPortNum()));
	  delete delfromvlan;
	  return false;
	}

      if(!delfromvlan->send_and_wait())
	{
	  remove_port_from_outstanding_list(port);
	  delete delfromvlan;
	  if(last_port && !delete_vlan(vlan))
	    {
	      write_log("session_manager::remove_port_from_vlan(): warning: delete vlan failed for vlan " + int2str(vlan));
	    }
	  return false;
	}

      remove_port_from_outstanding_list(port);
      
      switch_response *swresp = (switch_response*)delfromvlan->get_response();
      if(swresp->getStatus() != NCCP_Status_Fine)
	{
	  delete swresp;
	  delete delfromvlan;
	  if(last_port && !delete_vlan(vlan))
	    {
	      write_log("session_manager::remove_port_from_vlan(): warning: delete vlan failed for vlan " + int2str(vlan));
	    }
	  return false;
	}

      delete swresp;
      delete delfromvlan;
    }
  else //testing
    remove_port_from_outstanding_list(port);

  if(last_port && !delete_vlan(vlan))
  {
    write_log("session_manager::remove_port_from_vlan(): warning: delete vlan failed for vlan " + int2str(vlan));
  }
  return true;
}

bool
session_manager::delete_vlan(switch_vlan vlan)
{
  autoLockDebug vlock(vlan_lock, "session_manager::delete_vlan(): vlan_lock");
  num_ports_on_vlan.erase(vlan);
  vlock.unlock();

  if (!testing)
    {
      onld::delete_vlan* delvlan = new onld::delete_vlan(vlan);
      delvlan->set_connection(the_gige_conn);
      
      if(!delvlan->send_and_wait())
	{
	  delete delvlan;
	  return false;
	}
      
      switch_response *delresp = (switch_response*)delvlan->get_response();
      if(delresp->getStatus() != NCCP_Status_Fine)
	{
	  delete delresp;
	  delete delvlan;
	  return false;
	}
      
      delete delresp;
      delete delvlan;
    }
  return true;
}

bool
session_manager::add_port_to_outstanding_list(switch_port port)
{
  unsigned int num_attempts = 0;
  std::set<switch_port>::iterator pit;
  autoLockDebug plock(port_lock, "session_manager::add_port_to_outstanding_list(): port lock");
  pit = outstanding_ports.find(port);
  while(pit != outstanding_ports.end())
  {
    plock.unlock();

    num_attempts++;
    if(num_attempts == 60)
    {
      return false;
    }

    sleep(5);
    plock.lock();
    pit = outstanding_ports.find(port);
  }
  outstanding_ports.insert(port);
  plock.unlock();
  return true;
}

void
session_manager::remove_port_from_outstanding_list(switch_port port)
{
  autoLockDebug plock(port_lock, "session_manager::remove_port_from_outstanding_list(): port lock");
  outstanding_ports.erase(port);
  plock.unlock();
}

crd_component_ptr
session_manager::get_component(std::string name)
{
  crd_component_ptr comp;

  if(name.substr(0,5) == "vgige")
  {
    crd_component_ptr tmp(new crd_virtual_switch(name));
    return tmp;
  }

  autoLockDebug dlock(db_lock, "session_manager::get_component(): db_lock");
  onl::node_info info;
  onl::onldb_resp r = database->get_node_info(name, false, info);
  if(r.result() < 0)
  {
    write_log("session_manager::get_component(): failed: " + r.msg());
    return comp;
  }
  dlock.unlock();

  std::string cp = "";
  unsigned short cp_port = 0;
  if(info.has_cp())
  {
    cp = info.cp();
    cp_port = info.cp_port();
  }

  if(info.do_keeboot())
  {
    autoLockDebug clock(comp_lock, "session_manager::get_component(): comp_lock");
    keeboot_params.erase(name);
    clock.unlock();
  }

  crd_component_ptr new_comp(new crd_component(info.node(), cp, cp_port, info.do_keeboot(), info.is_dependent()));
  return new_comp;
}

void
session_manager::add_component(crd_component_ptr comp, session_ptr sess)
{
  comp->set_session(sess);
  if(comp->get_type() == "vgige")
  {
    return;
  }

  autoLockDebug clock(comp_lock, "session_manager::add_component(): comp_lock");
  std::list<crd_component_ptr>::iterator i;
  for(i=active_components.begin(); i!=active_components.end(); ++i)
  {
    if((*i)->getName() == comp->getName())
    {
      clock.unlock();
      session_ptr old_sess = (*i)->get_session();
      old_sess->expired();
      old_sess->clear();
      clock.lock();
      break;
    }
  }

  active_components.push_back(comp);
}

void
session_manager::remove_component(crd_component_ptr comp)
{
  if(comp->get_type() == "vgige")
  {
    return;
  }

  autoLockDebug clock(comp_lock, "session_manager::remove_component(): comp_lock");
  std::list<crd_component_ptr>::iterator i;
  for(i=active_components.begin(); i!=active_components.end();)
  {
    if((*i) == comp)
    {
      i = active_components.erase(i);
      refreshing_components.push_back(comp);
    }
    else
    {
      ++i;
    }
  }
}

void
session_manager::clear_component(crd_component* comp)
{
  if(comp->get_type() == "vgige")
  {
    comp->clear_session();
    return;
  }

  autoLockDebug clock(comp_lock, "session_manager::clear_component(): comp_lock");
  std::list<crd_component_ptr>::iterator i;
  for(i=refreshing_components.begin(); i!=refreshing_components.end(); ++i)
  {
    if((*i).get() == comp)
    {
      comp->clear_session();
      refreshing_components.erase(i);
      if (initializing && refreshing_components.empty())//JP added 11/13/12
	{
	  initializing = false;
	  write_log("session_manager::clear_component: all components processed. initializing phase of crd is over.");
	}
      return;
    }
  }

  // really should never get here..
  write_log("session_manager::clear_component: warning: calling clear on node " + comp->getName() + " which is not in the refreshing list");
  comp->clear_session();
}

session_ptr
session_manager::add_session(std::string username, nccp_connection* conn,begin_session_req* begin_req)
{
  autoLockDebug slock(session_lock, "session_manager::add_session(): session_lock");
  session_ptr sess(new session(next_session_id, username, conn));
  if (begin_req != NULL) sess->set_begin_request(begin_req);
  else write_log("session_manager::add_session user:" + username + " begin_req is NULL");
  next_session_id++;
  active_sessions.push_back(sess);
  return sess;
}

session_ptr
session_manager::get_session(nccp_connection* conn)
{
  autoLockDebug slock(session_lock, "session_manager::get_session(nccp_conn): session_lock");
  std::list<session_ptr>::iterator it;
  for(it = active_sessions.begin(); it != active_sessions.end(); ++it)
  {
    if(((*it)->getConnection() == conn) && !((*it)->is_cleared())) { return *it; }
  }
  return session_ptr();
}

session_ptr
session_manager::get_session(nccp_connection* conn, std::string sid)
{
  autoLockDebug slock(session_lock, "session_manager::get_session(nccp_conn): session_lock");
  std::list<session_ptr>::iterator it;
  for(it = active_sessions.begin(); it != active_sessions.end(); ++it)
  {
    if(((*it)->getConnection() == conn) && ((*it)->getID() == sid) && !((*it)->is_cleared())) { return *it; }
  }
  return session_ptr();
}

session_ptr
session_manager::get_session(std::string username)
{
  autoLockDebug slock(session_lock, "session_manager::get_session(string): session_lock");
  std::list<session_ptr>::iterator it;
  for(it = active_sessions.begin(); it != active_sessions.end(); ++it)
  {
    if(((*it)->getUser() == username) && !((*it)->is_cleared())) { return *it; }
  }
  return session_ptr();
}

void
session_manager::remove_session(session* sess)
{
  autoLockDebug slock(session_lock, "session_manager::remove_session(): session_lock");
  std::list<session_ptr>::iterator it;
  for(it = active_sessions.begin(); it != active_sessions.end(); ++it)
  {
    if((*it)->getID() == sess->getID())
    {
      write_log("session_manager::remove_session: removing session " + (*it)->getID() + " with ref count " + int2str((*it).use_count()));
      active_sessions.erase(it);
      break;
    }
  }
}

reservation*
session_manager::add_reservation(std::string username, nccp_connection* conn, begin_reservation_req* begin_req)
{
  autoLockDebug rlock(reservation_lock, "session_manager::add_reservation(): reservation_lock");
  reservation* res = new reservation(next_reservation_id, username, conn);
  if (begin_req != NULL) res->set_begin_request(begin_req);
  else write_log("session_manager::add_reservation user:" + username + " begin_req is NULL");
  next_reservation_id++;
  active_reservations.push_back(res);
  return res;
}

reservation*
session_manager::get_reservation(nccp_connection* conn)
{
  autoLockDebug rlock(reservation_lock, "session_manager::get_reservation(): reservation_lock");
  std::list<reservation *>::iterator it;
  for(it = active_reservations.begin(); it != active_reservations.end(); ++it)
  {
    if((*it)->getConnection() == conn) { return *it; }
  }
  return NULL;
}

reservation*
session_manager::get_reservation(nccp_connection* conn, std::string rid)
{
  autoLockDebug rlock(reservation_lock, "session_manager::get_reservation(): reservation_lock");
  std::list<reservation *>::iterator it;
  for(it = active_reservations.begin(); it != active_reservations.end(); ++it)
  {
    if((*it)->getConnection() == conn && (*it)->getID() == rid) { return *it; }
  }
  return NULL;
}

void
session_manager::remove_reservation(reservation* res)
{
  res->clear();

  autoLockDebug rlock(reservation_lock, "session_manager::remove_reservation(): reservation_lock");
  std::list<reservation *>::iterator it;
  for(it = active_reservations.begin(); it != active_reservations.end(); ++it)
  {
    if((*it)->getConnection() == res->getConnection())
    {
      active_reservations.erase(it);
      break;
    }
  }
  rlock.unlock();

  delete res;
}

void
session_manager::get_observable_sessions(std::string username, std::list<session_ptr>& list)
{
  autoLockDebug slock(session_lock, "session_manager::get_observable_sessions(): session_lock");
  std::list<session_ptr>::iterator it;
  for(it = active_sessions.begin(); it != active_sessions.end(); ++it)
  {
    if((*it)->observable_by(username)) { list.push_back(*it); }
  }
}
