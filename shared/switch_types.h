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

#ifndef _SWITCH_TYPES_H
#define _SWITCH_TYPES_H

using std::ostream;

namespace onld
{
  class switch_port
  {
    public:
      switch_port();
      switch_port(const std::string switch_id, uint32_t portnum);
      switch_port(const std::string switch_id, uint32_t portnum, bool interswitch_port);
      switch_port(const switch_port& sp);
      ~switch_port();

      switch_port& operator=(const switch_port& sp);
      bool operator==(const switch_port& sp);
      bool operator<(const switch_port& sp) const;

      bool isValid();

      std::string getSwitchId() { return switchid.getString(); }
      uint32_t getPortNum() { return port; }
      bool isInterSwitchPort() { return is_interswitch_port; }

      friend byte_buffer& operator<<(byte_buffer& buf, switch_port& sp);
      friend byte_buffer& operator>>(byte_buffer& buf, switch_port& sp);

      friend ostream& operator<<(ostream& os, switch_port& sp);
        
    private:
      nccp_string switchid;
      uint32_t port;
      bool is_interswitch_port;
  }; // class switch_port


  byte_buffer& operator<<(byte_buffer& buf, switch_port& sp);
  byte_buffer& operator>>(byte_buffer& buf, switch_port& sp);

  class switch_info
  {
    public:
      switch_info();
      switch_info(const std::string switch_id, uint32_t num_ports);
      switch_info(const switch_info& si);
      ~switch_info();

      void set_management_port(uint32_t port) 
           {management_port = port;}

      std::string getSwitchId() { return switchid.getString(); }
      uint32_t getNumPorts() { return ports; }
      uint32_t getManagementPort() { return management_port; }
      
      switch_info& operator=(const switch_info& si);

      friend byte_buffer& operator<<(byte_buffer& buf, switch_info& sp);
      friend byte_buffer& operator>>(byte_buffer& buf, switch_info& sp);

      friend ostream& operator<<(ostream& os, switch_info& sp);
 
    private:
      nccp_string switchid;
      uint32_t ports;
      uint32_t management_port;
  }; // class switch_info

  byte_buffer& operator<<(byte_buffer& buf, switch_info& sp);
  byte_buffer& operator>>(byte_buffer& buf, switch_info& sp);

  typedef uint32_t switch_vlan;
};

#endif // _SWITCH_TYPES_H
