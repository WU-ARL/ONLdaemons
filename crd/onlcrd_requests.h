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

#ifndef _ONLCRD_REQUESTS_H
#define _ONLCRD_REQUESTS_H

namespace onlcrd
{
  class ping_req : public onld::request
  {
    public:
      ping_req(uint8_t* mbuf, uint32_t size);
      virtual ~ping_req();
 
      virtual bool handle();
  }; // class ping_req

  class i_am_up_req : public onld::i_am_up
  {
    public:
      i_am_up_req(uint8_t* mbuf, uint32_t size);
      virtual ~i_am_up_req();
  
      virtual bool handle();
  }; // class i_am_up_req

  class rlicrd_request : public onld::request
  {
    public:
      rlicrd_request(uint8_t* mbuf, uint32_t size);
      virtual ~rlicrd_request();

      virtual void parse();

      experiment_info& getExpInfo() { return expinfo; }
      component& getComponent() { return comp; }

    protected:
      experiment_info expinfo; 
      component comp; 
  }; // class rlicrd_request

  static const NCCP_OperationType NCCP_Operation_BeginSession = 48;
  class begin_session_req : public onld::request
  {
    public:
      begin_session_req(uint8_t* mbuf, uint32_t size);
      virtual ~begin_session_req();

      uint32_t getNumRequests() { return num_topology_reqs; }

      virtual bool handle();

      virtual void parse();

    protected:
      experiment_info expinfo;
      nccp_string password;
      uint32_t num_topology_reqs;
  }; // class begin_session_req

  static const NCCP_OperationType NCCP_Operation_SessionAddComponent = 40;
  class session_add_component_req : public rlicrd_request
  {
    public:
      session_add_component_req(uint8_t* mbuf, uint32_t size);
      virtual ~session_add_component_req();

      std::list<param>& getRebootParams() { return reboot_params; }
      std::list<param>& getInitParams() { return init_params; }
      std::string getIPAddr() { return ipaddr.getString(); }
      std::string getExtTag() { return ext_tag.getString(); }
      uint32_t getCores() { return cores;}
      uint32_t getMemory() { return mem;}
      uint32_t getNumInterfaces() { return num_interfaces;}
      uint32_t getInterfaceBW() { return interfacebw;}
      virtual bool handle();

      virtual void parse();

    protected:
      nccp_string ipaddr;
      uint32_t cores;
      uint32_t mem;
      uint32_t num_interfaces;
      uint32_t interfacebw;
      nccp_string ext_tag;
      std::list<param> reboot_params;
      std::list<param> init_params;
  }; // class session_add_component_req

  static const NCCP_OperationType NCCP_Operation_SessionAddCluster = 42;
  class session_add_cluster_req : public rlicrd_request
  {
    public:
      session_add_cluster_req(uint8_t* mbuf, uint32_t size);
      virtual ~session_add_cluster_req();

      virtual bool handle();

      virtual void parse();

    protected:
      cluster clus;
  }; // class session_add_cluster_req

  static const NCCP_OperationType NCCP_Operation_SessionAddLink = 38;
  class session_add_link_req : public rlicrd_request
  {
    public:
      session_add_link_req(uint8_t* mbuf, uint32_t size);
      virtual ~session_add_link_req();

      component& getFromComponent() { return from_comp; }
      uint16_t getFromPort() { return from_port; }
      std::string getFromIP() { return from_ip.getString(); }
      std::string getFromSubnet() { return from_subnet.getString(); }
      std::string getFromNHIP() { return from_nhip.getString(); }
      uint32_t getFromCapacity() { return from_cap;}
      component& getToComponent() { return to_comp; }
      uint16_t getToPort() { return to_port; }
      std::string getToIP() { return to_ip.getString(); }
      std::string getToSubnet() { return to_subnet.getString(); }
      std::string getToNHIP() { return to_nhip.getString(); }
      uint32_t getToCapacity() { return to_cap;}
      uint32_t getCapacity() { return bandwidth;}

      virtual bool handle();

      virtual void parse();

    protected:
      component from_comp;
      uint16_t from_port;
      nccp_string from_ip;
      nccp_string from_subnet;
      nccp_string from_nhip;
      uint32_t from_cap;
      component to_comp;
      uint16_t to_port;
      nccp_string to_ip;
      nccp_string to_subnet;
      nccp_string to_nhip;
      uint32_t to_cap;
      uint32_t bandwidth;
  }; // class session_add_link_req

  static const NCCP_OperationType NCCP_Operation_CommitSession = 44;
  class commit_session_req : public onld::request
  {
    public:
      commit_session_req(uint8_t* mbuf, uint32_t size);
      virtual ~commit_session_req();

      virtual bool handle();

      virtual void parse();

    protected:
      nccp_string eid;
  }; // class commit_session_req

