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

#ifndef _SWRD_REQUESTS_H
#define _SWRD_REQUESTS_H

namespace swr
{
  static const NCCP_OperationType SWR_ConfigureNode = 70;
  class configure_node_req : public configure_node
  {
    public:
      configure_node_req(uint8_t *mbuf, uint32_t size);
      virtual ~configure_node_req();
 
      virtual bool handle();
  }; // class configure_node_req

/* just for reference, here is the rli_request class
 * class rli_request : public request
 * {
 *   public:
 *     rli_request(uint8_t *mbuf, uint32_t size);
 *     virtual ~rli_request();
 *
 *     virtual void parse();
 *
 *   protected:
 *     uint32_t id;
 *     uint16_t port;
 *     nccp_string version;
 *     std::vector<param> params;
 * }; //class rli_request
 */

  static const NCCP_OperationType SWR_AddRouteMain = 73;
  class add_route_main_req : public rli_request
  {
    public:
      add_route_main_req(uint8_t *mbuf, uint32_t size);
      virtual ~add_route_main_req();
 
      virtual void parse();
      virtual bool handle();

    protected:
      uint32_t prefix;
      uint32_t mask;
      uint32_t output_port;
      uint32_t nexthop_ip;
  }; // class add_route_main_req

  static const NCCP_OperationType SWR_AddRoutePort = 74;
  class add_route_port_req : public rli_request
  {
    public:
      add_route_port_req(uint8_t *mbuf, uint32_t size);
      virtual ~add_route_port_req();
 
      virtual void parse();
      virtual bool handle();

    protected:
      // port is in the rli_request base class.
      uint32_t prefix;
      uint32_t mask;
      uint32_t output_port;
      uint32_t nexthop_ip;
  }; // class add_route_port_req

  static const NCCP_OperationType SWR_DeleteRouteMain = 75;
  class del_route_main_req : public rli_request
  {
    public:
      del_route_main_req(uint8_t *mbuf, uint32_t size);
      virtual ~del_route_main_req();
 
      virtual void parse();
      virtual bool handle();

    protected:
      uint32_t prefix;
      uint32_t mask;
      uint32_t output_port;
      uint32_t nexthop_ip;
  }; // class del_route_main_req

  static const NCCP_OperationType SWR_DeleteRoutePort = 76;
  class del_route_port_req : public rli_request
  {
    public:
      del_route_port_req(uint8_t *mbuf, uint32_t size);
      virtual ~del_route_port_req();
 
      virtual void parse();
      virtual bool handle();

    protected:
      // port is in the rli_request base class.
      uint32_t prefix;
      uint32_t mask;
      uint32_t output_port;
      uint32_t nexthop_ip;
  }; // class del_route_port_req


/*
  static const NCCP_OperationType SWR_AddFilter = 76;
  class add_filter_req : public rli_request
  {
    public:
      add_filter_req(uint8_t *mbuf, uint32_t size);
      virtual ~add_filter_req();
 
      virtual void parse();
      virtual bool handle();

    protected:
  //echo "Usage: $0 <portnum> <iface> <vlanNum> <filterId> <dstAddr> <dstMask> <srcAddr> <srcMask> <proto> <dport> <sport> <tcpSyn> <tcpAck> <tcpFin> <tcpRst> <tcpUrg> <tcpPsh> <qid> <drop> <outputPortNum> <outputDev> <outputVlan> <gw>"
      bool aux;
      uint32_t dest_prefix;
      uint32_t dest_mask;
      uint32_t src_prefix;
      uint32_t src_mask;
      uint32_t plugin_tag;
      std::string protocol;
      uint32_t dest_port;
      uint32_t src_port;
      uint32_t tcp_fin;
      uint32_t tcp_syn;
      uint32_t tcp_rst;
      uint32_t tcp_psh;
      uint32_t tcp_ack;
      uint32_t tcp_urg;
      uint32_t qid;
      bool multicast;
      bool unicast_drop;
      std::string output_port;
      uint32_t priority;
  }; // class add_filter_req

  static const NCCP_OperationType SWR_DeleteFilter = 77;
  class delete_filter_req : public rli_request
  {
    public:
      delete_filter_req(uint8_t *mbuf, uint32_t size);
      virtual ~delete_filter_req();
 
      virtual void parse();
      virtual bool handle();

    protected:
      bool aux;
      uint32_t dest_prefix;
      uint32_t src_prefix;
      uint32_t plugin_tag;
      std::string protocol;
      uint32_t dest_port;
      uint32_t src_port;
      uint32_t exception_nonip;
      uint32_t exception_arp;
      uint32_t exception_ipopt;
      uint32_t exception_ttl;
      uint32_t tcp_fin;
      uint32_t tcp_syn;
      uint32_t tcp_rst;
      uint32_t tcp_psh;
      uint32_t tcp_ack;
      uint32_t tcp_urg;
  }; // class delete_filter_req
*/

/*
  static const NCCP_OperationType SWR_SetQueueParams = 78;
  class set_queue_params_req : public rli_request
  {
    public:
     set_queue_params_req(uint8_t *mbuf, uint32_t size);
     virtual ~set_queue_params_req();
 
      virtual void parse();
      virtual bool handle();

    protected:
      uint32_t qid;
      uint32_t threshold;
      uint32_t quantum;
  }; // class set_queue_params_req
*/

  static const NCCP_OperationType SWR_SetPortRate = 81;
  class set_port_rate_req : public rli_request
  {
    public:
      set_port_rate_req(uint8_t *mbuf, uint32_t size);
      virtual ~set_port_rate_req();
 
      virtual void parse();
      virtual bool handle();

