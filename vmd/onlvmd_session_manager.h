/*
 * Copyright (c) 2014 Jyoti Parwatikar 
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

#ifndef _ONLVMD_SESSION_MANAGER_H
#define _ONLVMD_SESSION_MANAGER_H

namespace onlvmd
{
  typedef struct _vmname_info
  {
    std::string name;
    int index;
    bool in_use;
  } vmname_info;

  typedef boost::shared_ptr<vmname_info> vmname_ptr;

  class session_manager
  {
  public:
      session_manager() throw();
      ~session_manager();
      session_ptr getSession(experiment_info& einfo);
      bool startVM(session_ptr sptr, vm_ptr vmp);
      bool assignVM(vm_ptr vmp);//assigns control addr and vm name for vm
      bool deleteVM(component& c, experiment_info& einfo);
      bool releaseVMname(std::string nm);
      session_ptr addSession(experiment_info& einfo);
      bool isDone()  { return done;}
  private:
      std::list<session_ptr> active_sessions;
      std::map<std::string, vmname_info> vmnames; //list of available vm names marked true if in use
      pthread_mutex_t vmname_lock;
      pthread_mutex_t session_lock;
      int getVMIndex(std::string vmnm);
      bool done;
  };
};

#endif //ONLVMD_SESSION_MANAGER_H 
