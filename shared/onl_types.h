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

#ifndef _ONL_TYPES_H
#define _ONL_TYPES_H

namespace onld
{
  class nccp_string
  {
    public:
      nccp_string();
      nccp_string(const char* c);
      nccp_string(const char* c, uint8_t len); //not null terminated
      nccp_string(const nccp_string& s);
      ~nccp_string();

      nccp_string& operator=(const nccp_string& s);
      nccp_string& operator=(const char* c);
      bool operator==(const nccp_string& s);
      bool operator<(const nccp_string& s) const;
      bool operator>(const nccp_string& s) const;

      char* getCString() { return nccp_str; }
      std::string getString() { return nccp_str; }
      uint8_t getLength() { return length; }
      uint32_t getAsIP();

      friend byte_buffer& operator<<(byte_buffer& buf, nccp_string& s);
      friend byte_buffer& operator>>(byte_buffer& buf, nccp_string& s);

    protected:
     char nccp_str[256];
     uint8_t length;
  }; // class nccp_string

  byte_buffer& operator<<(byte_buffer& buf, nccp_string& s);
  byte_buffer& operator>>(byte_buffer& buf, nccp_string& s);

  class experiment_info
  {
    public:
      experiment_info();
      experiment_info(const experiment_info& eid);
      ~experiment_info();

      experiment_info& operator=(const experiment_info& eid);
      bool operator==(const experiment_info& eid);

      void setID(std::string str) { ID = str.c_str(); }
      std::string getID() { return ID.getString(); }
      void setExpName(std::string str) { expName = str.c_str(); }
      std::string getExpName() { return expName.getString(); }
      void setUserName(std::string str) { userName = str.c_str(); }
      std::string getUserName() { return userName.getString(); }

      friend byte_buffer& operator<<(byte_buffer& buf, experiment_info& eid);
      friend byte_buffer& operator>>(byte_buffer& buf, experiment_info& eid);

    private:
      nccp_string ID;
      nccp_string expName;
      nccp_string userName;
  }; // class experiment_info

  byte_buffer& operator<<(byte_buffer& buf, experiment_info& eid);
  byte_buffer& operator>>(byte_buffer& buf, experiment_info& eid);

  class component
  {
    public:
      component();
      component(std::string t, uint32_t i);
      component(const component& c);
      virtual ~component();

      component& operator=(const component& c);
    
      std::string getType() { return type.getString(); }
      void setType(std::string t) { type = t.c_str(); }
      uint32_t getID() { return id; }
      void setID(uint32_t i) { id = i; }
      uint32_t getParent() { return parent; }
      void setParent(uint32_t p) { parent = p; }
      std::string getLabel() { return label.getString(); }
      void setLabel(std::string l) { label = l.c_str(); }
      std::string getCP() { return cp.getString(); }
      void setCP(std::string c) { cp = c.c_str(); }
      bool isRouter() { return is_router;}
      void setIsRouter(bool b) { is_router = b;}

      friend byte_buffer& operator<<(byte_buffer& buf, component& c);
      friend byte_buffer& operator>>(byte_buffer& buf, component& c);

    protected:
      nccp_string type;  // component type (e.g., npr, nsp, host...)
      uint32_t id;       // unique id for this component
      uint32_t parent;   // id of parent cluster, 0 if not part of one
      nccp_string label; // RLI/user provided label for this component
      nccp_string cp;    // the CP of the component, if user is requesting a
                         //   specific one, blank otherwise
      bool is_router;    // true if this is a router component, false otherwise
  }; // class component

  byte_buffer& operator<<(byte_buffer& buf, component& c);
  byte_buffer& operator>>(byte_buffer& buf, component& c);

  class cluster: public component
  {
    public:
      cluster();
      cluster(std::string t, uint32_t i);
      cluster(const cluster& c);
      virtual ~cluster();

      cluster& operator=(const cluster& c);

      void parseExtra(byte_buffer& buf);

      friend byte_buffer& operator<<(byte_buffer& buf, cluster& c);
      friend byte_buffer& operator>>(byte_buffer& buf, cluster& c);

    protected:
      uint32_t hardware[255];
      uint32_t numHardware;
  }; // class cluster

  byte_buffer& operator<<(byte_buffer& buf, cluster& c);
  byte_buffer& operator>>(byte_buffer& buf, cluster& c);

