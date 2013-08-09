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

#ifndef _NPRD_IPV4_H
#define _NPRD_IPV4_H

namespace npr
{
  class IPv4
  {
    public:
      IPv4() throw();
      ~IPv4() throw();

      void send_ttl_expired(DataPathMessage *) throw();
      void send_no_route(DataPathMessage *) throw();

      void handle_ip_options(DataPathMessage *) throw();
      void handle_next_hop_invalid(DataPathMessage *) throw();
      void handle_local_delivery(DataPathMessage *) throw();
      void handle_general_error(DataPathMessage *) throw();
  
    private:
      unsigned int compute_cksm(unsigned int *, unsigned int) throw();
      unsigned int compute_partial_cksm(unsigned int, unsigned int *, unsigned int) throw();
      void read_in(unsigned int, unsigned int *, unsigned int) throw();
      void write_out(unsigned int, unsigned int *, unsigned int) throw();

      typedef union __attribute__ ((__packed__)) _ipv4_hdr
      {
        struct __attribute__ ((__packed__))
        {
          unsigned int ip_version     : 4;
          unsigned int ip_hdr_len     : 4;
          unsigned int ip_tos         : 8;
          unsigned int ip_len         : 16;
          unsigned int ip_id          : 16;
          unsigned int ip_flags       : 3;
          unsigned int ip_frag_offset : 13;
          unsigned int ip_ttl         : 8;
          unsigned int ip_proto       : 8;
          unsigned int ip_hdr_cksm    : 16;
          unsigned int ip_src_addr    : 32;
          unsigned int ip_dest_addr   : 32;
        };
        unsigned int v[5];
      } ipv4_hdr;

      typedef union __attribute__ ((__packed__)) _icmp_hdr
      {
        struct __attribute__ ((__packed__))
        {
          unsigned int icmp_type      : 8;
          unsigned int icmp_code      : 8;
          unsigned int icmp_cksm      : 16;
          unsigned int icmp_id        : 16;
          unsigned int icmp_seq       : 16;
        };
        unsigned int v[2];
      } icmp_hdr;
  };
};

#endif // _NPRD_IPV4_H
