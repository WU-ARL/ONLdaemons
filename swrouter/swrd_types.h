#ifndef _SWRD_TYPES_H
#define _SWRD_TYPES_H

namespace swr 
{

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

};

#endif // _SWRD_TYPES_H
