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

#ifndef _RENDEZVOUS_H
#define _RENDEZVOUS_H

namespace onld
{
  class rendezvous
  {
    public:
      rendezvous();
      rendezvous(uint32_t id_num, uint32_t seq_num);
      ~rendezvous();
      rendezvous(const rendezvous& r);
      rendezvous& operator=(const rendezvous& r);
      bool operator==(const rendezvous& r);
      bool operator<(const rendezvous& r) const;

      friend byte_buffer& operator<<(byte_buffer& buf, rendezvous& rend);
      friend byte_buffer& operator>>(byte_buffer& buf, rendezvous& rend);

    private:
/* old format for use with REND-based scheduler
        uint16_t encapsulator_index; 
        uint16_t requester_pid;
        uint32_t encapsulator_seqnum;
        uint32_t req_HostID;
        struct timeval encap_timestamp; // sec, usec stored as uint32_t (each)
        void *encapsulator_address;     // stored as uint32_t
        void *requester_address;        // stored as uint32_t
*/
      // can be used however we want, but must total to 28 bytes
      uint8_t  id;      // top-level identifier, mapped to operation type 
      uint16_t unused1; // unused for now
      uint8_t  unused2; // unused for now
      uint32_t seq;     // sequence number within top-level id
      uint32_t unused3; // unused for now
      uint32_t unused4; // unused for now
      uint32_t unused5; // unused for now
      uint32_t unused6; // unused for now
      uint32_t unused7; // unused for now
  };

  byte_buffer& operator<<(byte_buffer& buf, rendezvous& rend);
  byte_buffer& operator>>(byte_buffer& buf, rendezvous& rend);
};

#endif // _RENDEZVOUS_H
