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

crd_component::crd_component(std::string n, std::string c, unsigned short p, bool do_k, bool is_d)
{
  name = n;
  cp = c;
  cp_port = p;
  keeboot = do_k;
  is_dependent = is_d;
  dependent_comp = "";

  compreq = NULL;
  nccpconn = NULL;
  internal_state = "free";
  local_state = "new";
  needs_refresh = false;
  keebooting_now = false;
  received_up_msg = false;
  devip = "";

  vmid = 0;
  cores = 1;
  memory = 1;

  mark = false;

  init_timeout = 120;

  //pthread_mutex_init(&db_lock, NULL);
  pthread_mutex_init(&state_lock, NULL);
  pthread_mutex_init(&up_msg_mutex, NULL);
  pthread_cond_init(&up_msg_cond, NULL);

  try
    {
      database = new onl::onldb();
    }
  catch(std::exception& er)
    {
      write_log("crd_component::crd_component (" + int2str((unsigned long) this) + ", " + name + ") Error: getting database connection - " + er.what());
      database = NULL;
    }

  write_log("crd_component::crd_component (" + int2str((unsigned long) this) + ") : adding component " + name);
  //write_usage_log("USAGE_LOG: COMPONENT " + name);
}

crd_component::~crd_component()
{
  write_log("crd_component::~crd_component(" + int2str((unsigned long) this) + "): deleting component " + name);

  reset_params();

  if (database != NULL)
    delete database;

  if(nccpconn) 
  {
    nccpconn->setFinished();
    nccpconn = NULL;
  }

  //pthread_mutex_destroy(&db_lock);
  pthread_mutex_destroy(&state_lock);
  pthread_mutex_destroy(&up_msg_mutex);
  pthread_cond_destroy(&up_msg_cond);
}

void 
crd_component::setDependentComp(std::string str)
{
  if (!is_dependent) 
    {
      write_log("crd_component::setDependentComp for comp:" + name + " dependent:" + str);
      dependent_comp = str;
    }
}


void
crd_component::setCluster(std::string str)
{
  if (str == "NULL") cluster_name = "";
  else cluster_name = str;
}

std::string
crd_component::get_state()
{
  //autoLock dlock(db_lock);
  if (database == NULL)
    {
      try
	{
	  database = new onl::onldb();
	}
      catch(std::exception& er)
	{
	  write_log("crd_component::get_state(): database call failed for " + name + ": " + er.what());
	  database = NULL;
	  return "";
	}
    }
  onl::onldb_resp rv = database->get_state(name,false);
  if(rv.result() < 0)
  {
    write_log("crd_component::get_state(): database call failed for " + name + ": " + rv.msg());
    return "";
  }

  if(rv.msg() == "testing")
  {
    return internal_state;
  }
  return rv.msg();
}

std::string
crd_component::get_dependent_state()
{
  //return free if this is a dependent component or if this is a non-cluster component
  if (is_dependent || dependent_comp.empty()) return "free"; 
  //autoLock dlock(db_lock);
  if (database == NULL)
    {
      try
	{
	  database = new onl::onldb();
	}
      catch(std::exception& er)
	{
	  write_log("crd_component::get_dependent_state(): database call failed for " + name + ": " + er.what());
	  database = NULL;
	  return "";
	}
    }
  onl::onldb_resp rv = database->get_state(dependent_comp,false);
  if(rv.result() < 0)
  {
    write_log("crd_component::get_dependent_state(): database call failed for " + name + ": " + rv.msg());
    return "free";
  }

  return rv.msg();
}

void
crd_component::set_state(std::string s)
{
  if(s == "repair")
  {
    needs_refresh = false;
    if (!is_dependent && !testing)
      {
	std::string cmd = "/root/scripts/power_cycle_node.pl " + name;
	write_log("crd_component::set_state(): repair node " + cmd );
	int ret = system(cmd.c_str());
	if(ret < 0 || WEXITSTATUS(ret) != 1) write_log("crd_component::set_state(): Warning: power_cycle_node call failed for " + name );
      }
  }

  internal_state = s;

  //autoLock dlock(db_lock);
  if (database == NULL)
    {
      try
	{
	  database = new onl::onldb();
	}
      catch(std::exception& er)
	{
	  write_log("crd_component::set_state(): database call failed for " + name + ": " + er.what());
	  database = NULL;
	  return;
	}
    }
  onl::onldb_resp rv = database->set_state(name,s);
  if(rv.result() < 0)
  {
    write_log("crd_component::set_state(): database call failed for " + name + ": " + rv.msg());
  }
}

bool
crd_component::is_admin_mode()
{
  autoLockDebug slock(state_lock, "crd_component::is_admin_mode(): state_lock");
  std::string cur_state = get_state();
  slock.unlock();
  if(cur_state == "repair" || cur_state == "testing")
  {
    return true;
  }
  return false;
}

std::string
crd_component::get_type()
{
  //autoLock dlock(db_lock);
  if (database == NULL)
    {
      try
	{
	  database = new onl::onldb();
	}
      catch(std::exception& er)
	{
	  write_log("crd_component::get_type(): database call failed for " + name + ": " + er.what());
	  database = NULL;
	  return "";
	}
    }
  autoLockDebug slock(state_lock, "crd_component::get_type(): state_lock");
  onl::onldb_resp rv = database->get_type(name,false);
  if(rv.result() < 0)
  {
    write_log("crd_component::get_type(): database call failed for " + name + ": " + rv.msg());
    return "";
  }
  return rv.msg();
}

bool
crd_component::is_virtual()
{
  std::string tp = get_type();
  return (tp == "vgige" || tp == "vm");
}


void
crd_component::add_reboot_params(std::list<param>& params)
{
  reboot_params = params;
  if(!reboot_params.empty() && keeboot)
  {
    param p = reboot_params.front();
    if(p.getType() == string_param && p.getString() != "")
    {
      the_session_manager->set_keeboot_param(name, p.getString());
      reboot_params.pop_front();
    }
  }
}

