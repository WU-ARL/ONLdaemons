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
#include <stdio.h>
#include <stdint.h>
#include <memory.h>
#include <pthread.h>
#include <sys/types.h>
#include <netinet/in.h>

#include "util.h"
#include "byte_buffer.h"

using namespace onld;

byte_buffer::byte_buffer()
{
  cur_index = 0;
  num_bytes_stored = 0;

  num_bytes_allocated = 2048;
  buffer_data = new uint8_t[num_bytes_allocated];
}

byte_buffer::byte_buffer(const byte_buffer & other)
{
  *this = other;
}

byte_buffer::~byte_buffer() 
{
  if(buffer_data) 
  {
    delete[] buffer_data;
  }
}

byte_buffer &
byte_buffer::operator=(const byte_buffer & other)
{
  if(buffer_data) { delete[] buffer_data; }

  num_bytes_stored = other.num_bytes_stored;
  cur_index = other.cur_index;
  num_bytes_allocated = other.num_bytes_allocated; 

  buffer_data = new uint8_t[num_bytes_allocated];
  memcpy(buffer_data, other.buffer_data, num_bytes_stored);

  return *this;
}

byte_buffer &
byte_buffer::operator<<(uint8_t data1)
{
  memcpy(&buffer_data[cur_index], &data1, 1);
  cur_index += 1;
  num_bytes_stored = cur_index;
  return *this;
}

byte_buffer &
byte_buffer::operator<<(uint16_t data2)
{
  data2 = htons(data2);
  memcpy(&buffer_data[cur_index], &data2, 2);
  cur_index += 2;
  num_bytes_stored = cur_index;
  return *this;
}

byte_buffer &
byte_buffer::operator<<(uint32_t data4)
{
  data4 = htonl(data4);
  memcpy(&buffer_data[cur_index], &data4, 4);
  cur_index += 4;
  num_bytes_stored = cur_index;
  return *this;
}

byte_buffer &
byte_buffer::operator<<(bool b)
{
  if(b)
  {
    *this << (uint16_t)1;
  }
  else
  {
    *this << (uint16_t)0;
  }
  return *this;
}

byte_buffer &
byte_buffer::operator<<(char c)
{
  *this << (uint8_t)c;
  return *this;
}

byte_buffer &
byte_buffer::operator<<(struct timeval & tv)
{
  *this << (uint32_t)tv.tv_sec;
  *this << (uint32_t)tv.tv_usec;
  return *this;
}

byte_buffer &
byte_buffer::operator>>(uint8_t & data1)
{
  memcpy(&data1, &buffer_data[cur_index], 1);
  cur_index += 1;
  return *this;
}

byte_buffer &
byte_buffer::operator>>(uint16_t & data2)
{
  memcpy(&data2, &buffer_data[cur_index], 2);
  cur_index += 2;
  data2 = ntohs(data2);
  return *this;
}

byte_buffer & 
byte_buffer::operator>>(uint32_t & data4)
{
  memcpy(&data4, &buffer_data[cur_index], 4);
  cur_index += 4;
  data4 = ntohl(data4);
  return *this;
}

byte_buffer &
byte_buffer::operator>>(bool & b)
{
  uint16_t data2;
  *this >> data2;
  if(data2 != 0) { b = true; }
  else { b = false; }
  return *this;
}

byte_buffer &
byte_buffer::operator>>(char & c)
{
  uint8_t data1;
  *this >> data1; c = (char)data1;
  return *this;
}

byte_buffer &
byte_buffer::operator>>(struct timeval & tv)
{
  uint32_t data4;
  *this >> data4; tv.tv_sec = data4;
  *this >> data4; tv.tv_usec = data4;
  return *this;
}

// Resize the buffer, so it can hold newLength bytes.  Clears the buffer at
// the same time.
void 
byte_buffer::resize(uint32_t newLength) 
{
  if(num_bytes_allocated < newLength) 
  {
    if(buffer_data) { delete[] buffer_data; }
    num_bytes_allocated = newLength;
    buffer_data = new uint8_t[num_bytes_allocated?num_bytes_allocated:1];
  }

  num_bytes_stored = 0;
  cur_index = 0;
}

void
byte_buffer::dump()
{
  char tmpstr[256];
  for(uint32_t i=0; i<num_bytes_stored; i++)
  {
    if((i % 4) == 0)
    {
      sprintf(tmpstr, "%d: 0x%x", i, (uint32_t)buffer_data[i]);
      write_log(tmpstr);
    }
  }
}
