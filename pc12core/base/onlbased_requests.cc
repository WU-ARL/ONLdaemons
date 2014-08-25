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

//#include "onlbased_userdata.h"
#include "onlbased_session_manager.h"
#include "onlbased_globals.h"
#include "onlbased_requests.h"
#include "onlbased_responses.h"

using namespace onlbased;
  
//locks for starting and connecting to a daemon
pthread_mutex_t start_lock;
pthread_mutex_t connection_lock;

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

  user = exp.getExpInfo().getUserName();

  session_ptr sptr = the_session_manager->getSession(exp.getExpInfo());
  NCCP_StatusType status = NCCP_Status_Fine;
  if(!init_params.empty() && !sptr)//(!spec_conn || spec_conn->isClosed()))
  {
    param p = init_params.front();
    if(p.getType() == string_param && p.getString() != "")
    {
      write_log("start_experiment_req::handle(): got specialization daemon: " + p.getString());
      //init_params.pop_front();
      using_spec_daemon = true;
      sptr = start_specialization_daemon(p.getString());
      if (!sptr)
      {
        status = NCCP_Status_Failed;
      }
      else
      {
        if(!connect_to_specialization_daemon(sptr))
        {
	  write_log("start_experiment_req::handle(): failed to connect to daemon: " + p.getString());
          status = NCCP_Status_Failed;
	  the_session_manager->removeSession(sptr);
	  //PROBLEM: what to do if subtype daemon gets started but isn't killed and we can't connect
        }
      }
    }
  }

  if (sptr && status == NCCP_Status_Fine)
    {
      std::string vmname = the_session_manager->getNewVMName();
      param p(vmname);
      autoLockDebug slock(sptr->session_lock, "start_experiment_req::handle(): sptr->session_lock");
      sptr->vms.push_back(vmname);
      slock.unlock();
      init_params.push_back(p);
      sleep (5);
      timeout = 300;
      if ((sptr->spec_conn == NULL || sptr->spec_conn->isClosed()) && !connect_to_specialization_daemon(sptr))
	{
	  status = NCCP_Status_Failed;
	  the_session_manager->removeSession(sptr);
	  //PROBLEM: what to do if subtype daemon gets started but isn't killed and we can't connect
	}
      else
	{
	  rendezvous orig_rend = rend;
	  nccp_connection* orig_conn = nccpconn;
	  set_connection(sptr->spec_conn);
	  bool success = send_and_wait();
	  rend = orig_rend;
	  set_connection(orig_conn);
	  if (!success)
	    {
	      status = NCCP_Status_Failed;
	    }
	}
    }
  crd_response* resp = new crd_response(this, status);
  resp->send();
  delete resp;

  return true;
}

session_ptr
start_experiment_req::start_specialization_daemon(std::string specd)
{
  struct stat specd_stat;
  session_ptr rtn_null;
  if(stat(specd.c_str(), &specd_stat) != 0)
  {
    return rtn_null;
  }

  //get a lock
  autoLockDebug stlock(start_lock, "start_experiment_req::start_specialization_daemon: start_lock");
  
  session_ptr sptr = the_session_manager->getSession(exp.getExpInfo());
  if (sptr) return sptr;
  else sptr = the_session_manager->addSession(exp.getExpInfo());
  int port = sptr->dport;//the_session_manager->getNewPort();
  write_log("start_experiment_req::start_specialization_daemon passing user: " + user + " using port " + int2str(port));
  if(fork() == 0)
  {    
    //figure out the number of arguments
    int argc = init_params.size();
    const char* argv[(argc+5)]; //arguments sent from RLI plus the user and port number
    std::list<param>::iterator pit;
    int i = 0;
    for (pit = init_params.begin(); pit != init_params.end(); ++pit)
      {
	argv[i] = pit->getString().c_str();
	++i;
      }
    argv[i] = "--onluser";
    argv[++i] = user.c_str();
    argv[++i] = "--onlport";
    argv[++i] = int2str(port).c_str();
    argv[++i] = (char*)NULL;
    execv(argv[0], (char* const*)argv);
    
    //execl(specd.c_str(), specd.c_str(), "--onluser", user.c_str(), (char *)NULL);
    exit(-1);
  }
  return sptr;
}