void
crd_component::add_init_params(std::list<param>& params)
{
  init_params = params;
}

bool
crd_component::ignore()
{
  // check for /export/keeboot_kernels/ignore/nodename, and it present we should not respond to it
  struct stat fb;
  std::string ignore_file = "/export/keeboot_kernels/ignore/" + name;
  if(lstat(ignore_file.c_str(), &fb) == 0)
  {
    if((fb.st_mode & S_IFMT) == S_IFLNK || (fb.st_mode & S_IFMT) == S_IFREG)
    {
      write_log("crd_component::ignore: ignoring " + name);
      return true;
    }
  }
  return false;
}

void
crd_component::reset_params()
{
  reboot_params.clear();
  init_params.clear();
}

void
crd_component::add_link(crd_link_ptr l)
{
  links.push_back(l);
}

bool
crd_component::operator<(const crd_component& c) const
{
  std::string sid = "";
  if(sess) { sid = sess->getID(); }

  std::string csid = "";
  if(c.sess) { csid = c.sess->getID(); }

  if(name < c.name) { return true; }
  if(name > c.name) { return false; }
  if(sid < csid) { return true; }
  return false;
}

bool
crd_component::operator==(const crd_component& c)
{
  std::string sid = "";
  if(sess) { sid = sess->getID(); }

  std::string csid = "";
  if(c.sess) { csid = c.sess->getID(); }

  if(name == c.name && sid == csid) { return true; }
  return false;
}

void
crd_component::set_session(session_ptr s)
{
  if(sess)
  {
    clear_session();
  }
  sess = s;
}

void
crd_component::clear_session()
{
  if(sess)
  {
    sess.reset();
  }
}

void
crd_component::initialize()
{
  if(!compreq)
  {
    autoLockDebug slock(state_lock, "crd_component::initialize(): state_lock");
    local_state = "not_used";
    slock.unlock();
    cleanup_links(true);
    return;
  }

  pthread_t tid;
  pthread_attr_t tattr;
  if(pthread_attr_init(&tattr) != 0)
  {
    write_log("crd_component::initialize(): pthread_attr_init failed");
    cleanup_reqs(true);
    cleanup_links(true);
    return;
  }
  if(pthread_attr_setdetachstate(&tattr,PTHREAD_CREATE_DETACHED) != 0)
  {
    write_log("crd_component::initialize(): pthread_attr_setdetachstate failed");
    cleanup_reqs(true);
    cleanup_links(true);
    return;
  }
  if(pthread_create(&tid, &tattr, crd_component_do_initialize_wrapper, (void *)this) != 0)
  {
    write_log("crd_component::initialize(): pthread_create failed");
    cleanup_reqs(true);
    cleanup_links(true);
    return;
  }
}

void *
onlcrd::crd_component_do_initialize_wrapper(void *obj)
{
  crd_component* comp = (crd_component *)obj;
  comp->do_initialize();
  return NULL;
}

