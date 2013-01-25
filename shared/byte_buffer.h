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

#ifndef _BYTE_BUFFER_H
#define _BYTE_BUFFER_H

namespace onld
{
  class byte_buffer
  {
    public:
      byte_buffer();
      byte_buffer(const byte_buffer & other);
      ~byte_buffer();

      byte_buffer & operator=(const byte_buffer & other);

      // Routines to add information to the buffer
      byte_buffer & operator<<(uint8_t data1);
      byte_buffer & operator<<(uint16_t data2);
      byte_buffer & operator<<(uint32_t data4);
      byte_buffer & operator<<(bool b);
      byte_buffer & operator<<(char c);
      byte_buffer & operator<<(struct timeval & tv);

      // Routines to remove information from the buffer
      byte_buffer & operator>>(uint8_t & data1);
      byte_buffer & operator>>(uint16_t & data2);
      byte_buffer & operator>>(uint32_t & data4);
      byte_buffer & operator>>(bool & b);
      byte_buffer & operator>>(char & c);
      byte_buffer & operator>>(struct timeval & tv);

      uint32_t getMaxSize() { return num_bytes_allocated; }
      uint32_t getSize() { return num_bytes_stored; }
      uint32_t getCurrentIndex() { return cur_index; }
      uint8_t* getData() { return buffer_data; }

      void reset() { cur_index = 0; }

      void resize(uint32_t newLength);  // change the size of the buffer
      void dump(); // debug output of data in buffer

    private:
      uint32_t num_bytes_allocated;  // number of bytes available
      uint32_t num_bytes_stored; // number of bytes used
      uint32_t cur_index;     // index into array, used in read/write
      uint8_t *buffer_data;    // num_bytes_allocated bytes of data
  }; // class byte_buffer
}; // namespace onld

#endif // _BYTE_BUFFER_H
