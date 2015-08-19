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

#ifndef _ONLCRD_SESSION_H
#define _ONLCRD_SESSION_H

namespace onlcrd
{
  class begin_session_req;
  class session_add_component_req;
  class session_add_cluster_req;
  class session_add_link_req;
  class observe_session_req;
  class grant_observe_req;
  class crd_component;
  class crd_link;

  static const std::string session_file_base = "/users/onl/experiments";
  static const int reservation_alert_time = 15; //minutes
  static const int rli_ping_period = 5; //minutes

  typedef struct _crd_vlan
  {
    switch_vlan vlanid;
    bool is_deleted;
  } crd_vlan;

  typedef boost::shared_ptr<crd_vlan> vlan_ptr;

  class session
  {
    public:
      session(uint32_t sid, std::string username, nccp_connection* nc);
      ~session();

      std::string getID() { return id; }
      std::string getUser() { return user; }
      nccp_connection* getConnection() { return nccpconn; }
      std::string getErrorMsg() { return errmsg; }
      void setErrorMsg(std::string err) { errmsg = err; }

      void set_begin_request(begin_session_req *req);
      void add_component(session_add_component_req *req);
      void add_cluster(session_add_cluster_req *req);
      void add_link(session_add_link_req *req);
      bool commit(boost::shared_ptr<session> sess);
      void set_vlans(boost::shared_ptr<crd_component> comp, vlan_ptr vlan);

      void clear();
      bool is_cleared() { return cleared; }
      void expired();

      void write_mapping();
      void clear_mapping();

      void send_alerts();

      void add_observer(std::string username); 
      bool observable_by(std::string username); 
      bool observe(observe_session_req* req);
      void grant_observe(grant_observe_req* req);
      void link_vlanports_added(uint32_t lid);//called by link request when vlan ports have been added

    private: 
      std::string id;
      std::string user;
      nccp_connection* nccpconn;
      std::string errmsg;

      bool cleared;
      bool mapping_written;

      std::string user_session_file;

      pthread_mutex_t req_lock;
      begin_session_req* begin_req;
      std::list<session_add_component_req *> component_reqs;
      std::list<session_add_cluster_req *> cluster_reqs;
      std::list<session_add_link_req *> link_reqs;

      onl::topology topology;
      std::list< boost::shared_ptr<crd_component> > components;
      std::list< boost::shared_ptr<crd_link> > links;
      std::list<uint32_t> links_unreported; //for setting vlans
      bool vlans_created;//marks when nmd create session req is sent so no duplicate
      std::list<uint32_t> unreported_links;//links that haven't sent vlan requests

      pthread_mutex_t share_lock;
      std::list<std::string> allowed_observers;
      std::list<observe_session_req *> observe_reqs;

      void set_clusters();//sets dependent name for those components that are part of an ixppair
  }; // class session

  void* session_send_alerts_wrapper(void* obj);

  typedef boost::shared_ptr<session> session_ptr;
};

#endif // _ONLCRD_SESSION_H
