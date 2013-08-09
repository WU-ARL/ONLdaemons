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

#ifndef _NPRD_CONFIGURATION_H
#define _NPRD_CONFIGURATION_H

namespace npr
{
  class configuration_exception: public std::runtime_error
  {
    public:
      configuration_exception(const std::string& e): std::runtime_error("Configuration exception: " + e) {}
  };

  class Configuration
  {
    #define NPRD_DEFAULT_BASE_CODE "/users/onl/ixp/npr_files/HW_NPUA.uof"

    /* scratch memory */
    #define SCR_MUX_MEMORY_BASE 0x80
    #define SCR_MUX_MEMORY_SIZE 0x10
    #define SCRATCH_SRAM_RING_OCCUPANCY_CNTRS_BASE 0x100
    #define SCRATCH_SRAM_RING_OCCUPANCY_CNTRS_NUM  19
    #define RX_TO_MUX_OCCUPANCY_CNTR (SCRATCH_SRAM_RING_OCCUPANCY_CNTRS_BASE)
    #define PLUGIN_TO_MUX_OCCUPANCY_CNTR (SCRATCH_SRAM_RING_OCCUPANCY_CNTRS_BASE + 4)
    #define XSCALE_TO_MUXQM_OCCUPANCY_CNTR (SCRATCH_SRAM_RING_OCCUPANCY_CNTRS_BASE + 8)
    #define XSCALE_TO_MUXPL_OCCUPANCY_CNTR (SCRATCH_SRAM_RING_OCCUPANCY_CNTRS_BASE + 12)

    #define LOCAL_DELIVERY_RING 1
    #define EXCEPTIONS_RING 2
    #define ERRORS_RING 3
    #define COUNTER_RING 4
    #define FREELIST_RING 5
    #define QM_RING 7
    /* end scratch memory */
    
    #define DROP_COUNTER 32

    #define COUNTER_REQUESTS_DROPPED_ADDR_BASE  0x0200
    #define XSCALE_STATS_REQUESTS_DROPPED_COUNTER 52
    #define XSCALE_STATS_DROPPED_REQUESTS_ADDR COUNTER_REQUESTS_DROPPED_ADDR_BASE+XSCALE_STATS_REQUESTS_DROPPED_COUNTER

    #define QM_NUM_PORTS 5
    #define QM_NUM_QUEUES 0x10000

    /* SRAM bank 0 */
    /* we have discovered that under certain circumstances, bad things happen 
       in SRAM BANK 0.  it appears to be hardware related (bad design) and so we
       can not reliably use BANK 0 for anything.  we use to have all of the
       route/filter results in BANK 0 as such:

       #define SRAM_BANK_0 0
       #define RESULT_BASE_ADDR 0x2000000

       now, they have been moved to BANK 1.
    */
    #define SRAM_BANK_0 1
    #define RESULT_BASE_ADDR 0x400000

    #define RESULT_UNIT_SIZE 16
    #define RESULT_ARRAY_SIZE 0x200000
    #define RESULT_NUM_ENTRIES (RESULT_ARRAY_SIZE / RESULT_UNIT_SIZE)

    /* these next ones are really for the TCAM, and must be updated if the database sizes change (onl_tcam.ini) */
    #define ROUTE_NUM_ENTRIES 0x4000
    #define ROUTE_FIRST_INDEX 1
    #define ROUTE_LAST_INDEX (ROUTE_NUM_ENTRIES - 2)
    #define PFILTER_NUM_ENTRIES 0x8000
    #define PFILTER_FIRST_INDEX 1
    #define PFILTER_LAST_INDEX (PFILTER_NUM_ENTRIES - 2)
    #define AFILTER_NUM_ENTRIES 0x4000
    #define AFILTER_FIRST_INDEX 1
    #define AFILTER_LAST_INDEX (AFILTER_NUM_ENTRIES - 2)
    #define ARP_NUM_ENTRIES 0x4000
    #define ARP_FIRST_INDEX 1
    #define ARP_LAST_INDEX (ARP_NUM_ENTRIES - 2)

