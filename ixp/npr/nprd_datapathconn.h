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

#ifndef _NPRD_DATAPATHCONN_H
#define _NPRD_DATAPATHCONN_H

namespace npr
{
  class datapathconn_exception: public std::runtime_error
  {
    public:
      datapathconn_exception(const std::string& e): std::runtime_error("Datapathconn exception: " + e) {}
  };

  class DataPathConn
  {
    public:
      DataPathConn() throw();
      ~DataPathConn() throw();
  
      bool check() throw();
      npr::DataPathMessage *readmsg() throw(datapathconn_exception);
      void writemsg(npr::DataPathMessage *) throw(datapathconn_exception);

    private:
      pthread_mutex_t read_lock;
      pthread_mutex_t write_lock;

      #define INVALIDSRC     0
      #define LOCAL_DELIVERY 1
      #define EXCEPTIONS     2
      #define ERRORS         3

      bool msg_ready;
      unsigned int source;
      unsigned int first_word;
  };
};

#endif // _NPRD_DATAPATHCONN_H
