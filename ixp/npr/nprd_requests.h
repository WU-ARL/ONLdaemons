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

#ifndef _NPRD_REQUESTS_H
#define _NPRD_REQUESTS_H

namespace npr
{
  class configure_node_req : public configure_node
  {
    public:
      configure_node_req(uint8_t *mbuf, uint32_t size);
      virtual ~configure_node_req();
 
      virtual bool handle();
  }; // class configure_node_req

  static const NCCP_OperationType NPR_AddRoute = 73;
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

  static const NCCP_OperationType NPR_DeleteRoute = 75;
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

  static const NCCP_OperationType NPR_AddFilter = 76;
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

  static const NCCP_OperationType NPR_DeleteFilter = 77;
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

  static const NCCP_OperationType NPR_SetSampleRates = 98;
  class set_sample_rates_req : public rli_request
  {
    public:
      set_sample_rates_req(uint8_t *mbuf, uint32_t size);
      virtual ~set_sample_rates_req();
 
      virtual void parse();
      virtual bool handle();

    protected:
      double rate1;
      double rate2;
      double rate3;
  }; // class set_sample_rates_req

  static const NCCP_OperationType NPR_SetQueueParams = 78;
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

  static const NCCP_OperationType NPR_SetPortRate = 81;
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

  static const NCCP_OperationType NPR_GetPlugins = 83;
  //cgw, this class will definitely need to change
  class get_plugins_req : public request
  {
    public:
      get_plugins_req(uint8_t *mbuf, uint32_t size);
      virtual ~get_plugins_req();
 
      virtual void parse();
      virtual bool handle();

    protected:
      nccp_string root_dir;
  }; // class get_plugins_req

  static const NCCP_OperationType NPR_AddPlugin = 85;
  class add_plugin_req : public rli_request
  {
    public:
      add_plugin_req(uint8_t *mbuf, uint32_t size);
      virtual ~add_plugin_req();
 
      virtual void parse();
      virtual bool handle();

    protected:
      uint32_t microengine;
      std::string path;
  }; // class add_plugin_req

  static const NCCP_OperationType NPR_DeletePlugin = 86;
  class delete_plugin_req : public rli_request
  {
    public:
      delete_plugin_req(uint8_t *mbuf, uint32_t size);
      virtual ~delete_plugin_req();
 
      virtual void parse();
      virtual bool handle();

    protected:
      uint32_t microengine;
  }; // class delete_plugin_req

  static const NCCP_OperationType NPR_PluginDebug = 92;
  class plugin_debug_req : public rli_request
  {
    public:
      plugin_debug_req(uint8_t *mbuf, uint32_t size);
      virtual ~plugin_debug_req();
 
      virtual void parse();
      virtual bool handle();

    protected:
      std::string path;
  }; // class plugin_debug_req

  static const NCCP_OperationType NPR_PluginControlMsg = 89;
  class plugin_control_msg_req : public rli_request
  {
    public:
      plugin_control_msg_req(uint8_t *mbuf, uint32_t size);
      virtual ~plugin_control_msg_req();

      void send_plugin_response(std::string msg);

      virtual void parse();
      virtual bool handle();

    protected:
      uint32_t microengine;
      std::string msg;
  }; // class plugin_control_msg_req

  static const NCCP_OperationType NPR_GetRXPkt = 107;
  class get_rx_pkt_req : public rli_request
  {
    public:
      get_rx_pkt_req(uint8_t *mbuf, uint32_t size);
      virtual ~get_rx_pkt_req();
 
      virtual void parse();
      virtual bool handle();

    protected:
  }; // class get_rx_pkt_req

  static const NCCP_OperationType NPR_GetRXByte = 108;
  class get_rx_byte_req : public rli_request
  {
    public:
      get_rx_byte_req(uint8_t *mbuf, uint32_t size);
      virtual ~get_rx_byte_req();
 
      virtual void parse();
      virtual bool handle();

    protected:
  }; // class get_rx_byte_req

  static const NCCP_OperationType NPR_GetTXPkt = 109;
  class get_tx_pkt_req : public rli_request
  {
    public:
      get_tx_pkt_req(uint8_t *mbuf, uint32_t size);
      virtual ~get_tx_pkt_req();
 
      virtual void parse();
      virtual bool handle();

    protected:
  }; // class get_tx_pkt_req

  static const NCCP_OperationType NPR_GetTXByte = 110;
  class get_tx_byte_req : public rli_request
  {
    public:
      get_tx_byte_req(uint8_t *mbuf, uint32_t size);
      virtual ~get_tx_byte_req();
 
      virtual void parse();
      virtual bool handle();

    protected:
  }; // class get_tx_byte_req

  static const NCCP_OperationType NPR_GetRegPkt = 105;
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

  static const NCCP_OperationType NPR_GetRegByte = 106;
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

  static const NCCP_OperationType NPR_GetStatsPreQPkt = 101;
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

  static const NCCP_OperationType NPR_GetStatsPostQPkt = 102;
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

  static const NCCP_OperationType NPR_GetStatsPreQByte = 103;
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

  static const NCCP_OperationType NPR_GetStatsPostQByte = 104;
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

  static const NCCP_OperationType NPR_GetQueueLength = 68;
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

  static const NCCP_OperationType NPR_GetPluginCounter = 87;
  class get_plugin_counter_req : public rli_request
  {
    public:
      get_plugin_counter_req(uint8_t *mbuf, uint32_t size);
      virtual ~get_plugin_counter_req();
 
      virtual void parse();
      virtual bool handle();

    protected:
      uint32_t microengine;
      uint32_t counter;
  }; // class get_plugin_counter_req
};

#endif // _NPRD_REQUESTS_H
