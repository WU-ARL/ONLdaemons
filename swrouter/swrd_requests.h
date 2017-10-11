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


  static const NCCP_OperationType SWR_AddFilter = 77;
  static const NCCP_OperationType SWR_DeleteFilter = 78;
  static const NCCP_OperationType SWR_GetFilterBytes = 130;
  static const NCCP_OperationType SWR_GetFilterPkts = 131;
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

  static const NCCP_OperationType SWR_AddQueue = 87;
  static const NCCP_OperationType SWR_ChangeQueue = 88;
  static const NCCP_OperationType SWR_DeleteQueue = 89;
  static const NCCP_OperationType SWR_AddNetemParams = 90;
  static const NCCP_OperationType SWR_DeleteNetemParams = 91;

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
  

  static const NCCP_OperationType SWR_AddDelay = 82;
  static const NCCP_OperationType SWR_DeleteDelay = 83;
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
  
  /*
  static const NCCP_OperationType SWR_AddLoss = 84;
  static const NCCP_OperationType SWR_DeleteLoss = 85;
  class set_loss_req : public rli_request
  {
    public:
      set_loss_req(uint8_t *mbuf, uint32_t size);
      virtual ~set_loss_req();
 
      virtual void parse();
      virtual bool handle();

    protected:
      uint32_t percentage;
  }; // class set_loss_req
  

  static const NCCP_OperationType SWR_SetCorrupt = 84;
  class set_corrupt_req : public rli_request
  {
    public:
      set_corrupt_req(uint8_t *mbuf, uint32_t size);
      virtual ~set_corrupt_req();
 
      virtual void parse();
      virtual bool handle();

    protected:
      uint32_t percentage;
  }; // class set_corrupt_req
  

  static const NCCP_OperationType SWR_SetDuplicate = 85;
  class set_duplicate_req : public rli_request
  {
    public:
      set_duplicate_req(uint8_t *mbuf, uint32_t size);
      virtual ~set_duplicate_req();
 
      virtual void parse();
      virtual bool handle();

    protected:
      uint32_t percentage;
  }; // class set_duplicate_req
  

  static const NCCP_OperationType SWR_SetReorder = 86;
  class set_reorder_req : public rli_request
  {
    public:
      set_reorder_req(uint8_t *mbuf, uint32_t size);
      virtual ~set_reorder_req();
 
      virtual void parse();
      virtual bool handle();

    protected:
      uint32_t percentage;
  }; // class set_reorder_req
  */
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
  static const NCCP_OperationType SWR_GetQueueTXPkt = 121;
  static const NCCP_OperationType SWR_GetClassTXPkt = 125;
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

  static const NCCP_OperationType SWR_GetTXKBits = 110;
  static const NCCP_OperationType SWR_GetQueueTXKBits = 122;
  static const NCCP_OperationType SWR_GetClassTXKBits = 126;
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

  static const NCCP_OperationType SWR_GetDefQueueLength = 68;
  static const NCCP_OperationType SWR_GetQueueLength = 92;
  static const NCCP_OperationType SWR_GetClassLength = 129;
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

  static const NCCP_OperationType SWR_GetBacklog = 111;
  static const NCCP_OperationType SWR_GetQueueBacklog = 123;
  static const NCCP_OperationType SWR_GetClassBacklog = 127;
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

  static const NCCP_OperationType SWR_GetDrops = 112;
  static const NCCP_OperationType SWR_GetQueueDrops = 124;
  static const NCCP_OperationType SWR_GetClassDrops = 128;
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
