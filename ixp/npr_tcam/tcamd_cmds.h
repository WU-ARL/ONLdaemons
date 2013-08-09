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

#ifndef _TCAMD_CMDS_H
#define _TCAMD_CMDS_H

namespace tcamd 
{
  #define DEFAULT_TCAMD_PORT 3559

  typedef unsigned int tcam_command;
  typedef unsigned int tcam_response;
  typedef unsigned int tcam_result;
  typedef unsigned int tcam_index;

  // the first word in the request packet payload will contain the command
  const tcam_command ROUTERINIT   =  0; // initialization (per router)
  const tcam_command ROUTERSTOP   =  1; // clean-up (per router)
  const tcam_command ADDROUTE     =  2;
  const tcam_command DELROUTE     =  3;
  const tcam_command QUERYROUTE   =  4;
  const tcam_command ADDPFILTER   =  5;
  const tcam_command DELPFILTER   =  6;
  const tcam_command QUERYPFILTER =  7;
  const tcam_command ADDAFILTER   =  8;
  const tcam_command DELAFILTER   =  9;
  const tcam_command QUERYAFILTER = 10;
  const tcam_command ADDARP       = 11;
  const tcam_command DELARP       = 12;
  const tcam_command QUERYARP     = 13;

  // the first word in the return packet payload will contain the return code
  const tcam_response SUCCESS         = 0;
  const tcam_response FAILURE         = 1;
  const tcam_response INUSE           = 2;
  const tcam_response ALREADYINVALID  = 3;
  const tcam_response NOTFOUND        = 4;
  const tcam_response UNKNOWN         = 5;

  typedef struct _route_info 
  {
    npr::route_key key;
    npr::route_key mask;
    tcam_result result;
    tcam_index index;
  } route_info;

  typedef struct _pfilter_info 
  {
    npr::pfilter_key key;
    npr::pfilter_key mask;
    tcam_result result;
    tcam_index index;
  } pfilter_info;

  typedef struct _afilter_info 
  {
    npr::afilter_key key;
    npr::afilter_key mask;
    tcam_result result;
    tcam_index index;
  } afilter_info;

  typedef struct _arp_info 
  {
    npr::arp_key key;
    npr::arp_key mask;
    tcam_result result;
    tcam_index index;
  } arp_info;
};
#endif // _TCAMD_CMDS_H
