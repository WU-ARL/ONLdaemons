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

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <list>
#include <vector>
#include <map>
#include <exception>
#include <stdexcept>

#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <memory.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pwd.h>
#include <boost/shared_ptr.hpp>

#include "shared.h"

#include "onlbased_userdata.h"
#include "onlbased_session_manager.h"
#include "onlbased_globals.h"

using namespace onlbased;

userdata::userdata(std::string file_name, uint32_t field) throw(std::runtime_error)
{
  filename = file_name;
  fieldnum = field;

  if(!checkfile())
  {
    throw std::runtime_error("file " + filename + ": permission denied");
  }

  file = new std::ifstream(filename.c_str());
  if(!file->good())
  {
    throw std::runtime_error("error opening file " + filename);
  }
  read_last_value();//added 6.27.11 by jp to skip to the end of file before it starts reading
}

userdata::~userdata() throw()
{ 
  if(file)
  {
    file->close();
    delete file; 
  }
}

uint32_t
userdata::read_last_value() throw(std::runtime_error)
{
  if(!file)
  {
    throw std::runtime_error("file not open");
  }

  file->seekg(0,std::ios::beg);
  if(!file->good())
  {
    throw std::runtime_error("error reading file");
  }

  char line[2048];
  char tmp_line[2048];
  line[0] = '\0';
  tmp_line[0] = '\0';
  while(!file->eof())
  {
    file->getline(tmp_line, 2048);
    if(strlen(tmp_line) > 0)
    {
      strncpy(line, tmp_line, 2048);
    }
  }
  file->clear();
  if(strlen(line) == 0)
  {
    return 0;
  }
  
  std::stringstream ss(line);
  uint32_t val = get_val(ss);
  write_log("userdata::read_last_value(): file " + filename + ", field " + int2str(fieldnum) + ": value " + int2str(val));
  return val;
}

void
userdata::read_next_values(std::list<ts_data>& vals) throw(std::runtime_error)
{
  if(!file)
  {
    throw std::runtime_error("file not open");
  }

  if(!file->good())
  {
    throw std::runtime_error("error reading file");
  }

  char line[2048];
  line[0] = '\0';
  while(!file->eof())
  {
    file->getline(line, 2048);
    if(strlen(line) == 0) { continue; }

    std::stringstream ss(line);
    ts_data next_val;
    next_val.val = get_val(ss);
    next_val.ts = ((uint32_t)(get_ts(ss) * 1000));
    next_val.rate = 1000;
    vals.push_back(next_val);

    write_log("userdata::read_next_values(): file " + filename + ", field " + int2str(fieldnum) + ": value " + int2str(next_val.val) + ", ts " + int2str(next_val.ts));
  }
  file->clear();
}

bool
userdata::checkfile()
{
  struct stat stbuf;
  if((stat(filename.c_str(), &stbuf)) == -1) //find file
  {
    return false;
  }

  if((stbuf.st_mode & S_IFMT) != S_IFREG) //see if it's a regular file
  {
    return false;
  }

  struct passwd *pwent = getpwnam(user.c_str());
  if(pwent == NULL)
  {
    return false;
  }

  uid_t uid = pwent->pw_uid;
  gid_t gid = pwent->pw_gid;
  if(((uid == stbuf.st_uid) && (stbuf.st_mode & S_IRUSR)) || //if the owner is the same as the experiment owner and file is readable
     ((gid == stbuf.st_gid) && (stbuf.st_mode & S_IRGRP)) || //if group readable by owner's group
     (stbuf.st_mode & S_IROTH)) //if world readable
  {
    return true;
  }

  return false;
}

uint32_t
userdata::get_val(std::stringstream& ss)
{
  uint32_t i=0;
  std::string word;
  while(ss >> word)
  {
    if(i == fieldnum)
    {
      return atoi(word.c_str());
    }
    ++i;
  }
  return 0;
}

double
userdata::get_ts(std::stringstream& ss)
{
  double ts=0;
  std::string word;
  while(ss >> word)
  {
    ts = atof(word.c_str());
  }
  return ts;
}
