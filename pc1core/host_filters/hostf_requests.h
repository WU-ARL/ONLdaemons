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

#ifndef _HOSTF_REQUESTS_H
#define _HOSTF_REQUESTS_H

namespace hostf
{
  static const NCCP_OperationType HOST_ConfigureNode = 70;
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

  static const NCCP_OperationType HOST_AddFilter = 77;
  static const NCCP_OperationType HOST_DeleteFilter = 78;
  static const NCCP_OperationType HOST_GetFilterBytes = 130;
  static const NCCP_OperationType HOST_GetFilterPkts = 131;
  class filter_req : public rli_request
  {
    public:
      filter_req(uint8_t *mbuf, uint32_t size);
      virtual ~filter_req();
 
      virtual void parse();
      virtual bool handle();

    protected:
  //echo "Usage: $0 <portnum> <iface> <vlanNum> <filterId> <dstAddr> <dstMask> <srcAddr> <srcMask> <proto> <dport> <sport> <tcpSyn> <tcpAck> <tcpFin> <tcpRst> <tcpUrg> <tcpPsh> <drop> <outputPortNum> <outputDev>"
      std::string dest_prefix;
      uint32_t dest_mask;
      std::string src_prefix;
      uint32_t src_mask;
      std::string protocol;
      uint32_t dest_port;
      uint32_t src_port;
      uint32_t tcp_fin;
      uint32_t tcp_syn;
      uint32_t tcp_rst;
      uint32_t tcp_psh;
      uint32_t tcp_ack;
      uint32_t tcp_urg;
      bool unicast_drop;
      std::string output_port;
      uint32_t sampling;
      uint32_t qid;
      uint32_t mark;
  }; // class filter_req

  static const NCCP_OperationType HOST_AddQueue = 87;
  static const NCCP_OperationType HOST_ChangeQueue = 88;
  static const NCCP_OperationType HOST_DeleteQueue = 89;
  static const NCCP_OperationType HOST_AddNetemParams = 90;
  static const NCCP_OperationType HOST_DeleteNetemParams = 91;

  class set_queue_params_req : public rli_request
  {
    public:
     set_queue_params_req(uint8_t *mbuf, uint32_t size);
     virtual ~set_queue_params_req();
 
      virtual void parse();
      virtual bool handle();

    protected:
      uint32_t qid;
      uint32_t rate;
      uint32_t burst;
      uint32_t ceil_rate;
      uint32_t cburst;
      uint32_t mtu;
      //netem params
      //delay
      uint32_t delay;
      uint32_t jitter;
      //loss
      uint32_t loss_percent; //uses random
      //corrupt
      uint32_t corrupt_percent;
      //duplicate
      uint32_t duplicate_percent;
  }; // class set_queue_params_req

  static const NCCP_OperationType HOST_SetPortRate = 81;
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
  

  static const NCCP_OperationType HOST_AddDelay = 82;
  static const NCCP_OperationType HOST_DeleteDelay = 83;
  class add_delay_req : public rli_request
  {
    public:
      add_delay_req(uint8_t *mbuf, uint32_t size);
      virtual ~add_delay_req();
 
      virtual void parse();
      virtual bool handle();

    protected:
      uint32_t dtime;
      uint32_t jitter;
  }; // class add_delay_req


  class delete_delay_req : public rli_request
  {
    public:
      delete_delay_req(uint8_t *mbuf, uint32_t size);
      virtual ~delete_delay_req();
 
      virtual bool handle();
  }; // class delete_delay_req
  
  static const NCCP_OperationType HOST_GetTXPkt = 109;
  static const NCCP_OperationType HOST_GetQueueTXPkt = 121;
  static const NCCP_OperationType HOST_GetClassTXPkt = 125;
  class get_tx_pkt_req : public rli_request
  {
    public:
      get_tx_pkt_req(uint8_t *mbuf, uint32_t size);
      virtual ~get_tx_pkt_req();
 
      virtual void parse();
      virtual bool handle();

    protected:
    protected:
      uint32_t qid;
  }; // class get_tx_pkt_req

  static const NCCP_OperationType HOST_GetTXKBits = 110;
  static const NCCP_OperationType HOST_GetQueueTXKBits = 122;
  static const NCCP_OperationType HOST_GetClassTXKBits = 126;
  class get_tx_kbits_req : public rli_request
  {
    public:
      get_tx_kbits_req(uint8_t *mbuf, uint32_t size);
      virtual ~get_tx_kbits_req();
 
      virtual void parse();
      virtual bool handle();

    protected:
    protected:
      uint32_t qid;
  }; // class get_tx_kbits_req

