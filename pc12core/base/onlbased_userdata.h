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

#ifndef _ONLBASED_USERDATA_H
#define _ONLBASED_USERDATA_H

namespace onlbased
{
  typedef struct _ts_data
  { 
    uint32_t ts;
    uint32_t rate;
    uint32_t val;
  } ts_data;

  class userdata
  {
    public:
      userdata(std::string file_num, uint32_t field) throw(std::runtime_error);
      ~userdata() throw();
   
      uint32_t read_last_value() throw(std::runtime_error);

      void read_next_values(std::list<ts_data>& vals) throw(std::runtime_error);
      
    private:
      std::string filename;
      uint32_t fieldnum;

      std::ifstream *file;

      bool checkfile();
      uint32_t get_val(std::stringstream& ss);
      double get_ts(std::stringstream& ss);
  };
};

#endif // _ONLBASED_USERDATA_H