    #define ROUTE_MC_NUM_ENTRIES (ROUTE_NUM_ENTRIES / 2)
    #define ROUTE_MC_FIRST_INDEX (ROUTE_FIRST_INDEX)
    #define ROUTE_MC_LAST_INDEX (ROUTE_MC_NUM_ENTRIES - 2)
    #define ROUTE_UC_NUM_ENTRIES (ROUTE_NUM_ENTRIES / 2)
    #define ROUTE_UC_FIRST_INDEX (ROUTE_MC_NUM_ENTRIES + 1)
    #define ROUTE_UC_LAST_INDEX (ROUTE_LAST_INDEX)

    #define MAX_PREFIX_LEN 32
    #define MAX_PRIORITY 63
    #define ROUTE_MC_NUM_PER_CLASS ((ROUTE_MC_LAST_INDEX - ROUTE_MC_FIRST_INDEX) / (MAX_PREFIX_LEN + 1))
    #define ROUTE_UC_NUM_PER_CLASS ((ROUTE_UC_LAST_INDEX - ROUTE_UC_FIRST_INDEX) / (MAX_PREFIX_LEN + 1))
    #define PFILTER_NUM_PER_CLASS ((PFILTER_LAST_INDEX - PFILTER_FIRST_INDEX) / (MAX_PRIORITY + 1))
    #define AFILTER_NUM_PER_CLASS ((AFILTER_LAST_INDEX - AFILTER_FIRST_INDEX) / (MAX_PRIORITY + 1))
    /* end SRAM bank 0 */

    /* SRAM bank 1 */
    #define SRAM_BANK_1 1
    #define QPARAMS_BASE_ADDR 0
    #define QPARAMS_UNIT_SIZE 16
    #define QPARAMS_ARRAY_SIZE (QM_NUM_QUEUES * 16)

    #define QM1_QSCHED_BASE_ADDR (QPARAMS_BASE_ADDR + QPARAMS_ARRAY_SIZE)
    #define QSCHED_UNIT_SIZE 44
    #define QSCHED_NUM 13109
    #define QM2_QSCHED_BASE_ADDR (QM1_QSCHED_BASE_ADDR + (QSCHED_NUM * QSCHED_UNIT_SIZE))
    #define QSCHED_ARRAY_SIZE (2*(QSCHED_NUM * QSCHED_UNIT_SIZE))
    #define QM1_QFREELIST_BASE_ADDR (QM1_QSCHED_BASE_ADDR + QSCHED_ARRAY_SIZE)
    #define QFREELIST_UNIT_SIZE 4
    #define QFREELIST_NUM QSCHED_NUM
    #define QM2_QFREELIST_BASE_ADDR (QM1_QFREELIST_BASE_ADDR + (QFREELIST_UNIT_SIZE * QFREELIST_NUM))
    #define QFREELIST_ARRAY_SIZE (2*(QFREELIST_NUM * QFREELIST_UNIT_SIZE))

    #define QPORT_RATE_CONVERSION 683
    #define QPORT_RATES_BASE_ADDR (QM1_QFREELIST_BASE_ADDR + QFREELIST_ARRAY_SIZE)
    #define QPORT_RATES_UNIT_SIZE 4
    #define QPORT_RATES_NUM QM_NUM_PORTS
    #define QPORT_RATES_ARRAY_SIZE (QPORT_RATES_NUM * QPORT_RATES_UNIT_SIZE)

    #define ONL_SRAM_RINGS_SIZE 0x8000 // longwords
    #define PLUGIN_0_RING 2
    #define PLUGIN_1_RING 3
    #define PLUGIN_2_RING 4
    #define PLUGIN_3_RING 5
    #define PLUGIN_4_RING 6
    #define MUX_RING 8
    #define MUX_PLUGIN_RING 19
    /* end SRAM bank 1 */

    /* SRAM bank 2 */
    #define SRAM_BANK_2 2
    #define BUF_DESC_BASE 0
    #define BUF_DESC_UNIT_SIZE 32
    #define BUF_DESC_NUM 0x38000
    #define BUF_DESC_ARRAY_SIZE (BUF_DESC_UNIT_SIZE * BUF_DESC_NUM)