void
crd_component::do_initialize()
{
  autoLockDebug slock(state_lock, "crd_component::do_initialize(): state_lock");
  std::string cur_state = get_state();//if shared then we're shifting from a vm to a non-vm status and we need to reboot
  std::string dependent_state = get_dependent_state();
  write_log("crd_component::initialize(" + int2str((unsigned long) this) + "): node " + name + " state = " + cur_state + " local_state = " + local_state + " dependent_state:" + dependent_state);

  //if (cur_state == "shared")
  //{
  //set_db_state("refreshing");
  //do_refresh();
  //}
  //cur_state = get_state();


  while(cur_state != "free" && cur_state != "repair" &&  local_state == "new")
    {
      slock.unlock();
      sleep(1);
      slock.lock();
      cur_state = get_state();
    }
  
  if(cur_state == "repair")
    {
      if(local_state != "repair")
	{
	  local_state = "repair";
	  slock.unlock();
	  cleanup_links(true);
	  cleanup_reqs(true);
	  return;
	}
      slock.unlock();
      the_session_manager->clear_component(this);
      return;
    }
  //if this is the dominant side of a cluster make sure the dependent is free 
  //if the dependent is anything but free. wait for a time and then refresh
  int i = 0;
  while(dependent_state == "refreshing" && i < 30)
    {
      sleep(1);
      dependent_state = get_dependent_state();
      ++i;
    }
  
  if (dependent_state == "refreshing" || dependent_state == "repair")
    {
      //slock.lock();
      //local_state = "refreshing";
      //needs_refresh = false;
      //set_state("refreshing");
      //slock.unlock();
      write_log("crd_component::initialize(): node " + name + " dependent_state:" + dependent_state + " refresh called due to dependent failure");
      keebooting_now = true; //refresh won't change the state
      do_refresh();
      keebooting_now = false;
      slock.lock();
      cur_state = get_state();
      slock.unlock();
      if(cur_state == "repair")
	{
	  the_session_manager->clear_component(this);
	  return;
	}
    }

  if(local_state != "new")
  {
    // this only happens if refresh() was called before we got to actually initialize
    slock.unlock();
    cleanup_reqs(false);
    cleanup_links(false);
    the_session_manager->clear_component(this);
    return;
  }
  
  local_state = "initializing";
  set_state("initializing");
  slock.unlock();

  // if the keeboot param isn't set then either the default was booted correctly, or 
  // the node booted the chosen fs already, and the param was removed. etiher way,
  // that means that node is done booting into the correct fs
  // otoh, if the param is still set, then we have to reboot to pick up the fs
  if(!testing & keeboot && the_session_manager->check_keeboot_param(name))
  {
    keebooting_now = true;
    do_refresh();
    keebooting_now = false;

    slock.lock();
    cur_state = get_state();
    slock.unlock();
    if(cur_state == "repair")
    {
      the_session_manager->clear_component(this);
      return;
    }
  }

  bool comp_failed = false;
  //std::string vmname;

  if(hasCP() && !testing)
  {
    try
    {
      nccpconn = new nccp_connection(cp, cp_port);
      nccpconn->receive_messages(true);
    }
    catch(std::exception& e)
    {
      write_log("crd_component::do_initialize(): putting " + name + " into repair after failing to connect to the cp");
      cleanup_reqs(true);
      cleanup_links(true);
      return;
    }

    // have to build up experiment info to send
    experiment_info info;
    if(sess) 
    {
      info.setUserName(sess->getUser().c_str());
      info.setID(sess->getID().c_str());
    }
    experiment exp;
    exp.setExpInfo(info);
  
    start_experiment* startexp = new start_experiment(exp, comp, ip_addr);
    startexp->set_init_params(init_params);
    startexp->set_connection(nccpconn);
    startexp->set_cores(cores);
    startexp->set_memory(memory);
    startexp->set_timeout(init_timeout);

    if(!startexp->send_and_wait())
    {
      write_log("crd_component::do_initialize(): putting " + name + " into repair after failing to get a response from the cp");
      delete startexp;
      cleanup_reqs(true);
      cleanup_links(true);
      return;
    }

    crd_response *startresp = (crd_response*)startexp->get_response();
    if(startresp->getStatus() != NCCP_Status_Fine)
    {
      comp_failed = true;
    }

    delete startresp;
    delete startexp;

    std::list<crd_link_ptr>::iterator lit;
    for(lit = links.begin(); lit != links.end(); lit++)
    {
      if(!((*lit)->send_port_configuration(this)))
      {
        comp_failed = true;
      }
      if ((*lit)->is_loopback()) //ADDED 9/2/2010 jp
      {
        if(!((*lit)->send_port_configuration(this, true)))
         {
           comp_failed = true;
         }
      }
    }

    end_configure_node* endconfig = new end_configure_node(exp, comp);
    endconfig->set_connection(nccpconn);
    endconfig->set_timeout(420);//mainly to give vms time to come up
    //bool no_endconfig = true;
    if (!endconfig->send_and_wait())
    {
      write_log("crd_component::do_initialize():  " + name + " failed to get a response from the cp for endconfig");
      comp_failed = true;
      //delete endconfig;
      //cleanup_reqs(true);
      //cleanup_links(true);
      //return;
    }
    else
    {
      crd_endconfig_response *endcfgresp = (crd_endconfig_response*)endconfig->get_response();
      if(endcfgresp->getStatus() != NCCP_Status_Fine)
	{
	  write_log("crd_component::do_initialize(): endconfig failed for " + name );
	  comp_failed = true;
	}
      
      vmname = endcfgresp->getVMName().getCString();
      delete endcfgresp;
    }
    delete endconfig;
  }

  component_response* resp;
  if(comp_failed)
  {
    resp = new component_response(compreq, NCCP_Status_Failed);
  }
  else
  {
    if (!testing)
      resp = new component_response(compreq, cp, cp_port, vmname);
    else
      resp = new component_response(compreq, cp, cp_port, vmname, NCCP_Status_Testing);
  }
  resp->send();
  delete resp;
  delete compreq;
  compreq = NULL;

  slock.lock();
  set_state("active");
  slock.unlock();

  if (sess != NULL) sess->write_mapping();

  while(!links.empty())
  {
    crd_link_ptr link = links.front();
    links.pop_front();

    if(link->both_comps_done())
    {
      link->initialize();
    }
  }

  if(needs_refresh)
  {
    refresh();
  }
}

void
crd_component::refresh()
{
  autoLockDebug slock(state_lock, "crd_component::refresh(): state_lock");
  std::string cur_state = get_state();
  write_log("crd_component::refresh(" + int2str((unsigned long) this) + "): node " + name + " state = " + cur_state + " local_state = " + local_state);
  if(local_state == "not_used")
  {
    // this is only set if the component was in an admin state to begin with, and so it was never initialized.  therefore we shouldn't do anything else here
    return;
  }
  if(cur_state == "repair")
  {
    if(local_state != "repair")
    {
      local_state = "repair";
      slock.unlock();
      cleanup_links(false);
      cleanup_reqs(false);
      the_session_manager->clear_component(this);// JP added 11/30/12 fix repair bug //shouldn't cause a problem if this is called twice
      return;
    }
    slock.unlock();
    the_session_manager->clear_component(this);
    return;
  }
  if(local_state == "new" && cur_state != "free") //JP is this a BUG? should it be cur_state == "free"
    //this is what happens if we're refreshing someone commits and then closes before the initial refresh ends
  {
    local_state = "done";
    slock.unlock();
    the_session_manager->clear_component(this);
    return;
  }
  if(cur_state == "initializing")
  {
    needs_refresh = true;
    return;
  }
  if(cur_state == "refreshing" || local_state == "refreshing") //the refreshing process has already started
    {
      needs_refresh = false;
      return;
    }
  local_state = "refreshing";
  set_state("refreshing");
  needs_refresh = false;
  slock.unlock();
  
  pthread_t tid;
  pthread_attr_t tattr;
  if(pthread_attr_init(&tattr) != 0)
  {
    write_log("crd_component::refresh(): pthread_attr_init failed");
    cleanup_reqs(true);
    cleanup_links(false);
    the_session_manager->clear_component(this);
    return;
  }
  if(pthread_attr_setdetachstate(&tattr,PTHREAD_CREATE_DETACHED) != 0)
  { 
    write_log("crd_component::refresh(): pthread_attr_setdetachstate failed");
    cleanup_reqs(true);
    cleanup_links(false);
    the_session_manager->clear_component(this);
    return;
  }
  if(pthread_create(&tid, &tattr, crd_component_do_refresh_wrapper, (void *)this) != 0)  
  { 
    write_log("crd_component::refresh(): pthread_create failed");
    cleanup_reqs(true);
    cleanup_links(false);
    the_session_manager->clear_component(this);
    return;
  }
}

