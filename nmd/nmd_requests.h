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

#ifndef _NMD_REQUESTS_H
#define _NMD_REQUESTS_H

namespace nmd
{

  class add_vlan_req : public add_vlan
  {
    public:
      add_vlan_req(uint8_t *mbuf, uint32_t size);
      add_vlan_req();
      ~add_vlan_req();

      bool handle();
  };

  class delete_vlan_req : public delete_vlan
  {
    public:
      delete_vlan_req(uint8_t *mbuf, uint32_t size);
      delete_vlan_req(switch_vlan vlan_id);
      ~delete_vlan_req();

      bool handle();
  };

  class add_to_vlan_req : public add_to_vlan
  {
    public:
      add_to_vlan_req(uint8_t *mbuf, uint32_t size);
      add_to_vlan_req(switch_vlan vlan_id, switch_port& p);
      ~add_to_vlan_req();

      bool handle();
  };

  class delete_from_vlan_req : public delete_from_vlan
  {
    public:
      delete_from_vlan_req(uint8_t *mbuf, uint32_t size);
      delete_from_vlan_req(switch_vlan vlan, switch_port& p);
      ~delete_from_vlan_req();

      bool handle();
  };

  class initialize_req : public initialize
  {
    public:
      initialize_req(uint8_t *mbuf, uint32_t size);
      initialize_req();
      ~initialize_req();

      bool handle();
  };

};

#endif // _NMD_REQUESTS_H