    #define QD_BASE_ADDR (BUF_DESC_BASE + BUF_DESC_ARRAY_SIZE)
    #define QD_UNIT_SIZE 16
    #define QD_ARRAY_SIZE (QM_NUM_QUEUES * 16)

    #define BUF_FREE_LIST 32
    /* end SRAM bank 2 */

    /* SRAM bank 3 */
    #define SRAM_BANK_3 3
    #define ONL_ROUTER_RESERVED_FOR_COMPILER_BASE 0
    #define ONL_ROUTER_RESERVED_FOR_COMPILER_SIZE 0x10000
    #define COPY_BLOCK_CONTROL_MEM_BASE (ONL_ROUTER_RESERVED_FOR_COMPILER_BASE + ONL_ROUTER_RESERVED_FOR_COMPILER_SIZE)
    #define COPY_BLOCK_CONTROL_MEM_SIZE 0x200

    #define HF_BLOCK_CONTROL_MEM_BASE (COPY_BLOCK_CONTROL_MEM_BASE + COPY_BLOCK_CONTROL_MEM_SIZE)
    #define HF_BLOCK_CONTROL_MEM_SIZE 0x100

    #define LOOKUP_BLOCK_CONTROL_MEM_BASE (HF_BLOCK_CONTROL_MEM_BASE + HF_BLOCK_CONTROL_MEM_SIZE)
    #define LOOKUP_BLOCK_CONTROL_MEM_SIZE 0x100

    #define ONL_ROUTER_PLUGIN_0_BASE 0x100000
    #define ONL_ROUTER_PLUGIN_0_SIZE 0x100000
    #define ONL_ROUTER_PLUGIN_1_BASE (ONL_ROUTER_PLUGIN_0_BASE + ONL_ROUTER_PLUGIN_0_SIZE)
    #define ONL_ROUTER_PLUGIN_1_SIZE 0x100000
    #define ONL_ROUTER_PLUGIN_2_BASE (ONL_ROUTER_PLUGIN_1_BASE + ONL_ROUTER_PLUGIN_1_SIZE)
    #define ONL_ROUTER_PLUGIN_2_SIZE 0x100000
    #define ONL_ROUTER_PLUGIN_3_BASE (ONL_ROUTER_PLUGIN_2_BASE + ONL_ROUTER_PLUGIN_2_SIZE)
    #define ONL_ROUTER_PLUGIN_3_SIZE 0x100000
    #define ONL_ROUTER_PLUGIN_4_BASE (ONL_ROUTER_PLUGIN_3_BASE + ONL_ROUTER_PLUGIN_3_SIZE)
    #define ONL_ROUTER_PLUGIN_4_SIZE 0x100000

    #define ONL_ROUTER_STATS_BASE (ONL_ROUTER_PLUGIN_4_BASE + ONL_ROUTER_PLUGIN_4_SIZE)
    #define ONL_ROUTER_STATS_NUM 0x00010000
    #define ONL_ROUTER_STATS_UNIT_SIZE 16
    #define ONL_ROUTER_STATS_SIZE   (ONL_ROUTER_STATS_NUM * ONL_ROUTER_STATS_UNIT_SIZE)
    #define ONL_ROUTER_STATS_INDEX_MIN 0
    #define ONL_ROUTER_STATS_INDEX_MAX (ONL_ROUTER_STATS_NUM - 1)

    #define ONL_ROUTER_REGISTERS_BASE (ONL_ROUTER_STATS_BASE + ONL_ROUTER_STATS_SIZE)
    #define ONL_ROUTER_REGISTERS_NUM 64
    #define ONL_ROUTER_REGISTERS_UNIT_SIZE 4
    #define ONL_ROUTER_REGISTERS_SIZE   (ONL_ROUTER_REGISTERS_NUM * ONL_ROUTER_REGISTERS_UNIT_SIZE)
    #define ONL_ROUTER_REGISTERS_MIN 0
    #define ONL_ROUTER_REGISTERS_MAX (ONL_ROUTER_REGISTERS_NUM - 1)

    #define TO_PLUGIN_0_CONTROL_RING 9
    #define TO_PLUGIN_1_CONTROL_RING 10
    #define TO_PLUGIN_2_CONTROL_RING 11
    #define TO_PLUGIN_3_CONTROL_RING 12
    #define TO_PLUGIN_4_CONTROL_RING 13