void *
onlcrd::crd_component_do_refresh_wrapper(void *obj)
{
  crd_component* comp = (crd_component *)obj;
  comp->do_refresh();
  the_session_manager->clear_component(comp);
  return NULL;
}

void
crd_component::do_refresh()
{
  write_log("crd_component::do_refresh(" + int2str((unsigned long) this) + "): node " + name);

  bool dep_failed = false;

  if(hasCP() && !testing)
  { 
    //if(nccpconn != NULL && nccpconn->isClosed()) { delete nccpconn; nccpconn=NULL; }
    if(nccpconn != NULL && nccpconn->isFinished()) { delete nccpconn; nccpconn=NULL; }
    if(nccpconn == NULL)
    {
      try
      {
        nccpconn = new nccp_connection(cp, cp_port);
        nccpconn->receive_messages(true);
      }
      catch(std::exception& e)
      {
        if(is_dependent)
        {
          write_log("crd_component::do_refresh(): warning: couldn't contact dependent " + name);
          dep_failed = true;
        }
        else
        {
          write_log("crd_component::do_refresh(): putting " + name + " into repair after failing to connect to the cp");
          cleanup_reqs(true);
          cleanup_links(false);
          return;
        }
      }
    }
  
    // have to build up experiment info to send
    experiment_info info;
    if(sess)
    {
      info.setUserName(sess->getUser().c_str());
      info.setID(sess->getID().c_str());
    }
    experiment exp("10.0.1.2", Default_CRD_Port, info);
    
    onld::refresh* ref = new onld::refresh(exp, comp);
    ref->set_reboot_params(reboot_params);
    ref->set_connection(nccpconn);

    if(is_dependent)
    {
      if(!dep_failed)
      {
        if(!ref->send_and_wait())
        {
          write_log("crd_component::do_refresh(): warning: did not get response to refresh message for dependent " + name);
        }
      }
      delete ref;
    }
    else
    {
      if(!ref->send_and_wait())
      {
        write_log("crd_component::do_refresh(): putting " + name + " into repair after not getting a response to the refresh message");
        delete ref;
        cleanup_reqs(true);
        cleanup_links(false);
        return;
      }

      crd_response *refresp = (crd_response*)ref->get_response();
      if(refresp->getStatus() != NCCP_Status_Fine)
      {
        write_log("crd_component::do_refresh(): warning: got a failed response for " + name);
      }

      delete refresp;
      delete ref;
    }
  }
  
  if(hasCP() && !testing && get_type() != "vm")
  {
    if(nccpconn != NULL) { nccpconn->setFinished(); nccpconn=NULL; }
    if(!wait_for_up_msg(900))
    {
      write_log("crd_component::do_refresh(" + int2str((unsigned long) this) + "): putting " + name + " into repair after not getting the up message from the cp");
      cleanup_reqs(true);
      cleanup_links(false);
      return;
    }
    
    //if this is the dominant component of a clustered pair make sure the dependent also comes up if not do_refresh again
    std::string dependent_state = get_dependent_state();
    int i = 0;
    while((dependent_state == "refreshing" || dependent_state == "repair") && i < 30)
    {
      sleep(1);
      dependent_state = get_dependent_state();
      ++i;
    }    
    if (dependent_state == "refreshing" || dependent_state == "repair")
    {
      write_log("crd_component::do_refresh(" + int2str((unsigned long) this) + "): node " + name + " refreshing due to dependent(" + dependent_comp + ") failure dependent_state:" + dependent_state);
      do_refresh();
    }


    if(keeboot)
    {
      try
      {
        nccpconn = new nccp_connection(cp, cp_port);
        nccpconn->receive_messages(true);
      }
      catch(std::exception& e)
      {
        write_log("crd_component::do_refresh(): putting " + name + " into repair after failing to connect to the cp after a reboot (before a keeboot)");
        cleanup_reqs(true);
        cleanup_links(false);
        return;
      }

      experiment_info info;
      if(sess)
      {
        info.setUserName(sess->getUser().c_str());
        info.setID(sess->getID().c_str());
      }
      experiment exp("10.0.1.2", Default_CRD_Port, info);

      std::string fs = the_session_manager->get_keeboot_param(name);
      start_experiment* keeboot = new start_experiment(exp, comp, ip_addr);
      std::list<param> keeboot_params;
      keeboot_params.push_back(param(fs));
      keeboot->set_init_params(keeboot_params);
      keeboot->set_connection(nccpconn);

      if(!keeboot->send_and_wait())
      {
        write_log("crd_component::do_refresh(): putting " + name + " into repair after failing to get a keeboot response");
        delete keeboot;
        cleanup_reqs(true);
        cleanup_links(false);
        return;
      }

      crd_response *keeresp = (crd_response*)keeboot->get_response();
      if(keeresp->getStatus() != NCCP_Status_Fine)
      {
        write_log("crd_component::do_refresh(): putting " + name + " into repair after getting a failed keeboot response");
        delete keeresp;
        delete keeboot;
        cleanup_reqs(true);
        cleanup_links(false);
        return;
      }
      delete keeresp;
      delete keeboot;

      if(nccpconn != NULL) { nccpconn->setFinished(); nccpconn=NULL; }
      if(!wait_for_up_msg(600))
      {
        write_log("crd_component::do_refresh(): putting " + name + " into repair after not getting the up message after a keeboot");
        cleanup_reqs(true);
        cleanup_links(false);
        return;
      }

      // whether or not we used an install image, go ahead and unlink it here
      std::string install_fs = "/export/keeboot_kernels/install/" + name + ".fsa";
      unlink(install_fs.c_str());

/*
      try
      {
        nccpconn = new nccp_connection(cp, cp_port);
        nccpconn->receive_messages(true);
      }
      catch(std::exception& e)
      {
        write_log("crd_component::do_refresh(): putting " + name + " into repair after failing to connect to the cp after a keeboot");
        cleanup_reqs(true);
        cleanup_links(false);
        return;
      }
*/
    }
  }

  if(!keebooting_now)
  {
    autoLockDebug slock(state_lock, "crd_component::do_refresh(): state_lock");
    set_state("free");
    slock.unlock();
  }
}

