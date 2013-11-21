//
// Copyright (c) 2009-2013 Mart Haitjema, Charlie Wiseman, Jyoti Parwatikar, John DeHart 
// and Washington University in St. Louis
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
//    limitations under the License.
//
//

#include "nmd_includes.h"

add_vlan_req::add_vlan_req(uint8_t *mbuf, uint32_t size) : add_vlan(mbuf, size)
{
}

add_vlan_req::add_vlan_req() : add_vlan()
{
}

add_vlan_req::~add_vlan_req()
{
}

bool add_vlan_req::handle()
{
  ostringstream msg;
  NCCP_StatusType status = NCCP_Status_Fine;

  switch_vlan vlan;

  if (!vlans->add_vlan(vlan)) status = NCCP_Status_Failed;
  
  msg << "add_vlan_req::handle() allocated vlan " << vlan;
  write_log(msg.str());

  add_vlan_response* resp = new add_vlan_response(this, status, vlan); 
  resp->send();
  delete resp;

  return true;
}

delete_vlan_req::delete_vlan_req(uint8_t *mbuf, uint32_t size) : delete_vlan(mbuf, size)
{
}

delete_vlan_req::delete_vlan_req(switch_vlan vlan_id) : delete_vlan(vlan_id)
{
}

delete_vlan_req::~delete_vlan_req()
{
}

bool delete_vlan_req::handle()
{
  ostringstream msg;
  NCCP_StatusType status = NCCP_Status_Fine;

  msg << "delete_vlan_req::handle() deleting vlan " << vlanid;
  write_log(msg.str());

  if (!vlans->delete_vlan(vlanid)) status = NCCP_Status_Failed;

  // SNMP message to delete VLAN

  switch_response* resp = new switch_response(this, status); 
  resp->send(); 
  delete resp;

  return true;
}

add_to_vlan_req::add_to_vlan_req(uint8_t *mbuf, uint32_t size) : add_to_vlan(mbuf, size)
{
}

add_to_vlan_req::add_to_vlan_req(switch_vlan vlan_id, switch_port &p) : add_to_vlan(vlan_id, p)
{
}

add_to_vlan_req::~add_to_vlan_req()
{
}

bool add_to_vlan_req::handle()
{
  ostringstream msg;
  NCCP_StatusType status = NCCP_Status_Fine;

  msg << "add_to_vlan_req::handle() adding " << port
      << " to vlan " << vlanid;
  write_log(msg.str());

  if (!vlans->add_to_vlan(vlanid, port)) status = NCCP_Status_Failed;
  // SNMP message to add port to VLAN

  switch_response* resp = new switch_response(this, status); 
  resp->send(); 
  delete resp;

  return true;
}

delete_from_vlan_req::delete_from_vlan_req(uint8_t *mbuf, uint32_t size) : delete_from_vlan(mbuf, size)
{
}

delete_from_vlan_req::delete_from_vlan_req(switch_vlan vlan, switch_port &p) : delete_from_vlan(vlan, p)
{
}

delete_from_vlan_req::~delete_from_vlan_req()
{
}

bool delete_from_vlan_req::handle()
{
  ostringstream msg;
  NCCP_StatusType status = NCCP_Status_Fine;

  msg << "delete_from_vlan_req::handle() removing " << port
      << " from vlan " << vlanid;
  write_log(msg.str());
  // SNMP message to remove port from VLAN

  if (!vlans->delete_from_vlan(vlanid, port)) status = NCCP_Status_Failed;

  switch_response* resp = new switch_response(this, status); 
  resp->send(); 
  delete resp;

  return true;
}

initialize_req::initialize_req(uint8_t *mbuf, uint32_t size) : initialize(mbuf, size)
{
}

initialize_req::initialize_req() : initialize()
{
}

initialize_req::~initialize_req()
{
}

// Mart: 6/17/10 
struct parms {
  std::string switch_id;
  unsigned int ret_status;
};

void *initialize_switch_thread(void *arg)
{
  struct parms *p = (struct parms *)arg;
  p->ret_status = NCCP_Status_Fine;
  ostringstream msg;   
  msg << "initialize_req::handle() initializing " << p->switch_id;
  write_log(msg.str());

  if (!initialize_switch(p->switch_id)) p->ret_status = NCCP_Status_Failed;

  pthread_exit((void *)p);
  return NULL;  
}


bool initialize_req::handle()
{
  NCCP_StatusType status = NCCP_Status_Fine;

  // Mart: 6/17/10
  // Lets spawn a thread to initialize each switch
  pthread_t *threads;
  struct parms *p;
  int n, i = 0;
  n = switches.size();
  threads = new pthread_t[n];
  p = new parms[n];
  // remember set of switches
  eth_switches->set_switches(switches);

  // loop through set of switches
  for (list<switch_info>::iterator iter = switches.begin();
       iter != switches.end(); iter++) {
    p[i].switch_id = iter->getSwitchId();
    i++;
  }

  // launch a new thread to initialize each switch
  for (i = 0; i < n; i++) {
    pthread_create(&threads[i], NULL, initialize_switch_thread, (void*)(&p[i])); 
  }

  // wait for all threads to complete and clean up
  for (i = 0; i < n; i++) {
    struct parms *pp;
    pthread_join(threads[i], (void**)&pp);
    if (pp->ret_status != NCCP_Status_Fine) {
      status = NCCP_Status_Failed;
      ostringstream msg;
      msg << "initialize_req:handle() failed to initialize switch " << pp->switch_id;
      write_log(msg.str());
    }
  }
  delete [] threads;
  delete [] p;
  
  if (status == NCCP_Status_Fine)//only initialize vlans if switch initialization was successful
    vlans->initialize_vlans();

  switch_response* resp = new switch_response(this, status); 
  resp->send(); 
  delete resp;

  return true;

} 


