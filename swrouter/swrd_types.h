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

#ifndef _SWRD_TYPES_H
#define _SWRD_TYPES_H

namespace swr 
{

  #define WILDCARD_VALUE 0xffffffff

  // These types do not need to be packed as they were in the NPR.
  // These are not going to be used to write directly into a device memory like the NPR.

/*
  typedef struct _route_key
  {
    uint32_t daddr;
  } ;

  typedef struct _route_result
  {
      uint16_t outputPort;   // typedef struct _port_info holds the vlan and device info
      uint32_t gateway;
  } route_result;

  typedef struct _route_entry
  {
    route_key key;
    route_key mask;
    route_result result;
  } route_entry;
*/

  typedef struct _filter_key
  {
      uint32_t daddr;
      uint32_t dmask;
      uint32_t saddr;
      uint32_t smask;
      uint16_t dport;
      uint16_t sport;
      uint8_t proto;
      uint8_t tcp_flags;
  } filter_key;

  typedef struct _filter_result
  {
      uint16_t qid;
      uint8_t drop;
      uint16_t outputPort;   // typedef struct _port_info holds the vlan and device info
      uint32_t gateway;
  } filter_result;

  typedef struct _filter_entry
  {
    filter_key key;
    filter_key mask;
    filter_result result;
    uint8_t priority;
  } filter_entry;


};

#endif // _SWRD_TYPES_H