  static const NCCP_OperationType NCCP_Operation_EndSession = 43;
  class clear_session_req : public onld::request
  {
    public:
      clear_session_req(uint8_t* mbuf, uint32_t size);
      virtual ~clear_session_req();

      virtual bool handle();

      virtual void parse();

    protected:
      nccp_string eid;
  }; // class clear_session_req

  class rli_ping : public onld::periodic
  {
    public:
      rli_ping(request* req); 
      virtual ~rli_ping();
  }; // class rli_ping

  static const NCCP_OperationType NCCP_Operation_BeginReservation = 47;
  class begin_reservation_req : public onld::request
  {
    public:
      begin_reservation_req(uint8_t* mbuf, uint32_t size);
      virtual ~begin_reservation_req();

      std::string getEarlyStart() { return earlyStart.getString(); }
      std::string getLateStart() { return lateStart.getString(); }
      std::string getUserName() { return username.getString(); }
      uint32_t getDuration() { return duration; }
      uint32_t getNumRequests() { return num_topology_reqs; }

      virtual bool handle();

      void commit_done(bool success, std::string msg, std::string start_time);

      virtual void parse();

    protected:
      nccp_string username;
      nccp_string password;
      nccp_string earlyStart;
      nccp_string lateStart;
      uint32_t duration; //in minutes
      uint32_t num_topology_reqs;
  }; // class begin_reservation_req

  static const NCCP_OperationType NCCP_Operation_ReservationAddComponent = 59;
  class reservation_add_component_req : public rlicrd_request
  {
    public:
      reservation_add_component_req(uint8_t* mbuf, uint32_t size);
      virtual ~reservation_add_component_req();
      uint32_t getCores() { return cores;}
      uint32_t getMemory() { return mem;}
      uint32_t getNumInterfaces() { return num_interfaces;}
      uint32_t getInterfaceBW() { return interfacebw;}

      virtual bool handle();

      virtual void parse();

    protected:
      nccp_string ipaddr;
      uint32_t cores;
      uint32_t mem;
      uint32_t num_interfaces;
      uint32_t interfacebw;
      std::list<param> reboot_params;
      std::list<param> init_params;
  }; // class reservation_add_component_req

  static const NCCP_OperationType NCCP_Operation_ReservationAddCluster = 60;
  class reservation_add_cluster_req : public rlicrd_request
  {
    public:
      reservation_add_cluster_req(uint8_t* mbuf, uint32_t size);
      virtual ~reservation_add_cluster_req();

      virtual bool handle();

      virtual void parse();

    protected:
      cluster clus;
  }; // class reservation_add_cluster_req

  static const NCCP_OperationType NCCP_Operation_ReservationAddLink = 58;
  class reservation_add_link_req : public rlicrd_request
  {
    public:
      reservation_add_link_req(uint8_t* mbuf, uint32_t size);
      virtual ~reservation_add_link_req();

      component& getFromComponent() { return from_comp; }
      uint16_t getFromPort() { return from_port; }
      component& getToComponent() { return to_comp; }
      uint16_t getToPort() { return to_port; }

      virtual bool handle();

      virtual void parse();

    protected:
      component from_comp;
      uint16_t from_port;
      nccp_string from_ip;
      nccp_string from_subnet;
      nccp_string from_nhip;
      uint32_t from_cap;
      component to_comp;
      uint16_t to_port;
      nccp_string to_ip;
      nccp_string to_subnet;
      nccp_string to_nhip;
      uint32_t to_cap;
      uint32_t bandwidth;
  }; // class reservation_add_link_req

  static const NCCP_OperationType NCCP_Operation_CommitReservation = 61;
  class commit_reservation_req : public onld::request
  {
    public:
      commit_reservation_req(uint8_t* mbuf, uint32_t size);
      virtual ~commit_reservation_req();

      virtual bool handle();

      virtual void parse();

    protected:
      nccp_string eid;
  }; // class commit_reservation_req

  static const NCCP_OperationType NCCP_Operation_ExtendReservation = 49;
  class extend_reservation_req : public onld::request
  {
    public:
      extend_reservation_req(uint8_t* mbuf, uint32_t size);
      virtual ~extend_reservation_req();

      virtual bool handle();

      virtual void parse();

    protected:
      nccp_string username;
      nccp_string password;
      uint32_t extend_mins;
  }; // class extend_reservation_req

  static const NCCP_OperationType NCCP_Operation_CancelReservation = 50;
  class cancel_reservation_req : public onld::request
  {
    public:
      cancel_reservation_req(uint8_t* mbuf, uint32_t size);
      virtual ~cancel_reservation_req();

      virtual bool handle();

      virtual void parse();

    protected:
      nccp_string username;
      nccp_string password;
  }; // class cancel_reservation_req
};

#endif // _ONLCRD_REQUESTS_H