    #define FROM_PLUGIN_0_CONTROL_RING 14
    #define FROM_PLUGIN_1_CONTROL_RING 15
    #define FROM_PLUGIN_2_CONTROL_RING 16
    #define FROM_PLUGIN_3_CONTROL_RING 17
    #define FROM_PLUGIN_4_CONTROL_RING 18
    /* end SRAM bank 3 */

    /* default values for some configurable things */
    #define DEF_MUX_RX_QUANTUM 0x100
    #define DEF_MUX_PL_QUANTUM 0x80
    #define DEF_MUX_XSQM_QUANTUM 0x10
    #define DEF_MUX_XSPL_QUANTUM 0x10

    #define DEF_PORT_RATE 1048576 // 1Kbps * 1024 * 1024

    #define DEF_Q_THRESHOLD 0x80000
    #define DEF_Q_QUANTUM 0x100

    #define DEF_SAMPLE_RATE_00 0xFFFF
    #define DEF_SAMPLE_RATE_01 0x7FFF
    #define DEF_SAMPLE_RATE_10 0x3FFF
    #define DEF_SAMPLE_RATE_11 0x1FFF

    #define DEF_DEST_NR 5
    #define DEF_DEST_AN 5
    #define DEF_DEST_NI 5
    /* end of default values */

    #define FILTER_USE_ROUTE 6
    #define ONL_ROUTER_PLUGIN_0_CNTR_0 38

    public:

      typedef struct _rtm_mac_addrs
      {
        unsigned int port_hi16[5];
        unsigned int port_low32[5];
      } rtm_mac_addrs;

      typedef struct _plugin_file
      {
        std::string label;
        std::string full_path;
      } plugin_file;

      Configuration(unsigned long, rtm_mac_addrs *) throw(configuration_exception);
      ~Configuration() throw();

      void start_router(std::string) throw(configuration_exception);
      void stop_router() throw();
      unsigned int router_started() throw();
      void configure_port(unsigned int port, std::string ip, std::string nexthop) throw(configuration_exception);

      unsigned int find_plugins(std::string, plugin_file **) throw(configuration_exception);
      void add_plugin(std::string, unsigned int) throw(configuration_exception);
      void del_plugin(unsigned int) throw(configuration_exception);
      
      void query_route(route_key *, route_result *) throw(configuration_exception);
      void add_route(route_key *, route_key *, route_result *) throw(configuration_exception);
      void del_route(route_key *) throw(configuration_exception);

      unsigned int conv_str_to_uint(std::string str) throw(configuration_exception);
      unsigned int get_proto(std::string proto_str) throw(configuration_exception);
      unsigned int get_tcpflags(unsigned int fin, unsigned int syn, unsigned int rst, unsigned int psh, unsigned int ack, unsigned urg) throw(configuration_exception);
      unsigned int get_tcpflags_mask(unsigned int fin, unsigned int syn, unsigned int rst, unsigned int psh, unsigned int ack, unsigned urg) throw(configuration_exception);
      unsigned int get_exceptions(unsigned int nonip, unsigned int arp, unsigned int ipopt, unsigned int ttl) throw(configuration_exception);
      unsigned int get_exceptions_mask(unsigned int nonip, unsigned int arp, unsigned int ipopt, unsigned int ttl) throw(configuration_exception);
      unsigned int get_pps(std::string pps_str, bool multicast) throw(configuration_exception);
      unsigned int get_output_port(std::string port_str) throw(configuration_exception);
      unsigned int get_output_plugin(std::string plugin_str) throw(configuration_exception);
      unsigned int get_outputs(std::string port_str, std::string plugin_str) throw(configuration_exception);

      void query_pfilter(pfilter_key *, pfilter_result *) throw(configuration_exception);
      void add_pfilter(pfilter_key *, pfilter_key *, unsigned int, pfilter_result *) throw(configuration_exception);
      void del_pfilter(pfilter_key *) throw(configuration_exception);

