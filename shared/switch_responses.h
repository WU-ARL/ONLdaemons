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

#ifndef _SWITCH_RESPONSES_H
#define _SWITCH_RESPONSES_H

namespace onld
{
  class switch_response: public response
  {
    public:
      switch_response(uint8_t *mbuf, uint32_t size);
      switch_response(request *req, NCCP_StatusType stat);
      virtual ~switch_response();

      virtual void parse();
      virtual void write();
    
      NCCP_StatusType getStatus() { return status; }

    protected:
      NCCP_StatusType status;
  }; //class switch_response

  class add_vlan_response: public switch_response
  {
    public:
      add_vlan_response(uint8_t *mbuf, uint32_t size);
      add_vlan_response(request *req, NCCP_StatusType stat, switch_vlan vlan);
      virtual ~add_vlan_response();

      virtual void parse();
      virtual void write();
    
      switch_vlan getVlan() { return vlanid; }

    protected:
      switch_vlan vlanid;
  }; //class add_vlan_response
};

#endif // _SWITCH_RESPONSES_H
