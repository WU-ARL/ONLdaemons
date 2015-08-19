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

#include <boost/shared_ptr.hpp>
#include "nmd_includes.h"

using std::cout;
using std::endl;

namespace nmd
{
  dispatcher* the_dispatcher;
  nccp_listener* rli_conn;
  vlan_set *vlans;  
  switch_info_set *eth_switches;
};

nccp_connection* the_nmd_conn;

switch_vlan send_add_vlan()
{
    add_vlan_response *vresp;
    switch_vlan retVlan;
    add_vlan *add_vlan_cmd = new add_vlan("client");
    add_vlan_cmd->set_connection(the_nmd_conn);
   
    if (!add_vlan_cmd->send_and_wait())
    {
      cout << "add vlan failed!" << endl;
      delete add_vlan_cmd;
      return 0;
    }
    vresp = (add_vlan_response*) add_vlan_cmd->get_response();
    cout << "add vlan returned status " << vresp->getStatus() <<  endl;
    if (vresp->getStatus() == NCCP_Status_Fine) {
        retVlan = vresp->getVlan();
	cout << "add_vlan returned vlan " << retVlan << endl;
    }
    delete add_vlan_cmd;
    return retVlan;
}

void send_delete_vlan(switch_vlan vlan)
{
  delete_vlan *delete_vlan_cmd = new delete_vlan(vlan, "client");
    delete_vlan_cmd->set_connection(the_nmd_conn);
    switch_response *resp;

    if (!delete_vlan_cmd->send_and_wait())
    {
      cout << "delete vlan failed!" << endl;
      delete delete_vlan_cmd;
      return;
    }
    resp = (switch_response*)delete_vlan_cmd->get_response();
    cout << "delete vlan returned " << resp->getStatus() << endl;
    delete delete_vlan_cmd;
}

void send_add_to_vlan(switch_vlan vlan, switch_port port)
{
  add_to_vlan *add_to_vlan_cmd = new add_to_vlan(vlan, port, "client");
    add_to_vlan_cmd->set_connection(the_nmd_conn);
    switch_response *resp;

    if (!add_to_vlan_cmd->send_and_wait())
    {
      cout << "add to vlan failed!" << endl;
      delete add_to_vlan_cmd;
      return;
    }
    resp = (switch_response*)add_to_vlan_cmd->get_response();
    cout << "add to vlan returned " << resp->getStatus() << endl;
    delete add_to_vlan_cmd;
}

void send_delete_from_vlan(switch_vlan vlan, switch_port port)
{
  delete_from_vlan *delete_from_vlan_cmd = new delete_from_vlan(vlan, port, "client");
    delete_from_vlan_cmd->set_connection(the_nmd_conn);
    switch_response *resp;

    if (!delete_from_vlan_cmd->send_and_wait())
    {
      cout << "delete from vlan failed!" << endl;
      delete delete_from_vlan_cmd;
      return;
    }
    resp = (switch_response*)delete_from_vlan_cmd->get_response();
    cout << "delete from vlan returned " << resp->getStatus() << endl;
    delete delete_from_vlan_cmd;
}

void send_initialize()
{
    initialize *initialize_cmd = new initialize();
    initialize_cmd->set_connection(the_nmd_conn);
    switch_info onlsw1("onlsw1", 52);
    onlsw1.set_management_port(47);
    switch_info onlsw2("onlsw2", 52);
    onlsw2.set_management_port(47);
    switch_info onlsw3("onlsw3", 52);
    onlsw3.set_management_port(47);
    switch_info onlsw4("onlsw4", 52);
    onlsw4.set_management_port(47);
    switch_info onlsw5("onlsw5", 52);
    onlsw5.set_management_port(47);
    switch_info onlsw6("onlsw6", 52);
    onlsw6.set_management_port(47);
    switch_info onlsw7("onlsw7", 24);
    switch_info onlsw8("onlsw8", 24);
    switch_info onlsw9("onlsw9", 24);
    switch_info onlsw10("onlsw10", 24);
    //initialize_cmd->add_switch(onlsw1);
    initialize_cmd->add_switch(onlsw1);
    initialize_cmd->add_switch(onlsw2);
    initialize_cmd->add_switch(onlsw3);
    initialize_cmd->add_switch(onlsw4);
    initialize_cmd->add_switch(onlsw5);
    initialize_cmd->add_switch(onlsw6);
    initialize_cmd->add_switch(onlsw7);
    initialize_cmd->add_switch(onlsw8);
    initialize_cmd->add_switch(onlsw9);
    initialize_cmd->add_switch(onlsw10);
    switch_response *resp;

    if (!initialize_cmd->send_and_wait())
    {
      cout << "initialize failed!" << endl;
      delete initialize_cmd;
      return;
    }
    resp = (switch_response*)initialize_cmd->get_response();
    cout << "initialize returned " << resp->getStatus() << endl;
    delete initialize_cmd;
}

int main() {

    the_dispatcher = dispatcher::get_dispatcher();
    the_nmd_conn = NULL;

    try
    {
    	the_nmd_conn = new nccp_connection("127.0.0.1", Default_NMD_Port);
    }
    catch(std::exception& e)
    {
      cout << "Could not create connection to network management daemon (nmd)!" << endl; 
      return 1;
    }
    the_nmd_conn->receive_messages(true);
    
    // configuration responses from switches
    //register_resp<switch_response>(NCCP_Operation_AddLink);
    //register_resp<switch_response>(NCCP_Operation_DeleteLink);
    register_resp<add_vlan_response>(NCCP_Operation_AddVlan);
    register_resp<switch_response>(NCCP_Operation_DeleteVlan);
    register_resp<switch_response>(NCCP_Operation_AddToVlan);
    register_resp<switch_response>(NCCP_Operation_DeleteFromVlan);
    register_resp<switch_response>(NCCP_Operation_Initialize);

    switch_port sw7p1("onlsw7",1);
    switch_port sw7p2("onlsw7",16);
    switch_port sw7p3("onlsw7",40);
    switch_port sw8p1("onlsw8",1);
    switch_port sw8p2("onlsw8",2);
    switch_port sw1p1("onlsw1",1);
    switch_port sw1p2("onlsw1",2);

    send_initialize();

//    switch_vlan vlan1 = send_add_vlan();
//    switch_vlan vlan2 = send_add_vlan();

//    send_add_to_vlan(vlan1, sw7p1);
  //  send_add_to_vlan(vlan1, sw1p1);
  //  send_add_to_vlan(vlan2, sw1p2);

//    send_delete_from_vlan(vlan1, sw7p1);
   // send_delete_from_vlan(vlan1, sw1p1);
   // send_delete_from_vlan(vlan2, sw1p2);
/*
    send_add_to_vlan(vlan1, sw8p1);
    send_add_to_vlan(vlan1, sw8p2);
    send_add_to_vlan(vlan1, sw7p2);
    send_add_to_vlan(vlan1, sw7p3);
    send_delete_from_vlan(vlan1, sw7p2);
*/
  
//    send_delete_vlan(vlan1);
//    send_delete_vlan(vlan2);


  return 0;
}

