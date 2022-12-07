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

#ifndef _ONLVPNBASED_SESSION_MANAGER_H
#define _ONLVPNBASED_SESSION_MANAGER_H

namespace onlvpnbased
{
  typedef struct _devname_info
  {
    std::string name;
    int index;
    bool in_use;
  } devname_info;

  typedef boost::shared_ptr<devname_info> devname_ptr;

  class session_manager
  {
  public:
      session_manager() throw();
      ~session_manager();
      session_ptr getSession(experiment_info& einfo);
      bool startDev(session_ptr sptr, dev_ptr devp);
      bool assignDev(dev_ptr devp);//assigns control addr and dev name for dev
      bool deleteDev(component& c, experiment_info& einfo);
      bool releaseDevName(std::string nm);
      session_ptr addSession(experiment_info& einfo);
  private:
      std::list<session_ptr> active_sessions;
      std::map<std::string, devname_info> devnames; //list of available dev names marked true if in use
      pthread_mutex_t devname_lock;
      pthread_mutex_t session_lock;
      int getDevIndex(std::string devnm);
  };
};

#endif //ONLVPNBASED_SESSION_MANAGER_H 