void
crd_component::got_up_msg()
{
  pthread_mutex_lock(&up_msg_mutex);
  received_up_msg = true;
  pthread_cond_signal(&up_msg_cond);
  pthread_mutex_unlock(&up_msg_mutex);
  write_log("crd_component::got_up_msg(" + int2str((unsigned long) this) + "): " + name + " set received_up_msg = true, calling pthread_cond_signal(&up_msg_cond)");
}

bool
crd_component::wait_for_up_msg(int timeout)
{
  struct timespec abstimeout;
  int wait_rc = 0;

  pthread_mutex_lock(&up_msg_mutex);

  clock_gettime(CLOCK_REALTIME, &abstimeout);
  abstimeout.tv_sec += timeout;

  while((!received_up_msg) && (wait_rc != ETIMEDOUT))
  {
    wait_rc = pthread_cond_timedwait(&up_msg_cond, &up_msg_mutex, &abstimeout);
  }

  write_log("crd_component::wait_for_up_msg(" + int2str((unsigned long) this) + "): " + name + " after while loop: wait_rc = " +  int2str(wait_rc) + " received_up_msg = " +  int2str(received_up_msg) + " ");

  received_up_msg = false;
  pthread_mutex_unlock(&up_msg_mutex);

  if(wait_rc == ETIMEDOUT) { return false; }
  return true;
}

void
crd_component::cleanup_reqs(bool failed)
{
  if(compreq)
  {
    if(failed)
    {
      component_response* resp = new component_response(compreq, NCCP_Status_Failed);
      resp->send();
      delete resp;
    }
    delete compreq;
    compreq = NULL;
  }

  if(failed)
  {
    autoLockDebug slock(state_lock, "crd_component::cleanup_reqs(): state_lock");
    bool do_refresh = needs_refresh;
    set_state("repair");
    local_state = "repair";
    slock.unlock();
    if(do_refresh && !keebooting_now && !ignore()) { refresh(); }
  }
}

void
crd_component::cleanup_links(bool do_init)
{
  if(do_init)
  {
    while(!links.empty())
    {
      crd_link_ptr link = links.front();
      links.pop_front();

      if(link->both_comps_done())
      {
        link->initialize();
      }
    }
    return;
  }

  links.clear();
}

crd_virtual_switch::crd_virtual_switch(std::string n): crd_component(n, "", 0, false)
{
  internal_state = "free";
  vlan_ptr null_vlan(new crd_vlan());
  null_vlan->vlanid = 0;
  vlan = null_vlan;
  alloc_vlan = false;
}

crd_virtual_switch::~crd_virtual_switch()
{
}

std::string
crd_virtual_switch::get_type()
{
  return "vgige";
}

std::string
crd_virtual_switch::get_state()
{
  return internal_state;
}

void
crd_virtual_switch::do_initialize()
{
  write_log("crd_virtual_switch::do_initialize(): node " + name);

  NCCP_StatusType status = NCCP_Status_Fine;

  if(vlan->vlanid == 0)
  {
    status = NCCP_Status_Failed;
  }

  component_response* resp = new component_response(compreq, status);
  resp->send();
  delete resp;
  delete compreq;
  compreq = NULL;

  internal_state = "active";

  while(!links.empty())
  {
    crd_link_ptr link = links.front();
    links.pop_front();

    if(link->both_comps_done())
    {
      link->initialize();
    }
  }

  if(needs_refresh) { refresh(); }
}

void
crd_virtual_switch::refresh()
{
  pthread_t tid;
  pthread_attr_t tattr;
  if(pthread_attr_init(&tattr) != 0)
  {
    write_log("crd_virtual_switch::refresh(): pthread_attr_init failed");
    cleanup_reqs(true);
    cleanup_links(false);
    the_session_manager->clear_component(this);
    links.clear();
    return;
  }
  if(pthread_attr_setdetachstate(&tattr,PTHREAD_CREATE_DETACHED) != 0)
  { 
    write_log("crd_virtual_switch::refresh(): pthread_attr_setdetachstate failed");
    cleanup_reqs(true);
    cleanup_links(false);
    the_session_manager->clear_component(this);
    links.clear();
    return;
  }
  if(pthread_create(&tid, &tattr, crd_component_do_refresh_wrapper, (void *)this) != 0)  
  { 
    write_log("crd_virtual_switch::refresh(): pthread_create failed");
    cleanup_reqs(true);
    cleanup_links(false);
    the_session_manager->clear_component(this);
    links.clear();
    return;
  }
}

void
crd_virtual_switch::do_refresh()
{
  write_log("crd_virtual_switch::do_refresh(): node " + name);

  internal_state = "free";
    if(alloc_vlan && vlan->vlanid != 0)
    {
      write_log("crd_virtual_switch::do_refresh(): component " + comp.getLabel() + " delete vlan " + int2str(vlan->vlanid));
      std::string sid;
      if (sess) sid = sess->getID();
      if(!the_session_manager->delete_vlan(vlan, sid))
      {
        write_log("crd_virtual_switch::do_refresh(): warning: delete vlan failed for " + int2str(vlan->vlanid) + " for experiment " + sid);
      }
    }
}

crd_virtual_machine::crd_virtual_machine(unsigned int vm, std::string n, std::string c, unsigned short p, bool do_k, bool is_d): crd_component(n, c, p, do_k, is_d)
{
  internal_state = "free";
  vmid = vm;
  init_timeout = 300;
}

crd_virtual_machine::~crd_virtual_machine()
{
}

std::string
crd_virtual_machine::get_type()
{
  return "vm";
}

std::string
crd_virtual_machine::get_state()
{
  return internal_state;
}


void
crd_virtual_machine::set_state(std::string s)
{
  if(s == "repair")
  {
    needs_refresh = false;
  }

  internal_state = s;
  //if (s == "active")// set the database to "shared"
}

