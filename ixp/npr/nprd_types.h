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

#ifndef _NPRD_TYPES_H
#define _NPRD_TYPES_H

namespace npr 
{
  typedef union __attribute__ ((__packed__)) _plugin_control_header
  {
    struct __attribute__ ((__packed__))
    {
      unsigned int response_needed : 1;
      unsigned int type            : 7;
      unsigned int num_words       : 8;
      unsigned int mid             : 16;
    };
    unsigned int v;
    unsigned char c[4];
  } plugin_control_header;

  typedef union __attribute__ ((__packed__)) _plc_to_xscale_data
  {
    struct __attribute__ ((__packed__))
    {
      unsigned int res1            : 8;
      unsigned int buf_handle      : 24;
      unsigned int l3_pkt_length   : 16;
      unsigned int qid             : 16;
      unsigned int ptag            : 5;
      unsigned int in_port         : 3;
      unsigned int res2            : 1;
      unsigned int arp_source      : 1;
      unsigned int nh_invalid      : 1;
      unsigned int arp_needed      : 1;
      unsigned int non_ip          : 1;
      unsigned int no_route        : 1;
      unsigned int ip_options      : 1;
      unsigned int ttl_expired     : 1;
      unsigned int stats_index     : 16;
      unsigned int dest_value      : 32; // triples as DIP, NH MAC hi32, SRAM result addr (lower 21 bits)
      unsigned int nh_mac_low16    : 16;
      unsigned int ethertype       : 16;
      unsigned int res5            : 20;
      unsigned int uc_mc_bits      : 12; // for unicast: res(4), drop(1), pps(1), out port(3), out plugin(3)
                                         // for mulitcast: pps(1), copy vector(11)
    };
    unsigned int v[6];
  } plc_to_xscale_data;
  #define PLC_TO_XSCALE_DATA_NUM_WORDS 6

  typedef plc_to_xscale_data xscale_to_plugin_data;
  #define XSCALE_TO_PLUGIN_DATA_NUM_WORDS PLC_TO_XSCALE_DATA_NUM_WORDS

  typedef union __attribute__ ((__packed__)) _xscale_to_mux_data
  {
    struct __attribute__ ((__packed__))
    {
      unsigned int res1          : 4;
      unsigned int out_port      : 4;
      unsigned int buf_handle    : 24;
      unsigned int l3_pkt_length : 16;
      unsigned int qid           : 16;
      unsigned int ptag          : 5;
      unsigned int in_port       : 3;
      unsigned int res2          : 7;
      unsigned int pass_through  : 1;
      unsigned int stats_index   : 16;
    };
    unsigned int v[3];
  } xscale_to_mux_data;
  #define XSCALE_TO_MUX_DATA_NUM_WORDS 3

  typedef union __attribute__ ((__packed__)) _xscale_to_qm_data
  {
    struct __attribute__ ((__packed__))
    {
      unsigned int res1          : 8;
      unsigned int buf_handle    : 24;
      unsigned int res2          : 4;
      unsigned int out_port      : 4;
      unsigned int res3          : 8;
      unsigned int qid           : 16;
      unsigned int l3_pkt_length : 16;
      unsigned int res4          : 16;
    };
    unsigned int v[3];
  } xscale_to_qm_data;
  #define XSCALE_TO_QM_DATA_NUM_WORDS 3

  typedef union __attribute__ ((__packed__)) _xscale_to_flm_data
  {
    struct __attribute__ ((__packed__)) 
    {
      unsigned int res1          : 8;
      unsigned int buf_handle    : 24;
    };
    unsigned int v[1];
  } xscale_to_flm_data;
  #define XSCALE_TO_FLM_DATA_NUM_WORDS 1

  #define WILDCARD_VALUE 0xffffffff

  typedef union __attribute__ ((__packed__)) _route_key
  {
    struct __attribute__ ((__packed__))
    {
      unsigned int daddr : 32;
      unsigned int saddr : 32;
      unsigned int ptag  : 5;
      unsigned int port  : 3;
    };
    unsigned int v[3];
    unsigned char c[9];
  } route_key;

  typedef union __attribute__ ((__packed__)) _route_result
  {
    struct __attribute__ ((__packed__))
    {
      unsigned int entry_valid  : 1;
      unsigned int nh_ip_valid  : 1;
      unsigned int nh_mac_valid : 1;
      unsigned int ip_mc_valid  : 1;
      unsigned int uc_mc_bits   : 12;
      unsigned int qid          : 16;
      unsigned int stats_index  : 16;
      unsigned int nh_hi16      : 16;
      unsigned int nh_low32     : 32;
    };
    unsigned int v[3];
    unsigned char c[12];
  } route_result;

  typedef struct _route_entry
  {
    route_key key;
    route_key mask;
    route_result result;
  } route_entry;

  typedef union __attribute__ ((__packed__)) _pfilter_key
  {
    struct __attribute__ ((__packed__))
    {
      unsigned int daddr      : 32;
      unsigned int saddr      : 32;
      unsigned int ptag       : 5;
      unsigned int port       : 3;
      unsigned int proto      : 8;
      unsigned int dport      : 16;
      unsigned int sport      : 16;
      unsigned int exceptions : 16;
      unsigned int tcp_flags  : 12;
      unsigned int res        : 4;
    };
    unsigned int v[5];
    unsigned char c[18];
  } pfilter_key;

