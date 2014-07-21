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
 */

#ifndef _HOST_REQUESTS_H
#define _HOST_REQUESTS_H

namespace host
{
  class configure_node_req : public configure_node
  {
    public:
      configure_node_req(uint8_t *mbuf, uint32_t size);
      virtual ~configure_node_req();
 
      virtual bool handle();
  }; // class configure_node_req

  static const NCCP_OperationType HOST_AddRoute = 73;
  class add_route_req : public rli_request
  {
    public:
      add_route_req(uint8_t *mbuf, uint32_t size);
      virtual ~add_route_req();

      virtual void parse();
      virtual bool handle();

    protected:
      uint32_t prefix;
      uint32_t mask;
      uint32_t output_port;
      uint32_t nexthop_ip;
      uint32_t stats_index;
  }; // class add_route_req

  static const NCCP_OperationType HOST_DeleteRoute = 75;
  class delete_route_req : public rli_request
  {
    public:
      delete_route_req(uint8_t *mbuf, uint32_t size);
      virtual ~delete_route_req();

      virtual void parse();
      virtual bool handle();

    protected:
      uint32_t prefix;
      uint32_t mask;
  }; // class delete_route_req

	//ard: start of vm code
  class start_vm_req : public start_vm
  {
    public:
      start_vm_req(uint8_t *mbuf, uint32_t size);
      virtual ~start_vm_req();
 
      virtual bool handle();
  }; // class start_vm_req

  class end_configure_node_req : public end_configure_node
  {
    public:
      end_configure_node_req(uint8_t *mbuf, uint32_t size);
      virtual ~end_configure_node_req();
 
      virtual bool handle();
  }; // class end_configure_node_req
};

#endif // _HOST_REQUESTS_H
