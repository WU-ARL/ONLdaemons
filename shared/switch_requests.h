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

#ifndef _SWITCH_REQUESTS_H
#define _SWITCH_REQUESTS_H

namespace onld
{
  static const NCCP_OperationType NCCP_Operation_AddVlan = 62;
  class add_vlan : public request
  {
    public:
      add_vlan(uint8_t *mbuf, uint32_t size);
      add_vlan();
      virtual ~add_vlan();

      virtual void parse();
      virtual void write();

    protected:
  }; // class add_vlan

  static const NCCP_OperationType NCCP_Operation_DeleteVlan = 63;
  class delete_vlan : public request
  {
    public:
      delete_vlan(uint8_t *mbuf, uint32_t size);
      delete_vlan(switch_vlan vlan);
      virtual ~delete_vlan();

      virtual void parse();
      virtual void write();

    protected:
      switch_vlan vlanid;
  }; // class delete_vlan

  static const NCCP_OperationType NCCP_Operation_AddToVlan = 64;
  class add_to_vlan : public request
  {
    public:
      add_to_vlan(uint8_t *mbuf, uint32_t size);
      add_to_vlan(switch_vlan vlan, switch_port& p);
      virtual ~add_to_vlan();

      virtual void parse();
      virtual void write();

    protected:
      switch_vlan vlanid;
      switch_port port;
  }; // class add_to_vlan

  static const NCCP_OperationType NCCP_Operation_DeleteFromVlan = 65;
  class delete_from_vlan : public request
  {
    public:
      delete_from_vlan(uint8_t *mbuf, uint32_t size);
      delete_from_vlan(switch_vlan vlan, switch_port& p);
      virtual ~delete_from_vlan();

      virtual void parse();
      virtual void write();

    protected:
      switch_vlan vlanid;
      switch_port port;
  }; // class delete_from_vlan

  static const NCCP_OperationType NCCP_Operation_Initialize = 66;
  class initialize : public request
  {
    public:
      initialize(uint8_t *mbuf, uint32_t size);
      initialize();
      virtual ~initialize();
      void add_switch(switch_info newSwitch);

      virtual void parse();
      virtual void write();

    protected:
      std::list<switch_info> switches;
  }; // class delete_from_vlan
};

#endif // _SWITCH_REQUESTS_H