  typedef union __attribute__ ((__packed__)) _pfilter_result
  {
    struct  __attribute__ ((__packed__))
    {
      unsigned int entry_valid  : 1;
      unsigned int nh_ip_valid  : 1;
      unsigned int nh_mac_valid : 1;
      unsigned int ip_mc_valid  : 1;
      unsigned int uc_mc_bits   : 12;
      unsigned int qid          : 16;
      unsigned int stats_index  : 16;
      unsigned int nh_hi16      : 16;
      unsigned int nh_low32     : 32;
    };
    unsigned int v[3];
    unsigned char c[12];
  } pfilter_result;

  typedef struct _pfilter_entry
  {
    pfilter_key key;
    pfilter_key mask;
    pfilter_result result;
    unsigned int priority : 8;
  } pfilter_entry;

  typedef union __attribute__ ((__packed__)) _afilter_key
  {
    struct __attribute__ ((__packed__))
    {
      unsigned int daddr      : 32;
      unsigned int saddr      : 32;
      unsigned int ptag       : 5;
      unsigned int port       : 3;
      unsigned int proto      : 8;
      unsigned int dport      : 16;
      unsigned int sport      : 16;
      unsigned int exceptions : 16;
      unsigned int tcp_flags  : 12;
      unsigned int res        : 4;
    };
    unsigned int v[5];
    unsigned char c[18];
  } afilter_key;

  typedef union __attribute__ ((__packed__)) _afilter_result
  {
    struct __attribute__ ((__packed__))
    {
      unsigned int entry_valid  : 1;
      unsigned int nh_ip_valid  : 1;
      unsigned int nh_mac_valid : 1;
      unsigned int ip_mc_valid  : 1;
      unsigned int sampling     : 2;
      unsigned int res          : 2;
      unsigned int drop         : 1;
      unsigned int pps          : 1;
      unsigned int out_port     : 3;
      unsigned int out_plugin   : 3;
      unsigned int qid          : 16;
      unsigned int stats_index  : 16;
      unsigned int nh_hi16      : 16;
      unsigned int nh_low32     : 32;
    };
    unsigned int v[3];
    unsigned char c[12];
  } afilter_result;

  typedef struct _afilter_entry
  {
    afilter_key key;
    afilter_key mask;
    afilter_result result;
    unsigned int priority : 8;
  } afilter_entry;

  typedef union __attribute__ ((__packed__)) _arp_key
  {
    struct __attribute__ ((__packed__))
    {
      unsigned int daddr      : 32;
      unsigned int res1       : 32;
      unsigned int res2       : 8;
    };
    unsigned int v[3];
    unsigned char c[9];
  } arp_key;

  typedef union __attribute__ ((__packed__)) _arp_result
  {
    struct __attribute__ ((__packed__))
    {
      unsigned int entry_valid : 1;
      unsigned int res         : 15;
      unsigned int nh_hi16      : 16;
      unsigned int nh_low32     : 32;
    };
    unsigned int v[2];
    unsigned char c[8];
  } arp_result;

  typedef struct _arp_entry
  {
    arp_key key;
    arp_key mask;
    arp_result result;
  } arp_entry;

  typedef union __attribute__ ((__packed__)) _aux_sample_rates
  {
    struct __attribute__ ((__packed__))
    {
      unsigned int rate00;
      unsigned int rate01;
      unsigned int rate10;
      unsigned int rate11;
    };
    unsigned int i[4];
  } aux_sample_rates;

  typedef union __attribute__ ((__packed__)) _port_rates
  {
    struct __attribute__ ((__packed__))
    {
      unsigned int port0_rate;
      unsigned int port1_rate;
      unsigned int port2_rate;
      unsigned int port3_rate;
      unsigned int port4_rate;
    };
    unsigned int i[5];
  } port_rates;

  typedef union __attribute__ ((__packed__)) _mux_quanta
  {
    struct __attribute__ ((__packed__))
    {
      unsigned int rx_quantum;
      unsigned int plugin_quantum;
      unsigned int xscaleqm_quantum;
      unsigned int xscalepl_quantum;
    };
    unsigned int i[4];
  } mux_quanta;

  typedef union __attribute__ ((__packed__)) _exception_dests
  {
    struct __attribute__ ((__packed__))
    {
      unsigned int no_route   : 3;
      unsigned int arp_needed : 3;
      unsigned int nh_invalid : 3;
    };
    unsigned int i;
  } exception_dests;

  typedef union __attribute__ ((__packed__)) _buffer_desc
  {
    struct __attribute__ ((__packed__))
    {
      unsigned int buffer_next  : 32;
      unsigned int buffer_size  : 16;
      unsigned int offset       : 16;
      unsigned int packet_size  : 16;
      unsigned int freelist_id  : 4;
      unsigned int res1         : 4;
      unsigned int ref_count    : 8;
      unsigned int stats_index  : 16;
      unsigned int nh_mac_hi16  : 16;
      unsigned int nh_mac_low32 : 32;
      unsigned int ethertype    : 16;
      unsigned int res2         : 16;
      unsigned int res3         : 32;
      unsigned int packet_next  : 32;
    };
    unsigned int v[8];
  } buffer_desc;
};

#endif // _NPRD_TYPES_H
