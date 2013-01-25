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

#ifndef _ONLCRD_RESERVATION_H
#define _ONLCRD_RESERVATION_H

namespace onlcrd
{
  class begin_reservation_req;
  class reservation_add_component_req;
  class reservation_add_cluster_req;
  class reservation_add_link_req;

  class reservation
  {
    public:
      reservation(uint32_t rid, std::string username, nccp_connection* nc);
      ~reservation();

      std::string getID() { return id; }
      nccp_connection* getConnection() { return nccpconn; }
      void setErrorMsg(std::string err) { errmsg = err; }

      void set_begin_request(begin_reservation_req* req);
      void add_component(reservation_add_component_req *req);
      void add_cluster(reservation_add_cluster_req *req);
      void add_link(reservation_add_link_req *req);
      void commit();
      void clear();

    private: 
      std::string id;
      std::string user;
      nccp_connection* nccpconn;
      std::string errmsg;

      bool cleared;

      pthread_mutex_t req_lock;
      begin_reservation_req* begin_req; // must delete this when done
      std::list<reservation_add_component_req *> component_reqs;
      std::list<reservation_add_cluster_req *> cluster_reqs;
      std::list<reservation_add_link_req *> link_reqs;

      onl::topology topology;
  }; // class reservation
};

#endif // _ONLCRD_RESERVATION_H
