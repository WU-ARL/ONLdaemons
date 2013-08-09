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

#ifndef _NPRD_RESPONSES_H
#define _NPRD_RESPONSES_H

namespace npr
{
  class get_plugins_resp : public response
  {
    public:
      get_plugins_resp(get_plugins_req* req, NCCP_StatusType stat);
      get_plugins_resp(get_plugins_req* req, std::vector<std::string>& label_list, std::vector<std::string>& path_list);
      virtual ~get_plugins_resp();

      virtual void write();

    protected:
      NCCP_StatusType status;
      uint32_t num_plugins;
      std::vector<std::string> labels;
      std::vector<std::string> paths;
  }; // class get_plugins_resp
};

#endif // _NPRD_RESPONSES_H
