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

#include "onlvmbased_userdata.h"
#include "onlvmbased_session.h"
#include "onlvmbased_session_manager.h"
#include "onlvmbased_globals.h"
#include "onlvmbased_requests.h"
#include "onlvmbased_responses.h"

using namespace onlvmbased;

start_experiment_req::start_experiment_req(uint8_t *mbuf, uint32_t size): start_experiment(mbuf, size)
{
}

start_experiment_req::~start_experiment_req()
{
}

bool
start_experiment_req::handle()
{
  write_log("start_experiment_req::handle() session:" + exp.getExpInfo().getID());
  NCCP_StatusType status = NCCP_Status_Fine;

  session_ptr sess_ptr = the_session_manager->addSession(exp.getExpInfo());

  if (sess_ptr)
    {
      std::list<param>::iterator pit;
      pit = (++init_params.begin());
      
      std::string ip_str = ipaddr.getCString();
      std::string pwd = pit->getString();
      ++pit;
      std::string img = pit->getString();
      if (!sess_ptr->addVM(comp, ip_str, cores, memory, pwd, img))
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
  user = exp.getExpInfo().getUserName();

  //std::string cmd = "/bin/mkdir -p /users";
  // write_log("start_experiment_req::handle(): request vm");
  

  crd_response* resp = new crd_response(this, status);
  resp->send();
  delete resp;
  write_log("start_experiment_req::handle() response sent resession:" + exp.getExpInfo().getID());

  return true;
}

//int
//start_experiment_req::system_cmd(std::string cmd)
//{
//write_log("start_experiment_req::system_cmd(): cmd = " + cmd);
//int rtn = system(cmd.c_str());
//if(rtn == -1) return rtn;
//return WEXITSTATUS(rtn);
//}

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
  write_log("refresh_req::handle: about to send Fine response removing vm");

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
      return true;
    }
  else if (!the_session_manager->deleteVM(comp, exp.getExpInfo()))
    {	
      write_log("refresh_req::handle failed to get session ptr for experiment");
    }

  crd_response* resp = new crd_response(this, NCCP_Status_Fine);
  resp->send();
  delete resp;

  return true;
}

configure_node_req::configure_node_req(uint8_t *mbuf, uint32_t size): configure_node(mbuf, size)
{
}

configure_node_req::~configure_node_req()
{
}

bool
configure_node_req::handle()
{
  write_log("configure_node_req::handle() interface " + int2str(node_conf.getPort()) + " with ipaddr:" + node_conf.getIPAddr() + " to physical port:" + int2str(node_conf.getRealPort()) + " vlan:" + int2str(node_conf.getVLan()));

  NCCP_StatusType status = NCCP_Status_Fine;
  session_ptr sess_ptr = the_session_manager->getSession(exp.getExpInfo());

  if (sess_ptr)
    {
      if (!sess_ptr->configureVM(comp, node_conf))
	{
	  status = NCCP_Status_Failed;
	  write_log("configure_node_req::handle() failed to configure vm");
	}
    }
  else
    {
      status = NCCP_Status_Failed;
      write_log("configure_node_req::handle() failed to get session pointer");
    }
  //conf->set_port_info(node_conf.getPort(), node_conf.getIPAddr(), node_conf.getSubnet(), node_conf.getNHIPAddr());

  crd_response* resp = new crd_response(this, status);
  resp->send();
  delete resp;

  write_log("configure_node_req::handle() response sent interface " + int2str(node_conf.getPort()) + " with ipaddr:" + node_conf.getIPAddr() + " to physical port:" + int2str(node_conf.getRealPort()) + " vlan:" + int2str(node_conf.getVLan()));
  return true;
}

end_configure_node_req::end_configure_node_req(uint8_t *mbuf, uint32_t size): end_configure_node(mbuf, size)
{
}

end_configure_node_req::~end_configure_node_req()
{
}

