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

// Network Management Daemon (NMD)
// Mart Haitjema - April, 2010
#include <boost/shared_ptr.hpp>
#include "nmd_includes.h"

namespace nmd
{
  dispatcher* the_dispatcher;
  nccp_listener* rli_conn;
  vlan_set *vlans;  
  switch_info_set *eth_switches;
};

int main()
{
  log = new log_file("/etc/ONL/nmd.log");
  the_dispatcher = dispatcher::get_dispatcher();
  rli_conn = NULL;
  eth_switches = new switch_info_set;

  // allocates the bitmap that keeps track of available vlans
  vlans = new vlan_set(253);

  try
  {
    std::string tmp_addr("127.0.0.1");
    rli_conn = new nccp_listener(tmp_addr, Default_ND_Port);
    //rli_conn = new nccp_listener("127.0.0.1", Default_NMD_Port);
  }
  catch(std::exception& e)
  {
    write_log(e.what());
    exit(1);
  }

  register_req<add_vlan_req>(NCCP_Operation_AddVlan);
  register_req<delete_vlan_req>(NCCP_Operation_DeleteVlan);
  register_req<add_to_vlan_req>(NCCP_Operation_AddToVlan);
  register_req<delete_from_vlan_req>(NCCP_Operation_DeleteFromVlan);
  register_req<initialize_req>(NCCP_Operation_Initialize);
  register_req<start_session_req>(NCCP_Operation_StartSession);

  rli_conn->receive_messages(false);

  pthread_exit(NULL);
}