crd_link::crd_link(crd_component_ptr e1, unsigned short e1p, crd_component_ptr e2, unsigned short e2p)
{
  p1.comp = e1;
  p1.port = e1p;
  p2.comp = e2;
  p2.port = e2p;

  pthread_mutex_init(&state_lock, NULL);
  state = "new";
  needs_refresh = false;

  linkreq = NULL;
  pthread_mutex_init(&done_lock, NULL);
  num_comps_done = 0;

  vlan_ptr null_vlan(new crd_vlan());
  null_vlan->vlanid = 0;
  link_vlan = null_vlan;
  alloc_vlan = true;

  mark = false;
  vlan_status = NCCP_Status_OperationIncomplete;
  write_log("crd_link::crd_link: adding link");
}

crd_link::~crd_link()
{
  write_log("crd_link::~crd_link: deleting link");
  if(linkreq) { delete linkreq; }
  pthread_mutex_destroy(&state_lock);
  pthread_mutex_destroy(&done_lock);
}

bool
crd_link::send_port_configuration(crd_component* c, bool use2)
{
  //JP SWROUTER will need to send the actual port assigned if the port is a virtual port
  autoLockDebug slock(state_lock, "crd_link::send_port_configuration(): state_lock");
  if(state != "new" ) { return true; }
  slock.unlock();

  experiment_info info;
  if(sess)
  {
    info.setUserName(sess->getUser().c_str());
    info.setID(sess->getID().c_str());
  }
  else 
  {
    write_log("crd_link::send_port_configuration() error no session: comp " + c->getName() + ", link " + int2str(comp.getID()));
    return false;
  }

  // if (link_vlan)
  //   write_log("crd_link::send_port_configuration(): comp (" + c->getName() + ","+ int2str(c->get_component().getID()) + ") link " + int2str(comp.getID()) + ", vlan " + int2str(link_vlan->vlanid) + " endpoint1(" + p1.comp->getName() + "," + int2str(p1.comp->get_component().getID()) + ") endpoint2(" + p2.comp->getName() + "," + int2str(p2.comp->get_component().getID()) + ")");
  // else
  //   write_log("crd_link::send_port_configuration(): comp (" + c->getName() + ","+ int2str(c->get_component().getID()) + ") link " + int2str(comp.getID()) + ", no vlan endpoint1(" + p1.comp->getName() + "," + int2str(p1.comp->get_component().getID()) + ") endpoint2(" + p2.comp->getName() + "," + int2str(p2.comp->get_component().getID()) + ")");
  experiment exp;
  exp.setExpInfo(info);
  std::string myip="";

  configure_node* cfgnode;
  //compare component ideas could be 2 different vms on same machine
  if(c->getName() == p1.comp->getName() && !use2 && c->get_component().getID() == p1.comp->get_component().getID())
  {
    //cgw, this doesn't work they way i think it should, but i'll change it for now
    //node_info nodeinfo(linkreq->getFromIP(), endpoint1_port, endpoint2->get_type(), endpoint2->get_component().isRouter(), linkreq->getFromNHIP());
    if(p1.comp->get_component().isRouter())
    {
      myip = linkreq->getToNHIP();
    }
    else
    {
      myip = linkreq->getFromIP();
    }
    //node_info nodeinfo(myip, linkreq->getFromSubnet(), endpoint1_port, endpoint2->get_type(), endpoint2->get_component().isRouter(), linkreq->getFromNHIP(), endpoint1_rport);
    node_info nodeinfo(myip, linkreq->getFromSubnet(), p1.port, p1.rport, p1.comp->getDevIP());
    nodeinfo.setNextHop(p2.comp->get_type(), p2.comp->get_component().isRouter(), linkreq->getFromNHIP(), p2.comp->getDevIP(), p2.mac);
    nodeinfo.setVLan(link_vlan->vlanid);
    nodeinfo.setBandwidth(p1.cap);
    cfgnode = new configure_node(exp, p1.comp->get_component(), nodeinfo);
    cfgnode->set_connection(p1.comp->get_connection());
  
    if (link_vlan)
      write_log("crd_link::send_port_configuration(" + myip +"): comp (" + c->getName() + ","+ int2str(c->get_component().getID()) + ") link " + int2str(comp.getID()) + ", vlan " + int2str(link_vlan->vlanid) + " endpoint1(" + p1.comp->getName() + "," + int2str(p1.comp->get_component().getID()) + ") endpoint2(" + p2.comp->getName() + "," + int2str(p2.comp->get_component().getID()) + ")");
    else
      write_log("crd_link::send_port_configuration(" + myip +"): comp (" + c->getName() + ","+ int2str(c->get_component().getID()) + ") link " + int2str(comp.getID()) + ", no vlan endpoint1(" + p1.comp->getName() + "," + int2str(p1.comp->get_component().getID()) + ") endpoint2(" + p2.comp->getName() + "," + int2str(p2.comp->get_component().getID()) + ")");
  }
  //compare component ideas could be 2 different vms on same machine
  else if(c->getName() == p2.comp->getName() && c->get_component().getID() == p2.comp->get_component().getID())
    {
      //cgw, this doesn't work they way i think it should, but i'll change it for now
      //node_info nodeinfo(linkreq->getFromIP(), endpoint1_port, endpoint2->get_type(), endpoint2->get_component().isRouter(), linkreq->getFromNHIP());    std::string myip;
      if(p2.comp->get_component().isRouter())
	{
	  myip = linkreq->getFromNHIP();
	}
      else
	{
	  myip = linkreq->getToIP();
	}
      //node_info nodeinfo(myip, linkreq->getFromSubnet(), endpoint1_port, endpoint2->get_type(), endpoint2->get_component().isRouter(), linkreq->getFromNHIP(), endpoint1_rport);
      node_info nodeinfo(myip, linkreq->getToSubnet(), p2.port, p2.rport, p2.comp->getDevIP());
      nodeinfo.setNextHop(p1.comp->get_type(), p1.comp->get_component().isRouter(), linkreq->getToNHIP(), p1.comp->getDevIP(), p1.mac);
      nodeinfo.setVLan(link_vlan->vlanid);
      nodeinfo.setBandwidth(p2.cap);
      cfgnode = new configure_node(exp, p2.comp->get_component(), nodeinfo);
      cfgnode->set_connection(p2.comp->get_connection());
  
      if (link_vlan)
	write_log("crd_link::send_port_configuration(" + myip +"): comp (" + c->getName() + ","+ int2str(c->get_component().getID()) + ") link " + int2str(comp.getID()) + ", vlan " + int2str(link_vlan->vlanid) + " endpoint1(" + p1.comp->getName() + "," + int2str(p1.comp->get_component().getID()) + ") endpoint2(" + p2.comp->getName() + "," + int2str(p2.comp->get_component().getID()) + ")");
      else
	write_log("crd_link::send_port_configuration(" + myip +"): comp (" + c->getName() + ","+ int2str(c->get_component().getID()) + ") link " + int2str(comp.getID()) + ", no vlan endpoint1(" + p1.comp->getName() + "," + int2str(p1.comp->get_component().getID()) + ") endpoint2(" + p2.comp->getName() + "," + int2str(p2.comp->get_component().getID()) + ")");
    }
    
  // if(c->getName() == endpoint2->getName() && c->get_component().getID() == endpoint2->get_component().getID())
  // { 
  //   //cgw, this doesn't work they way i think it should, but i'll change it for now
  //   //node_info nodeinfo(linkreq->getToIP(), endpoint2_port, endpoint1->get_type(), endpoint1->get_component().isRouter(), linkreq->getToNHIP());
  //   std::string myip;
  //   if(endpoint2->get_component().isRouter())
  //   {
  //     myip = linkreq->getFromNHIP();
  //   }
  //   else
  //   {
  //     myip = linkreq->getToIP();
  //   }
  //   node_info nodeinfo(myip, linkreq->getToSubnet(), endpoint2_port, endpoint1->get_type(), endpoint1->get_component().isRouter(), linkreq->getToNHIP(), endpoint2_rport);
  //   nodeinfo.setVLan(link_vlan->vlanid);
  //   nodeinfo.setBandwidth(endpoint2_cap);
  //   cfgnode = new configure_node(exp, endpoint2->get_component(), nodeinfo);
  //   cfgnode->set_connection(endpoint2->get_connection());
  // }
  else
  {
    return false;
  }

  if(!cfgnode->send_and_wait(true))//true should print the msg length JP
  {
    delete cfgnode;
    return false;
  }

  crd_response *cfgresp = (crd_response*)cfgnode->get_response();
  if(cfgresp->getStatus() != NCCP_Status_Fine)
  {
    delete cfgresp;
    delete cfgnode;
    return false;
  }

  delete cfgresp;
  delete cfgnode;
  return true;
}

