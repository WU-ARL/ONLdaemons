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

#ifndef _NMD_GLOBALS_H
#define _NMD_GLOBALS_H

// in DEBUG mode, state change messages are written to 
// the log file rather than sent to the switches
//#define DEBUG
#define default_vlan 255

namespace nmd
{
  extern dispatcher* the_dispatcher;
  extern nccp_listener* rli_conn;
  extern vlan_set *vlans;
  extern switch_info_set *eth_switches;
}; // namespace nmd

using namespace nmd;

#endif // _NMD_GLOBALS_H