  //cgw, anyway we can ditch this? i don't think the addr/port is used anywhere..
  class experiment
  {
    public:
      experiment();
      experiment(std::string a, uint32_t p, experiment_info& ei);
      experiment(const experiment& e);
      ~experiment();

      experiment& operator=(const experiment& e);

      void setExpInfo(experiment_info& ei) { info = ei; }
      experiment_info& getExpInfo() { return info; }

      friend byte_buffer& operator<<(byte_buffer& buf, experiment& e);
      friend byte_buffer& operator>>(byte_buffer& buf, experiment& e);

    private:
      nccp_string addr;
      uint32_t port;
      experiment_info info;
  }; // class experiment

  byte_buffer& operator<<(byte_buffer& buf, experiment& e);
  byte_buffer& operator>>(byte_buffer& buf, experiment& e);

  typedef uint16_t paramtype;
  static const paramtype unknown_param  = 0;
  static const paramtype int_param      = 1;
  static const paramtype double_param   = 2;
  static const paramtype bool_param     = 3;
  static const paramtype string_param   = 4;
  static const paramtype max_param_type = string_param;

  class param
  {
    public:
      param();
      param(std::string s);
      param(const param& p);
      ~param();

      param& operator=(const param& p);

      paramtype getType() { return pt; }
      uint32_t getInt() { return int_val; }
      double getDouble() { return double_val; }
      bool getBool() { return bool_val; }
      std::string getString() { return string_val.getString(); }
      
      friend byte_buffer& operator<<(byte_buffer& buf, param& p);
      friend byte_buffer& operator>>(byte_buffer& buf, param& p);

    private:
      paramtype pt;
      uint32_t int_val;
      double double_val;
      bool bool_val;
      nccp_string string_val;
  }; // class param

  byte_buffer& operator<<(byte_buffer& buf, param& p);
  byte_buffer& operator>>(byte_buffer& buf, param& p);

  byte_buffer& operator<<(byte_buffer& buf, std::list<param>& plist);
  byte_buffer& operator>>(byte_buffer& buf, std::list<param>& plist);

  byte_buffer& operator<<(byte_buffer& buf, std::vector<param>& pvec);
  byte_buffer& operator>>(byte_buffer& buf, std::vector<param>& pvec);

  class node_info
  {
    public:
      node_info();
      //node_info(std::string ip, std::string sn, uint32_t portnum, std::string rtype, bool rrouter, std::string next_hop_ip);
      node_info(std::string ip, std::string sn, uint32_t portnum, std::string rtype, bool rrouter, std::string next_hop_ip, uint32_t rport);
      node_info(const node_info& ni);
      ~node_info();
 
      node_info& operator=(const node_info& ni);
  
      std::string getIPAddr() { return ipaddr.getString(); }
      std::string getSubnet() { return subnet.getString(); }
      uint32_t getPort() { return port; }
      uint32_t getRealPort() { return real_port; }
      std::string getRemoteType() { return remote_type.getString(); }
      bool isRemoteRouter() { return is_remote_router; }
      std::string getNHIPAddr() { return nexthop_ipaddr.getString(); }
      uint32_t getVLan() { return vlanid;}
      void setVLan(uint32_t v) { vlanid = v;}
      uint32_t getBandwidth() { return bandwidth;}
      void setBandwidth(uint32_t bw) { bandwidth = bw;} 

      friend byte_buffer& operator<<(byte_buffer& buf, node_info& ni);
      friend byte_buffer& operator>>(byte_buffer& buf, node_info& ni);

    private:
      nccp_string ipaddr;
      nccp_string subnet;
      uint32_t port;//for components that support virtual ports this is the virtual port index
      uint32_t real_port;//used for components that support virtual ports this is the real interface assigned
      uint32_t vlanid;
      uint32_t bandwidth; //in Mbits/s
      nccp_string remote_type; //component we're connected to's type
      bool is_remote_router; //if the component we're connected to is a router
      nccp_string nexthop_ipaddr;
  }; // class node_info

  byte_buffer& operator<<(byte_buffer& buf, node_info& ni);
  byte_buffer& operator>>(byte_buffer& buf, node_info& ni);
};

#endif // _ONL_TYPES_H
