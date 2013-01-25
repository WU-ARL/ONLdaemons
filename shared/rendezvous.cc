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

#include <string>
#include <iostream>
#include <sstream>
#include <exception>
#include <stdexcept>

#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <memory.h>
#include <pthread.h>
#include <sys/types.h>
#include <netinet/in.h>

#include "util.h"
#include "byte_buffer.h"
#include "rendezvous.h"

using namespace onld;

rendezvous::rendezvous()
{
  id = 0;
  seq = 0;
  unused1 = 0;
  unused2 = 0;
  unused3 = 0;
  unused4 = 0;
  unused5 = 0;
  unused6 = 0;
  unused7 = 0;
}

rendezvous::rendezvous(uint32_t id_num, uint32_t seq_num)
{
  id = id_num;
  seq = seq_num;
  unused1 = 0;
  unused2 = 0;
  unused3 = 0;
  unused4 = 0;
  unused5 = 0;
  unused6 = 0;
  unused7 = 0;
}

rendezvous::~rendezvous()
{
}

rendezvous::rendezvous(const rendezvous& r)
{
  id = r.id;
  seq = r.seq;
  unused1 = r.unused1;
  unused2 = r.unused2;
  unused3 = r.unused3;
  unused4 = r.unused4;
  unused5 = r.unused5;
  unused6 = r.unused6;
  unused7 = r.unused7;
}

rendezvous &
rendezvous::operator=(const rendezvous& r)
{
  id = r.id;
  seq = r.seq;
  unused1 = r.unused1;
  unused2 = r.unused2;
  unused3 = r.unused3;
  unused4 = r.unused4;
  unused5 = r.unused5;
  unused6 = r.unused6;
  unused7 = r.unused7;
  return *this;
}

bool
rendezvous::operator==(const rendezvous& r)
{
  if((id == r.id) && (seq == r.seq)) { return true; }
  return false;
}

bool
rendezvous::operator<(const rendezvous& r) const
{
  if(id < r.id) { return true; }
  if(id > r.id) { return false; }
  if(seq < r.seq) { return true; }
  return false;
}

byte_buffer&
onld::operator<<(byte_buffer& buf, rendezvous& rend)
{
  buf << rend.id;
  buf << rend.unused1;
  buf << rend.unused2;
  buf << rend.seq;
  buf << rend.unused3;
  buf << rend.unused4;
  buf << rend.unused5;
  buf << rend.unused6;
  buf << rend.unused7;
  return buf;
}

byte_buffer&
onld::operator>>(byte_buffer& buf, rendezvous& rend)
{
  buf >> rend.id;
  buf >> rend.unused1;
  buf >> rend.unused2;
  buf >> rend.seq;
  buf >> rend.unused3;
  buf >> rend.unused4;
  buf >> rend.unused5;
  buf >> rend.unused6;
  buf >> rend.unused7;
  return buf;
}