bool
end_configure_node_req::handle()
{
  write_log("end_configure_node_req::handle() session:" + exp.getExpInfo().getID() + " vm:" + comp.getLabel());

  session_ptr sess_ptr = the_session_manager->getSession(exp.getExpInfo());

  NCCP_StatusType status = NCCP_Status_Fine;
  vm_ptr vmp = sess_ptr->getVM(comp);
  if (sess_ptr)
    {
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

  write_log("end_configure_node_req::handle() response sent session:" + exp.getExpInfo().getID() + " vm:" + comp.getLabel());
  return true;
}

user_data_req::user_data_req(uint8_t *mbuf, uint32_t size): rli_request(mbuf, size)
{ 
  user_data = NULL;
} 

user_data_req::~user_data_req()
{ 
  if(user_data) { delete user_data; }
}   

bool
user_data_req::handle()
{
  write_log("user_data_req::handle():");

  rli_response* rliresp;

  try
  {
    if(user_data == NULL)
    {
      write_log("user_data_req::handle(): making new userdata instance for " + file + " field " + int2str(field));
      user_data = new userdata(file,field);
    }

    uint32_t val = user_data->read_last_value();
    rliresp = new rli_response(this, NCCP_Status_Fine, val);
  }
  catch(std::exception& e)
  {
    std::string msg = e.what();
    write_log("get_stats_preq_byte_req::handle(): got exception: " + msg);
    rliresp = new rli_response(this, NCCP_Status_Failed, msg);
    received_stop = true;
  }

  rliresp->send();
  delete rliresp;

  return true;
}

void
user_data_req::parse()
{
  rli_request::parse();

  file = params[0].getString();
  field = params[1].getInt();
}

user_data_ts_req::user_data_ts_req(uint8_t *mbuf, uint32_t size): rli_request(mbuf, size)
{
  user_data = NULL;
}

user_data_ts_req::~user_data_ts_req()
{
  if(user_data) { delete user_data; }
}

bool
user_data_ts_req::handle()
{
  write_log("user_data_req::handle():");

  try
  {
    if(user_data == NULL)
    {
      write_log("user_data_req::handle(): making new userdata instance for " + file + " field " + int2str(field));
      user_data = new userdata(file,field);
    }

    std::list<ts_data> vals;
    user_data->read_next_values(vals);
    while(!vals.empty())
    {
      ts_data data = vals.front();
      vals.pop_front();

      rli_response* rliresp = new rli_response(this, NCCP_Status_Fine, data.val, data.ts, data.rate);
      rliresp->send();
      delete rliresp;
    }
  }
  catch(std::exception& e)
  {
    std::string msg = e.what();
    write_log("get_stats_preq_byte_req::handle(): got exception: " + msg);
    received_stop = true;

    rli_response* rliresp = new rli_response(this, NCCP_Status_Failed, msg);
    rliresp->send();
    delete rliresp;
  }

  return true;
}

void
user_data_ts_req::parse()
{
  rli_request::parse();

  file = params[0].getString();
  field = params[1].getInt();
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
  if(!using_spec_daemon || !spec_conn || spec_conn->isClosed())
  {
    rli_response* err = new rli_response(this, NCCP_Status_TimeoutRemote);
    err->send();
    delete err;

    if(periodic_message) { received_stop = true; }
    return true;
  }

  rendezvous orig_rend = rend;
  bool orig_periodic_message = periodic_message;
  nccp_connection* orig_conn = nccpconn;

  periodic_message = false;
  set_connection(spec_conn);
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
}

crd_relay_req::~crd_relay_req()
{
}

bool
crd_relay_req::handle()
{
  if(!using_spec_daemon)
  {
    crd_response* resp = new crd_response(this, NCCP_Status_Fine);
    resp->send();
    delete resp;
    return true;
  }

  if(!spec_conn || spec_conn->isClosed())
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
  set_connection(spec_conn);
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