bool
start_experiment_req::connect_to_specialization_daemon(session_ptr sptr)
{
  //get connection lock
  autoLockDebug clock(connection_lock, "start_experiment_req::connect_to_specialization_daemon: connection_lock");
  if (sptr->spec_conn != NULL && !sptr->spec_conn->isClosed()) return true;
  sleep(2);
  for(int i=0; i<5; ++i)
  {
    try
    {
      if (sptr->spec_conn == NULL)
	sptr->spec_conn = new nccp_connection("localhost", sptr->dport);
      sptr->spec_conn->receive_messages(true);
    }
    catch(std::exception& e)
    {
      if (sptr->spec_conn != NULL) delete sptr->spec_conn; //JP added 4/2/12
      sptr->spec_conn = NULL;
      std::string e_msg(e.what());
      write_log("start_experiment_req::connect_to_specialization_daemon exception:" + e_msg);
    }
    write_log("start_experiment_req::connect_to_specialization_daemon");
    if(sptr->spec_conn != NULL) { return true; }
    
    sleep(5);
  }
  write_log("start_experiment_req::connect_to_specialization_daemon failed");

  return false;
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
  NCCP_StatusType status = NCCP_Status_Fine;
  if (exp.getExpInfo().getID() == "") //this is the crd sending a reboot to the machine
    {
      write_log("refresh_req::handle: about to send Fine response");
      
      crd_response* resp = new crd_response(this, NCCP_Status_Fine);
      resp->send();
      delete resp;
      
      // jdd: added 12/06/2010 to give the response time to get out before we reboot
      sleep(2);
      
      if (!testing)
	system("reboot");
      else //skip the reboot and send i_am_up
	{  
	  nccp_connection* crd_conn = NULL;
	  i_am_up* upmsg = NULL;
	  try
	    {
	      // cgw, read file to get crd host/port?
	      //crd_conn = new nccp_connection("10.0.1.2", Default_CRD_Port);
	      crd_conn = new nccp_connection("onlsrv", Default_CRD_Port);
	      i_am_up* upmsg = new i_am_up();
	      upmsg->set_connection(crd_conn);
	      upmsg->send();
	    }
	  catch(std::exception& e)
	    {
	      write_log("refresh_req::handle: warning: could not connect CRD");
	    }
	  if(upmsg) delete upmsg;
	  if(crd_conn) delete crd_conn;
	}
    }
  else //send refresh message to vm daemon
    {
      timeout = 300;
      session_ptr sptr = the_session_manager->getSession(exp.getExpInfo());
      if (!sptr || !sptr->spec_conn || sptr->spec_conn->isClosed())
	{
	  status = NCCP_Status_Failed;
	}
      else
	{
	  rendezvous orig_rend = rend;
	  nccp_connection* orig_conn = nccpconn;
	  set_connection(sptr->spec_conn);
	  bool success = send_and_wait();
	  rend = orig_rend;
	  set_connection(orig_conn);
	  if (!success)
	    {
	      status = NCCP_Status_Failed;
	      //probably need to clean up vmnames here
	    }
	  else
	    {
	      crd_endconfig_response* spec_resp = (crd_endconfig_response*) resp;
	      if (spec_resp->getStatus() != NCCP_Status_Fine)
		{
		  status = NCCP_Status_Failed;
		}
	      else
		{
		  std::string vname = spec_resp->getVMName().getCString();
		  the_session_manager->releaseVMname(vname);
		  status = NCCP_Status_Fine;
		}
	    }
	}
      crd_response* resp = new crd_response(this, status);
      resp->send();
      delete resp;
    }
  
  return true;
}

rli_relay_req::rli_relay_req(uint8_t *mbuf, uint32_t size): rli_request(mbuf, size)
{
  timeout = 300;
  write_log("rli_relay_req::rli_relay_req: got message to relay");
}

rli_relay_req::~rli_relay_req()
{
}

