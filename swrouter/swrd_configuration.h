#ifndef _SWRD_CONFIGURATION_H
#define _SWRD_CONFIGURATION_H

namespace swr
{
  class configuration_exception: public std::runtime_error
  {
    public:
      configuration_exception(const std::string& e): std::runtime_error("Configuration exception: " + e) {}
  };

  class Configuration
  {
    #define QM_NUM_PORTS 5
    #define QM_NUM_QUEUES 0x10000

 
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

    public:

      typedef struct _rtm_mac_addrs
      {
        unsigned int port_hi16[5];
        unsigned int port_low32[5];
      } rtm_mac_addrs;

      Configuration(rtm_mac_addrs *) throw(configuration_exception);
      ~Configuration() throw();

      void start_router(std::string) throw(configuration_exception);
      void stop_router() throw();
      unsigned int router_started() throw();
      void configure_port(unsigned int port, std::string ip, std::string nexthop) throw(configuration_exception);

      void add_route(route_key *, route_key *, route_result *) throw(configuration_exception);
      void del_route(route_key *) throw(configuration_exception);

      //following methods are for converting user entered RLI values into something that means something to the system
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
      //end RLI value conversion

      void query_filter(pfilter_key *, pfilter_result *) throw(configuration_exception);
      void add_filter(pfilter_key *, pfilter_key *, unsigned int, pfilter_result *) throw(configuration_exception);
      void del_filter(pfilter_key *) throw(configuration_exception);

      //void get_sample_rates(aux_sample_rates *) throw();
      //void set_sample_rates(aux_sample_rates *) throw();


      unsigned int get_queue_quantum(unsigned int, unsigned int) throw();
      void set_queue_quantum(unsigned int, unsigned int, unsigned int) throw();
      unsigned int get_queue_threshold(unsigned int, unsigned int) throw();
      void set_queue_threshold(unsigned int, unsigned int, unsigned int) throw();

      void get_port_rates(port_rates *) throw();
      void set_port_rates(port_rates *) throw();
      unsigned int get_port_rate(unsigned int) throw(configuration_exception); 
      void set_port_rate(unsigned int, unsigned int) throw(configuration_exception); 

      unsigned int get_port_mac_addr_hi16(unsigned int) throw(configuration_exception);
      unsigned int get_port_mac_addr_low32(unsigned int) throw(configuration_exception);

      unsigned int get_port_addr(unsigned int port) throw();
      unsigned int get_next_hop_addr(unsigned int port) throw();
      void set_username(std::string un) throw();
      std::string get_username() throw();
      

    private:
      rtm_mac_addrs macs;


      unsigned int control_address;
      unsigned int port_addrs[5];
      unsigned int next_hops[5];
      std::string username;


      pthread_mutex_t conf_lock;    // anything else

      #define STOP  0
      #define START 1
      int state;
   
      // need a mapping of current filters and routes with their sram result addresses so that
      // we can uniquely identify them in the ARP code
      // ... this is so horrible ...
      unsigned int next_id;

      typedef struct _indexlist
      {
        unsigned int left;
        unsigned int right;
        struct _indexlist *next;
      } indexlist;


      indexlist *mc_routes[MAX_PREFIX_LEN+1];
      indexlist *uc_routes[MAX_PREFIX_LEN+1];
      indexlist *pfilters[MAX_PRIORITY+1];

      unsigned int get_prefix_length(unsigned int) throw();
      unsigned int get_free_index(indexlist **) throw(configuration_exception);
      void add_free_index(indexlist **, unsigned int) throw();
      void free_list_mem(indexlist **) throw();
    
  };
};

#endif // _SWRD_CONFIGURATION_H
