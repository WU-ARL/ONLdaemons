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

#ifndef _NMD_SWITCH_H
#define _NMD_SWITCH_H

// All functions return true on success, false on failure

// Configures the switch state for switch switch_id
// This will set the specified vlan to include
// only the ports included in the specified list of ports
bool set_switch_vlan_membership(port_list switch_ports, string switch_id, switch_vlan vlan_id);

// This configures the specified switch to tag untagged fromaes with the specified vlan
// note this function will only does this for ports connected to hosts
bool set_switch_pvids(port_list switch_ports, string switch_id, switch_vlan vlan_id);

// Just like set_switch_pvids but only changes the configuration for a single port
// Note: management of PVIDs via Arista CLI is not independent
// of VLAN membership so ARISTA switches should always update
// all pvids at once
bool set_switch_pvid(switch_port port, string switch_id, switch_vlan vlan_id);

// clears all vlan state associated with the specified switch
bool initialize_switch(string switch_id);

#endif