//ADDED 9/2/2010 jp
bool
crd_link::is_loopback()
{
  //added JP 10_5_15 fix returning loopback for different VMs on same machine

  if (p1.comp->getName() == p2.comp->getName() && p1.comp->get_component().getID() == p2.comp->get_component().getID())
    {
      write_log("crd_link::is_loopback loopback detected comp:" + int2str(comp.getID()) + " endpoint1(" + p1.comp->getName() + ", " + int2str(p1.comp->get_component().getID()) + ") endpoint2(" + p2.comp->getName() + ", " + int2str(p2.comp->get_component().getID()) + ")");
      return true;
    }

  return false;

  //return (endpoint1->getName() == endpoint2->getName());
}

bool
crd_link::both_comps_done()
{
  autoLockDebug dlock(done_lock, "crd_link::both_comps_done(): done_lock");
  num_comps_done++;
  if(num_comps_done == 2) return true;
  return false;
}

void
crd_link::set_switch_ports(std::list<int>& connlist)
{
  std::list<int>::iterator c;
  for(c = connlist.begin(); c != connlist.end(); ++c)
  {
    switch_port sp1, sp2;
    the_session_manager->get_switch_ports(*c, sp1, sp2);
    if(sp1.getSwitchId() != "")
    {
      switch_ports.push_back(sp1);
    }
    if(sp2.getSwitchId() != "")
    {
      switch_ports.push_back(sp2);
    }
    //PROBLEM: need to know if we're ignoring vports here to set the pass tag as false
  }
}

void
crd_link::set_session(session_ptr s)
{
  if(sess)
  {
    clear_session();
  }
  sess = s;
}

void
crd_link::clear_session()
{
  if(sess)
  {
    sess.reset();
  }
}

void
crd_link::initialize()
{
  autoLockDebug slock(state_lock, "crd_link::initialize(): state_lock");
  write_log("crd_link::initialize(): link " + int2str(comp.getID()) + " state = " + state);
  if(state != "new") { return; }
  state = "initializing";
  slock.unlock();

  pthread_t tid;
  pthread_attr_t tattr;
  if(pthread_attr_init(&tattr) != 0)
  {
    write_log("crd_link::initialize(): pthread_attr_init failed");
    cleanup_reqs();
    return;
  } 
  if(pthread_attr_setdetachstate(&tattr,PTHREAD_CREATE_DETACHED) != 0)
  {
    write_log("crd_link::initialize(): pthread_attr_setdetachstate failed");
    cleanup_reqs();
    return;
  } 
  if(pthread_create(&tid, &tattr, crd_link_do_initialize_wrapper, (void *)this) != 0)
  {
    write_log("crd_link::initialize(): pthread_create failed");
    cleanup_reqs();
    return;
  } 
} 

void *
onlcrd::crd_link_do_initialize_wrapper(void *obj)
{
  crd_link* link = (crd_link *)obj;
  link->do_initialize();
  return NULL;
}