    protected:
      uint32_t rate;
  }; // class set_port_rate_req
  


  //MONITORING REQUESTS
  /*
  static const NCCP_OperationType SWR_GetRXPkt = 107;
  class get_rx_pkt_req : public rli_request
  {
    public:
      get_rx_pkt_req(uint8_t *mbuf, uint32_t size);
      virtual ~get_rx_pkt_req();
 
      virtual void parse();
      virtual bool handle();

    protected:
  }; // class get_rx_pkt_req

  static const NCCP_OperationType SWR_GetRXByte = 108;
  class get_rx_byte_req : public rli_request
  {
    public:
      get_rx_byte_req(uint8_t *mbuf, uint32_t size);
      virtual ~get_rx_byte_req();
 
      virtual void parse();
      virtual bool handle();

    protected:
  }; // class get_rx_byte_req
  */
  static const NCCP_OperationType SWR_GetTXPkt = 109;
  class get_tx_pkt_req : public rli_request
  {
    public:
      get_tx_pkt_req(uint8_t *mbuf, uint32_t size);
      virtual ~get_tx_pkt_req();
 
      virtual void parse();
      virtual bool handle();

    protected:
  }; // class get_tx_pkt_req

  static const NCCP_OperationType SWR_GetTXKBits = 110;
  class get_tx_kbits_req : public rli_request
  {
    public:
      get_tx_kbits_req(uint8_t *mbuf, uint32_t size);
      virtual ~get_tx_kbits_req();
 
      virtual void parse();
      virtual bool handle();

    protected:
  }; // class get_tx_kbits_req

  static const NCCP_OperationType SWR_GetQueueLength = 68;
  class get_queue_len_req : public rli_request
  {
    public:
      get_queue_len_req(uint8_t *mbuf, uint32_t size);
      virtual ~get_queue_len_req();
 
      virtual void parse();
      virtual bool handle();
  }; // class get_queue_len_req

  static const NCCP_OperationType SWR_GetBacklog = 111;
  class get_backlog_req : public rli_request
  {
    public:
      get_backlog_req(uint8_t *mbuf, uint32_t size);
      virtual ~get_backlog_req();
 
      virtual void parse();
      virtual bool handle();

  }; // class get_backlog_req

  static const NCCP_OperationType SWR_GetDrops = 112;
  class get_drops_req : public rli_request
  {
    public:
      get_drops_req(uint8_t *mbuf, uint32_t size);
      virtual ~get_drops_req();
 
      virtual void parse();
      virtual bool handle();
  }; // class get_drops_req

  static const NCCP_OperationType SWR_GetLinkRXPkt = 113;
  class get_link_rx_pkt_req : public rli_request
  {
    public:
      get_link_rx_pkt_req(uint8_t *mbuf, uint32_t size);
      virtual ~get_link_rx_pkt_req();
 
      virtual void parse();
      virtual bool handle();

    protected:
  }; // class get_link_rx_pkt_req

  static const NCCP_OperationType SWR_GetLinkTXPkt = 114;
  class get_link_tx_pkt_req : public rli_request
  {
    public:
      get_link_tx_pkt_req(uint8_t *mbuf, uint32_t size);
      virtual ~get_link_tx_pkt_req();
 
      virtual void parse();
      virtual bool handle();

    protected:
  }; // class get_link_tx_pkt_req

  static const NCCP_OperationType SWR_GetLinkRXKBits = 115;
  class get_link_rx_kbits_req : public rli_request
  {
    public:
      get_link_rx_kbits_req(uint8_t *mbuf, uint32_t size);
      virtual ~get_link_rx_kbits_req();
 
      virtual void parse();
      virtual bool handle();

    protected:
  }; // class get_link_rx_kbits_req

  static const NCCP_OperationType SWR_GetLinkTXKBits = 116;
  class get_link_tx_kbits_req : public rli_request
  {
    public:
      get_link_tx_kbits_req(uint8_t *mbuf, uint32_t size);
      virtual ~get_link_tx_kbits_req();
 
      virtual void parse();
      virtual bool handle();

    protected:
  }; // class get_link_tx_kbits_req

  static const NCCP_OperationType SWR_GetLinkRXErrors = 117;
  class get_link_rx_errors_req : public rli_request
  {
    public:
      get_link_rx_errors_req(uint8_t *mbuf, uint32_t size);
      virtual ~get_link_rx_errors_req();
 
      virtual void parse();
      virtual bool handle();

    protected:
  }; // class get_link_rx_errors_req

  static const NCCP_OperationType SWR_GetLinkTXErrors = 118;
  class get_link_tx_errors_req : public rli_request
  {
    public:
      get_link_tx_errors_req(uint8_t *mbuf, uint32_t size);
      virtual ~get_link_tx_errors_req();
 
      virtual void parse();
      virtual bool handle();

    protected:
  }; // class get_link_tx_errors_req

  static const NCCP_OperationType SWR_GetLinkRXDrops = 119;
  class get_link_rx_drops_req : public rli_request
  {
    public:
      get_link_rx_drops_req(uint8_t *mbuf, uint32_t size);
      virtual ~get_link_rx_drops_req();
 
      virtual void parse();
      virtual bool handle();

    protected:
  }; // class get_link_rx_drops_req

  static const NCCP_OperationType SWR_GetLinkTXDrops = 120;
  class get_link_tx_drops_req : public rli_request
  {
    public:
      get_link_tx_drops_req(uint8_t *mbuf, uint32_t size);
      virtual ~get_link_tx_drops_req();
 
      virtual void parse();
      virtual bool handle();

    protected:
  }; // class get_link_tx_drops_req
};  //namespace swr

#endif // _SWRD_REQUESTS_H
