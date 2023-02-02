/*
 * Copyright (c) 2022 Jyoti Parwatikar and John DeHart
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

#ifndef _ONLVPNBASED_SESSION_H
#define _ONLVPNBASED_SESSION_H

namespace onlvpnbased
{

  struct _devinterface_info;

  typedef struct _dev_info
  { 
    std::string ctladdr;
    component comp;
    std::list< boost::shared_ptr<_devinterface_info> > interfaces;
    std::string expaddr;
    uint32_t cores;
    uint32_t memory;
    std::string name;
    std::string table_id;
  } dev_info;

  typedef boost::shared_ptr<dev_info> dev_ptr;

  struct _vlan_info;

  typedef struct _devinterface_info
  {
    node_info ninfo;
    boost::shared_ptr<_vlan_info> vlan;
    dev_ptr dev;
    std::string subn_addr;
  } devinterface_info;

  typedef boost::shared_ptr<devinterface_info> devinterface_ptr;

  typedef struct _vlan_info
  {
    uint32_t id;
    uint32_t bridge_id;
    std::list<devinterface_ptr> interfaces;
  } vlan_info;

  typedef boost::shared_ptr<vlan_info> vlan_ptr;

  class session
  {
    friend class session_manager;
    public:
      session(experiment_info& ei) throw(std::runtime_error);
      ~session() throw();   

      experiment_info& getExpInfo() { return expInfo;}
      dev_ptr addDev(component& c, std::string eaddr, uint32_t crs, uint32_t mem);
      bool removeDev(component& c);
      bool removeDev(dev_ptr devp);

      bool configureDev(component& c, node_info& ninfo);
      dev_ptr getDev(component& c);
      dev_ptr getDev(uint32_t cid);
      
      //bool addToVlan(uint32_t vlan, component& c);
      void clear();
      vlan_ptr getVLan(uint32_t vlan);//adds vlan if not already there
      static std::string getDevName(std::string eaddr);
      static std::string getLastAddrByte(std::string eaddr);
      
      bool add_route(uint32_t id, uint16_t port, std::string prefix, uint32_t mask, std::string nexthop);
      bool delete_route(uint32_t id, uint16_t port, std::string prefix, uint32_t mask);
      
    private:
      experiment_info expInfo;
      std::list<dev_ptr> devs;
      std::list<vlan_ptr> vlans;
      int system_cmd(std::string cmd);
      devinterface_ptr getInterface(dev_ptr dp, uint16_t port);
  };

  typedef boost::shared_ptr<session> session_ptr;
};

#endif // _ONLVPNBASED_SESSION_H
