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

#ifndef _NPRD_ARP_H
#define _NPRD_ARP_H

namespace npr
{
  class arp_exception: public std::runtime_error
  {
    public:
      arp_exception(const std::string& e): std::runtime_error("ARP exception: " + e) {}
  };

  class ARP
  {
    public:
      ARP() throw();
      ~ARP() throw();

      void process_arp_packet(DataPathMessage *) throw(arp_exception);
      void send_arp_request(DataPathMessage *) throw(arp_exception);
  
    private:
      pthread_mutex_t request_list_lock;

      typedef struct _nhrnode
      {
        DataPathMessage *nhr_msg;
        unsigned int sram_addr;
        unsigned int sram_id;
        struct _nhrnode *next;
      } nhrnode;

      #define ARP_OUTSTANDING 0
      #define ARP_FINISHED    1
      #define ARP_FAILED      2
      typedef struct _requestnode
      {
        unsigned int ipaddr;
        unsigned int macaddr_hi16;
        unsigned int macaddr_low32;
        unsigned int state;
        //unsigned int valid_dip_msg;
        unsigned int valid_arp_db_entry;
        DataPathMessage *dip_msg;
        nhrnode *nhr_list;
        struct _requestnode *next;
      } requestnode;

      requestnode *request_list;

      void send_packet_to_data_path(requestnode *, plc_to_xscale_data *) throw(arp_exception);
      requestnode *add_request(requestnode **, unsigned int);
      requestnode *find_request(requestnode *, unsigned int);
      nhrnode *find_nhr(requestnode *, unsigned int);
      void free_list_mem(requestnode **);

      typedef union __attribute__ ((__packed__)) _arp_packet
      {
        struct __attribute__ ((__packed__))
        {
          unsigned int dest_mac_hi16      : 16;
          unsigned int dest_mac_low32     : 32;
          unsigned int src_mac_hi16       : 16;
          unsigned int src_mac_low32      : 32;
          unsigned int ethertype          : 16;
          unsigned int hw_type            : 16;
          unsigned int proto_type         : 16;
          unsigned int hw_length          : 8;
          unsigned int proto_length       : 8;
          unsigned int operation          : 16;
          unsigned int src_hw_addr_hi16   : 16;
          unsigned int src_hw_addr_low32  : 32;
	  unsigned int src_proto_addr     : 32;
          unsigned int trgt_hw_addr_hi16  : 16;
          unsigned int trgt_hw_addr_low32 : 32;
	  unsigned int trgt_proto_addr    : 32;
          unsigned int res1               : 16;
          unsigned int res2               : 32;
          unsigned int res3               : 32;
          unsigned int res4               : 32;
          unsigned int res5               : 32;
          unsigned int res6               : 32;
        };
        unsigned int v[16];
      } arp_packet;
  };
};

#endif // _NPRD_ARP_H