bool
rli_relay_req::handle()
{
  //session_ptr sptr = the_session_manager->getSession(nccp_conn);
  //if(!sptr || !sptr->spec_conn || sptr->spec_conn->isClosed())
  //{

  //PROBLEM:can't do this now because we don't have a way to match the RLI to the session
  //the RLI doesn't send an experiment id so we'll have to use the nccp_conn or change the
  //messaging
  rli_response* err = new rli_response(this, NCCP_Status_TimeoutRemote);
  err->send();
  delete err;
  
  if(periodic_message) { received_stop = true; }
  return true;
  //}
  /*
    rendezvous orig_rend = rend;
    bool orig_periodic_message = periodic_message;
    nccp_connection* orig_conn = nccpconn;
    
    periodic_message = false;
    set_connection(sptr->spec_conn);
    //write_log("rli_relay_req::handle 1 op:" + int2str(op) + " type:" + int2str(type)); //JP remove
    bool success = send_and_wait();
    //if (success)	
    // write_log("rli_relay_req::handle 2 op:" + int2str(op) + " type:" + int2str(type) + " return from send_and_wait success:true");
    //else
    // write_log("rli_relay_req::handle 2 op:" + int2str(op) + " type:" + int2str(type) + " return from send_and_wait success:false");
    
    rend = orig_rend;
    periodic_message = orig_periodic_message;
    set_connection(orig_conn);
    
    if(!success)
    {
    // send error response back
    rli_response* err = new rli_response(this, NCCP_Status_TimeoutRemote);
    err->send();
    delete err;
    }
    else
    {
    // got response (resp) to relay back to RLI
    rli_response* spec_resp = (rli_response*)resp;
    
    // snoop on response status, if not good and this is periodic, stop periodic
    if((spec_resp->getStatus() != NCCP_Status_Fine) && (periodic_message))
    {
    received_stop = true;
    }
    
    // send original response back to RLI
    rli_relay_resp* rr = new rli_relay_resp(spec_resp, this);
    rr->send();
    delete rr;
    delete resp;
    }
    //write_log("rli_relay_req::handle 3 op:" + int2str(op) + " type:" + int2str(type) + " done");
    return true;
  */
}

void
rli_relay_req::parse()
{
  request::parse();

  uint8_t* bytes = buf.getData();
  uint32_t len = buf.getSize();
  data.resize(len);
  uint32_t i = buf.getCurrentIndex();

  for(; i < len; ++i)
  {
    data << bytes[i];
  }
}

void
rli_relay_req::write()
{
  buf.reset();
  request::write();
  
  uint8_t* bytes = data.getData();
  for(uint32_t i=0; i<data.getSize(); ++i)
  {
    buf << bytes[i];
  }
}

crd_relay_req::crd_relay_req(uint8_t *mbuf, uint32_t size): crd_request(mbuf, size)
{
  write_log("crd_relay_req::crd_relay_req: got message to relay");
  crd_request::parse();
  buf.reset();
}

crd_relay_req::~crd_relay_req()
{
}

bool
crd_relay_req::handle()
{
  session_ptr sptr = the_session_manager->getSession(exp.getExpInfo());
  if(!sptr)
  {
    crd_response* resp = new crd_response(this, NCCP_Status_Fine);
    resp->send();
    delete resp;
    return true;
  }

  if(!sptr->spec_conn || sptr->spec_conn->isClosed())
  {
    crd_response* err = new crd_response(this, NCCP_Status_TimeoutRemote);
    err->send();
    delete err;

    if(periodic_message) { received_stop = true; }
    return true;
  }

  rendezvous orig_rend = rend;
  bool orig_periodic_message = periodic_message;
  nccp_connection* orig_conn = nccpconn;

  periodic_message = false;
  set_connection(sptr->spec_conn);
  bool success = send_and_wait();
  
  rend = orig_rend;
  periodic_message = orig_periodic_message;
  set_connection(orig_conn);

  if(!success)
  {
    // send error response back
    crd_response* err = new crd_response(this, NCCP_Status_TimeoutRemote);
    err->send();
    delete err;
  }
  else
  {
    // got response (resp) to relay back to CRD
    crd_response* spec_resp = (crd_response*)resp;
    
    // snoop on response status, if not good and this is periodic, stop periodic
    if((spec_resp->getStatus() != NCCP_Status_Fine) && (periodic_message))
    {
      received_stop = true;
    }
    
    // send original response back to CRD
    crd_relay_resp* rr = new crd_relay_resp(spec_resp, this);
    rr->send();
    delete rr;
    delete resp;
  } 
  return true;
} 

void
crd_relay_req::parse()
{
  request::parse();
  
  uint8_t* bytes = buf.getData();
  uint32_t len = buf.getSize();
  data.resize(len);
  uint32_t i = buf.getCurrentIndex();
  
  for(; i < len; ++i)
  {
    data << bytes[i];
  }
}

void
crd_relay_req::write()
{
  buf.reset();
  request::write();

  uint8_t* bytes = data.getData();
  for(uint32_t i=0; i<data.getSize(); ++i)
  {
    buf << bytes[i];
  }
}
