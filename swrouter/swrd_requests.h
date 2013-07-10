#ifndef _SWRD_REQUESTS_H
#define _SWRD_REQUESTS_H

namespace swr
{
  class configure_node_req : public configure_node
  {
    public:
      configure_node_req(uint8_t *mbuf, uint32_t size);
      virtual ~configure_node_req();
 
      virtual bool handle();
  }; // class configure_node_req

  static const NCCP_OperationType SWR_AddRoute = 73;
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

  static const NCCP_OperationType SWR_DeleteRoute = 75;
  class delete_route_req : public rli_request
  {
    public:
      delete_route_req(uint8_t *mbuf, uint32_t size);
      virtual ~delete_route_req();
 
      virtual void parse();
      virtual bool handle();

    protected:
      uint32_t prefix;
  }; // class delete_route_req

  static const NCCP_OperationType SWR_AddFilter = 76;
  class add_filter_req : public rli_request
  {
    public:
      add_filter_req(uint8_t *mbuf, uint32_t size);
      virtual ~add_filter_req();
 
      virtual void parse();
      virtual bool handle();

    protected:
      bool aux;
      uint32_t dest_prefix;
      uint32_t dest_mask;
      uint32_t src_prefix;
      uint32_t src_mask;
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
      uint32_t qid;
      uint32_t stats_index;
      bool multicast;
      std::string port_plugin_selection;
      bool unicast_drop;
      std::string output_port;
      std::string output_plugin;
      uint32_t sampling_bits;
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

  static const NCCP_OperationType SWR_GetTXByte = 110;
  class get_tx_byte_req : public rli_request
  {
    public:
      get_tx_byte_req(uint8_t *mbuf, uint32_t size);
      virtual ~get_tx_byte_req();
 
      virtual void parse();
      virtual bool handle();

    protected:
  }; // class get_tx_byte_req

  static const NCCP_OperationType SWR_GetRegPkt = 105;
  class get_reg_pkt_req : public rli_request
  {
    public:
      get_reg_pkt_req(uint8_t *mbuf, uint32_t size);
      virtual ~get_reg_pkt_req();
 
      virtual void parse();
      virtual bool handle();

    protected:
      uint32_t stats_index;
  }; // class get_reg_pkt_req

  static const NCCP_OperationType SWR_GetRegByte = 106;
  class get_reg_byte_req : public rli_request
  {
    public:
      get_reg_byte_req(uint8_t *mbuf, uint32_t size);
      virtual ~get_reg_byte_req();
 
      virtual void parse();
      virtual bool handle();

    protected:
      uint32_t stats_index;
  }; // class get_reg_byte_req

  static const NCCP_OperationType SWR_GetStatsPreQPkt = 101;
  class get_stats_preq_pkt_req : public rli_request
  {
    public:
      get_stats_preq_pkt_req(uint8_t *mbuf, uint32_t size);
      virtual ~get_stats_preq_pkt_req();
 
      virtual void parse();
      virtual bool handle();

    protected:
      uint32_t stats_index;
  }; // class get_stats_preq_pkt_req

  static const NCCP_OperationType SWR_GetStatsPostQPkt = 102;
  class get_stats_postq_pkt_req : public rli_request
  {
    public:
      get_stats_postq_pkt_req(uint8_t *mbuf, uint32_t size);
      virtual ~get_stats_postq_pkt_req();
 
      virtual void parse();
      virtual bool handle();

    protected:
      uint32_t stats_index;
  }; // class get_stats_postq_pkt_req

  static const NCCP_OperationType SWR_GetStatsPreQByte = 103;
  class get_stats_preq_byte_req : public rli_request
  {
    public:
      get_stats_preq_byte_req(uint8_t *mbuf, uint32_t size);
      virtual ~get_stats_preq_byte_req();
 
      virtual void parse();
      virtual bool handle();

    protected:
      uint32_t stats_index;
  }; // class get_stats_preq_byte_req

  static const NCCP_OperationType SWR_GetStatsPostQByte = 104;
  class get_stats_postq_byte_req : public rli_request
  {
    public:
      get_stats_postq_byte_req(uint8_t *mbuf, uint32_t size);
      virtual ~get_stats_postq_byte_req();
 
      virtual void parse();
      virtual bool handle();

    protected:
      uint32_t stats_index;
  }; // class get_stats_postq_byte_req

  static const NCCP_OperationType SWR_GetQueueLength = 68;
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
  */

#endif // _SWRD_REQUESTS_H
