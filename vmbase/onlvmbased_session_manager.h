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

#ifndef _ONLVMBASED_SESSION_MANAGER_H
#define _ONLVMBASED_SESSION_MANAGER_H

namespace onlvmbased
{
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
  private:
      std::list<session_ptr> active_sessions;
      std::map<std::string, bool> vmnames; //list of available vm names marked true if in use
  };
};

#endif //ONLVMBASED_SESSION_MANAGER_H 