  static const NCCP_OperationType HOST_GetDefQueueLength = 68;
  static const NCCP_OperationType HOST_GetQueueLength = 92;
  static const NCCP_OperationType HOST_GetClassLength = 129;
  class get_queue_len_req : public rli_request
  {
    public:
      get_queue_len_req(uint8_t *mbuf, uint32_t size);
      virtual ~get_queue_len_req();
 
      virtual void parse();
      virtual bool handle();

    protected:
      uint32_t qid;
  }; // class get_queue_len_req

  static const NCCP_OperationType HOST_GetBacklog = 111;
  static const NCCP_OperationType HOST_GetQueueBacklog = 123;
  static const NCCP_OperationType HOST_GetClassBacklog = 127;
  class get_backlog_req : public rli_request
  {
    public:
      get_backlog_req(uint8_t *mbuf, uint32_t size);
      virtual ~get_backlog_req();
 
      virtual void parse();
      virtual bool handle();

    protected:
      uint32_t qid;
  }; // class get_backlog_req

  static const NCCP_OperationType HOST_GetDrops = 112;
  static const NCCP_OperationType HOST_GetQueueDrops = 124;
  static const NCCP_OperationType HOST_GetClassDrops = 128;
  class get_drops_req : public rli_request
  {
    public:
      get_drops_req(uint8_t *mbuf, uint32_t size);
      virtual ~get_drops_req();
 
      virtual void parse();
      virtual bool handle();

    protected:
      uint32_t qid;
  }; // class get_drops_req

  static const NCCP_OperationType HOST_GetLinkRXPkt = 113;
  class get_link_rx_pkt_req : public rli_request
  {
    public:
      get_link_rx_pkt_req(uint8_t *mbuf, uint32_t size);
      virtual ~get_link_rx_pkt_req();
 
      virtual void parse();
      virtual bool handle();

    protected:
  }; // class get_link_rx_pkt_req

  static const NCCP_OperationType HOST_GetLinkTXPkt = 114;
  class get_link_tx_pkt_req : public rli_request
  {
    public:
      get_link_tx_pkt_req(uint8_t *mbuf, uint32_t size);
      virtual ~get_link_tx_pkt_req();
 
      virtual void parse();
      virtual bool handle();

    protected:
  }; // class get_link_tx_pkt_req

  static const NCCP_OperationType HOST_GetLinkRXKBits = 115;
  class get_link_rx_kbits_req : public rli_request
  {
    public:
      get_link_rx_kbits_req(uint8_t *mbuf, uint32_t size);
      virtual ~get_link_rx_kbits_req();
 
      virtual void parse();
      virtual bool handle();

    protected:
  }; // class get_link_rx_kbits_req

  static const NCCP_OperationType HOST_GetLinkTXKBits = 116;
  class get_link_tx_kbits_req : public rli_request
  {
    public:
      get_link_tx_kbits_req(uint8_t *mbuf, uint32_t size);
      virtual ~get_link_tx_kbits_req();
 
      virtual void parse();
      virtual bool handle();

    protected:
  }; // class get_link_tx_kbits_req

  static const NCCP_OperationType HOST_GetLinkRXErrors = 117;
  class get_link_rx_errors_req : public rli_request
  {
    public:
      get_link_rx_errors_req(uint8_t *mbuf, uint32_t size);
      virtual ~get_link_rx_errors_req();
 
      virtual void parse();
      virtual bool handle();

    protected:
  }; // class get_link_rx_errors_req

  static const NCCP_OperationType HOST_GetLinkTXErrors = 118;
  class get_link_tx_errors_req : public rli_request
  {
    public:
      get_link_tx_errors_req(uint8_t *mbuf, uint32_t size);
      virtual ~get_link_tx_errors_req();
 
      virtual void parse();
      virtual bool handle();

    protected:
  }; // class get_link_tx_errors_req

  static const NCCP_OperationType HOST_GetLinkRXDrops = 119;
  class get_link_rx_drops_req : public rli_request
  {
    public:
      get_link_rx_drops_req(uint8_t *mbuf, uint32_t size);
      virtual ~get_link_rx_drops_req();
 
      virtual void parse();
      virtual bool handle();

    protected:
  }; // class get_link_rx_drops_req

  static const NCCP_OperationType HOST_GetLinkTXDrops = 120;
  class get_link_tx_drops_req : public rli_request
  {
    public:
      get_link_tx_drops_req(uint8_t *mbuf, uint32_t size);
      virtual ~get_link_tx_drops_req();
 
      virtual void parse();
      virtual bool handle();

    protected:
  }; // class get_link_tx_drops_req
};  //namespace hostf

#endif // _HOSTF_REQUESTS_H
