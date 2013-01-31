/*
 * ONL Notice
 *
 * Copyright (c) 2009-2013 Pierluigi Rolando, Charlie Wiseman, Jyoti Parwatikar, John DeHart
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
 *   limitations under the License.
 */

#ifndef _HOST_REQUESTS_H
#define _HOST_REQUESTS_H

namespace host {

class req_logger : public rli_request {
  public:
	req_logger(uint8_t *mbuf, uint32_t size);
	virtual ~req_logger();

	virtual void parse();
	virtual bool handle();
};

// -------------- Node initialization

  static const NCCP_OperationType CCNX_SubtypeInit = 200;
  class get_subtype_init_req : public rli_request
  {
    public:
      get_subtype_init_req(uint8_t *mbuf, uint32_t size);
      virtual ~get_subtype_init_req();
 
      virtual void parse();
      virtual bool handle();

    protected:
  }; // class get_do_role_req



// -------------- CCNx role support

  static const NCCP_OperationType CCNX_DoRole = 201;
  class get_do_role_req : public rli_request
  {
    public:
      get_do_role_req(uint8_t *mbuf, uint32_t size);
      virtual ~get_do_role_req();
 
      virtual void parse();
      virtual bool handle();

    protected:
  }; // class get_do_role_req

  static const NCCP_OperationType CCNX_DoDaemon = 202;
  class get_do_daemon_req : public rli_request
  {
    public:
      get_do_daemon_req(uint8_t *mbuf, uint32_t size);
      virtual ~get_do_daemon_req();
 
      virtual void parse();
      virtual bool handle();

    protected:
  }; // class get_do_daemon_req

// --------------- CCN routing

  static const NCCP_OperationType CCNX_AddRoute = 173;
  class get_add_route_req : public rli_request
  {
    public:
      get_add_route_req(uint8_t *mbuf, uint32_t size);
      virtual ~get_add_route_req();
 
      virtual void parse();
      virtual bool handle();

    protected:
  }; // class get_add_route_req

  static const NCCP_OperationType CCNX_DelRoute = 175;
  class get_del_route_req : public rli_request
  {
    public:
      get_del_route_req(uint8_t *mbuf, uint32_t size);
      virtual ~get_del_route_req();
 
      virtual void parse();
      virtual bool handle();

    protected:
  }; // class get_del_route_req

  static const NCCP_OperationType CCNX_UpdateRoute = 174;
  class get_update_route_req : public rli_request
  {
    public:
      get_update_route_req(uint8_t *mbuf, uint32_t size);
      virtual ~get_update_route_req();
 
      virtual void parse();
      virtual bool handle();

    protected:
  }; // class get_update_route_req

//----------------------------  CCNd logging level
  static const NCCP_OperationType CCNX_SetLogLevel = 176;
  class set_log_level_req : public rli_request
  {
    public:
      set_log_level_req(uint8_t *mbuf, uint32_t size);
      virtual ~set_log_level_req();
 
      virtual void parse();
      virtual bool handle();

    protected:
      std::string level;
  }; // class set_log_level_req



// --------------- Monitoring

  static const NCCP_OperationType CCNX_GetRXPkt = 107;
  class get_rx_pkt_req : public rli_request
  {
    public:
      get_rx_pkt_req(uint8_t *mbuf, uint32_t size);
      virtual ~get_rx_pkt_req();
 
      virtual void parse();
      virtual bool handle();

    protected:
  }; // class get_rx_pkt_req

  static const NCCP_OperationType CCNX_GetRXByte = 108;
  class get_rx_byte_req : public rli_request
  {
    public:
      get_rx_byte_req(uint8_t *mbuf, uint32_t size);
      virtual ~get_rx_byte_req();
 
      virtual void parse();
      virtual bool handle();

    protected:
  }; // class get_rx_byte_req

  static const NCCP_OperationType CCNX_GetTXPkt = 109;
  class get_tx_pkt_req : public rli_request
  {
    public:
      get_tx_pkt_req(uint8_t *mbuf, uint32_t size);
      virtual ~get_tx_pkt_req();
 
      virtual void parse();
      virtual bool handle();

    protected:
  }; // class get_tx_pkt_req

  static const NCCP_OperationType CCNX_GetTXByte = 110;
  class get_tx_byte_req : public rli_request
  {
    public:
      get_tx_byte_req(uint8_t *mbuf, uint32_t size);
      virtual ~get_tx_byte_req();
 
      virtual void parse();
      virtual bool handle();

    protected:
  }; // class get_tx_byte_req

// ------------ memory and CPU

  static const NCCP_OperationType CCNX_GetMemUsage = 111;
  class get_mem_usage_req : public rli_request
  {
    public:
      get_mem_usage_req(uint8_t *mbuf, uint32_t size);
      virtual ~get_mem_usage_req();
 
      virtual void parse();
      virtual bool handle();

    protected:
  }; // class get_mem_usage_req

  static const NCCP_OperationType CCNX_GetCPUUsage = 112;
  class get_cpu_usage_req : public rli_request
  {
    public:
      get_cpu_usage_req(uint8_t *mbuf, uint32_t size);
      virtual ~get_cpu_usage_req();
 
      virtual void parse();
      virtual bool handle();

    protected:
  }; // class get_cpu_usage_req


// ------------ configuration

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
};

#endif // _HOST_REQUESTS_H