      void query_afilter(afilter_key *, afilter_result *) throw(configuration_exception);
      void add_afilter(afilter_key *, afilter_key *, unsigned int, afilter_result *) throw(configuration_exception);
      void del_afilter(afilter_key *) throw(configuration_exception);
      void get_sample_rates(aux_sample_rates *) throw();
      void set_sample_rates(aux_sample_rates *) throw();

      void query_arp(arp_key *, arp_result *) throw(configuration_exception);
      void update_arp(arp_key *, arp_result *) throw(configuration_exception);
      void add_arp(arp_key *, arp_key *, arp_result *) throw(configuration_exception);
      void del_arp(arp_key *) throw(configuration_exception);

      unsigned int get_queue_quantum(unsigned int, unsigned int) throw();
      void set_queue_quantum(unsigned int, unsigned int, unsigned int) throw();
      unsigned int get_queue_threshold(unsigned int, unsigned int) throw();
      void set_queue_threshold(unsigned int, unsigned int, unsigned int) throw();

      void get_port_rates(port_rates *) throw();
      void set_port_rates(port_rates *) throw();
      unsigned int get_port_rate(unsigned int) throw(configuration_exception); 
      void set_port_rate(unsigned int, unsigned int) throw(configuration_exception); 

      void get_mux_quanta(mux_quanta *) throw();
      void set_mux_quanta(mux_quanta *) throw();

      void get_exception_dests(exception_dests *) throw();
      void set_exception_dests(exception_dests *) throw();

      unsigned int get_port_mac_addr_hi16(unsigned int) throw(configuration_exception);
      unsigned int get_port_mac_addr_low32(unsigned int) throw(configuration_exception);

      unsigned int get_port_addr(unsigned int port) throw();
      unsigned int get_next_hop_addr(unsigned int port) throw();
      void set_username(std::string un) throw();
      std::string get_username() throw();
      
      unsigned int get_sram_map(unsigned int) throw(configuration_exception);

      void increment_drop_counter() throw();
      void update_stats_index(unsigned int, unsigned int) throw();

    private:
      rtm_mac_addrs macs;

      std::string ixp_version;
      int tcamd_conn;
      void *router_base_handle;
      unsigned int used_me_mask;

      unsigned int npu;  // 1 is NPUA, 0 is NPUB
      unsigned int control_address;
      unsigned int port_addrs[5];
      unsigned int next_hops[5];
      std::string username;

      std::string plugin_uof_files[5];
      void *plugin_uof_handles[5];

      pthread_mutex_t tcam_lock;    // only for tcam ops
      pthread_mutex_t sram_lock;    // only for sram ops
      pthread_mutex_t scratch_lock; // only for scratch ops
      pthread_mutex_t conf_lock;    // anything else

      #define STOP  0
      #define START 1
      int state;
   
      // need a mapping of current filters and routes with their sram result addresses so that
      // we can uniquely identify them in the ARP code
      // ... this is so horrible ...
      unsigned int next_id;
      typedef struct _sram_result_map
      {
        unsigned int sram_addr;
        unsigned int id;
        struct _sram_result_map *next;
      } sram_result_map;

      sram_result_map *srmap;

      typedef struct _indexlist
      {
        unsigned int left;
        unsigned int right;
        struct _indexlist *next;
      } indexlist;

      indexlist *sram_free_list;

      indexlist *mc_routes[MAX_PREFIX_LEN+1];
      indexlist *uc_routes[MAX_PREFIX_LEN+1];
      indexlist *pfilters[MAX_PRIORITY+1];
      indexlist *afilters[MAX_PRIORITY+1];
      indexlist *arps;

      void start_mes() throw(configuration_exception);
      void stop_mes() throw(configuration_exception);
      void stop_mes(unsigned int) throw(configuration_exception);

      void add_sram_map(unsigned int, unsigned int) throw();
      void del_sram_map(unsigned int) throw(configuration_exception);

      unsigned int get_prefix_length(unsigned int) throw();
      unsigned int get_free_index(indexlist **) throw(configuration_exception);
      void add_free_index(indexlist **, unsigned int) throw();
      void free_list_mem(indexlist **) throw();
    
      unsigned int sram_index2addr(unsigned int) throw();
      unsigned int sram_addr2index(unsigned int) throw();
  };
};

#endif // _NPRD_CONFIGURATION_H