void
crd_link::do_initialize()
{
  write_log("crd_link::do_initialize(): link " + int2str(comp.getID()));

  NCCP_StatusType status = NCCP_Status_Fine;

  if(!testing)
  {
    if(alloc_vlan && link_vlan->vlanid == 0)
    {
      if (sess)
	link_vlan = the_session_manager->add_vlan(sess->getID());
      write_log("crd_link::do_initialize(): link " + int2str(comp.getID()) + " adding vlan " + int2str(link_vlan->vlanid));
      if(sess == NULL || link_vlan->vlanid == 0)
      {
        status = NCCP_Status_Failed;
      }
    }
    else if(link_vlan->vlanid == 0)
    {
      status = NCCP_Status_Failed;
    }

    if(link_vlan->vlanid != 0)
    {
      std::list<switch_port>::iterator port;
      for(port = switch_ports.begin(); port != switch_ports.end(); ++port)
      {
        if(!the_session_manager->add_port_to_vlan(link_vlan, *port, sess->getID()))
        {
          status = NCCP_Status_Failed;
          break;
        }
      }
      // if we didn't make it through the list, delete all the ports we didn't
      // get to, including the one that failed, so that we don't try to 
      // remove them later
      while(port != switch_ports.end())
      {
        port = switch_ports.erase(port);
      }
    }
  }

  vlan_status = status;
  if(needs_refresh) //if session is closed complete the initialization
    complete_initialization(vlan_status);
  else //else inform session and wait until all vlans are created by nmd
    sess->link_vlanports_added(comp.getID());
}


//called after nmd initializes all vlans for session at once.
void
crd_link::complete_initialization(NCCP_StatusType sess_st)
{
  NCCP_StatusType status = NCCP_Status_Fine;
  if (sess_st != NCCP_Status_Fine || vlan_status != NCCP_Status_Fine)
    status = NCCP_Status_Failed;
  rlicrd_response* resp = new rlicrd_response(linkreq, status);
  resp->send();
  delete resp;
  delete linkreq;
  linkreq = NULL;

  vlan_status = status;

  autoLockDebug slock(state_lock, "crd_link::complete_initialization(): state_lock");
  if(status == NCCP_Status_Fine)
  {
    state = "active";
  }
  else
  {
    state = "failed";
  }

  if(needs_refresh)
  { 
    slock.unlock();
    refresh();
  }
}

bool
crd_link::allocate_vlan()
{
  //  NCCP_StatusType status = NCCP_Status_Fine;

  if(!testing && alloc_vlan && link_vlan->vlanid == 0)
    {
      vlan_ptr tmp_ptr = link_vlan;
      if (sess)
	link_vlan = the_session_manager->add_vlan(sess->getID());
      write_log("crd_link::allocate_vlan(): link " + int2str(comp.getID()) + " adding vlan " + int2str(link_vlan->vlanid));
      if(!link_vlan || !sess)
	{
	  link_vlan = tmp_ptr;
	  //	  status = NCCP_Status_Failed;
	  return false;
	}
    }
  return true;
}

void
crd_link::refresh()
{
  autoLockDebug slock(state_lock, "crd_link::refresh(): state_lock");
  write_log("crd_link::refresh(): link " + int2str(comp.getID()) + " state = " + state);
  if(state == "failed" && link_vlan->vlanid == 0)
  {
    slock.unlock();
    clear_session();
    return;
  }
  if(state == "new" && link_vlan->vlanid == 0)
  {
    state = "refreshing";
    slock.unlock();
    clear_session();
    return;
  }
  if(state == "initializing")
  {
    needs_refresh = true;
    return;
  }
  if(state != "failed")
  {
    state = "refreshing";
  }
  slock.unlock();

  pthread_t tid;
  pthread_attr_t tattr;
  if(pthread_attr_init(&tattr) != 0)
  {
    write_log("crd_link::refresh(): pthread_attr_init failed");
    clear_session();
    return;
  }
  if(pthread_attr_setdetachstate(&tattr,PTHREAD_CREATE_DETACHED) != 0)
  {
    write_log("crd_link::refresh(): pthread_attr_setdetachstate failed");
    clear_session();
    return;
  }
  if(pthread_create(&tid, &tattr, crd_link_do_refresh_wrapper, (void *)this) != 0)
  {
    write_log("crd_link::refresh(): pthread_create failed");
    clear_session();
    return;
  }
}

void *
onlcrd::crd_link_do_refresh_wrapper(void *obj)
{
  crd_link* link = (crd_link *)obj;
  link->do_refresh();
  link->clear_session();
  return NULL;
}

void
crd_link::do_refresh()
{
  write_log("crd_link::do_refresh(): link " + int2str(comp.getID()));

  if(!testing)
  {
    /*  if(link_vlan->vlanid != 0)
    {
      std::list<switch_port>::iterator port;
      for(port = switch_ports.begin(); port != switch_ports.end(); ++port)
	{
	  if(!the_session_manager->remove_port_from_vlan(link_vlan, *port))
	    {
	      write_log("crd_link::do_refresh(): warning: delete from vlan failed for " + int2str(link_vlan->vlanid));
	    }
	}
	}*/

    if(alloc_vlan && link_vlan->vlanid != 0)
    {
      write_log("crd_link::do_refresh(): component " + comp.getLabel() + " delete vlan " + int2str(link_vlan->vlanid));
      std::string sid;
      if (sess) sid = sess->getID();
      if(!the_session_manager->delete_vlan(link_vlan, sid))
      {
        write_log("crd_link::do_refresh(): warning: delete vlan failed for " + int2str(link_vlan->vlanid) + " for experiment " + sid);
      }
    }
  }

  switch_ports.clear();
}

void
crd_link::cleanup_reqs()
{
  autoLockDebug slock(state_lock, "crd_link::cleanup_reqs(): state_lock");
  state = "failed";
  slock.unlock();

  if(linkreq != NULL)
  {
    rlicrd_response* resp = new rlicrd_response(linkreq, NCCP_Status_Failed);
    resp->send();
    delete resp;
    delete linkreq;
    linkreq = NULL;
  }
}
