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

#ifndef _TCAMD_LOGFILE_H
#define _TCAMD_LOGFILE_H

namespace tcamd
{
  class log_exception: std::exception
  {
    private:
      std::string info;

    public:
      log_exception::log_exception(std::string s) throw()
      {
        info = s;
      }
      log_exception::~log_exception() throw()
      {
      }
      virtual const char* what() const throw()
      {
        return ("Exception with the log api: " + info).c_str();
      }
  };

  class LogFile
  {
    #define DEFAULT_TCAMD_LOG "tcamd.log"

    #define NO_LOG_ROTATE 0
    #define LOG_ROTATE 1

    #define LOG_NORMAL  0
    #define LOG_VERBOSE 1

    public:
      LogFile(std::string, unsigned short, std::string) throw(log_exception);
      ~LogFile() throw();
      void write(unsigned int, std::string) throw(log_exception);
      std::string int2str(unsigned int);

    private:
      int fd;
      unsigned int debuglevel;
  };

  extern LogFile *tcamlog;
};

#endif // _TCAMD_LOGFILE_H
