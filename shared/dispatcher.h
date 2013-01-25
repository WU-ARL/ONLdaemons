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

#ifndef _DISPATCHER_H
#define _DISPATCHER_H

namespace onld
{
  // the dispatcher servers two primary purposes:
  // 1 - map message classes to message types so that new instances of the correct
  //     type can be built when a packet comes in, i.e., when an NCCP message arrives,
  //     this code will peek at the header to get the operation type and then use that
  //     to instantiate a new instance of a message derivate class for that type
  // 2 - match responses to requests via the rendezvous marker in the NCCP header.
  //     this class maintains the necessary mappings and will call a special function
  //     in the requester object to handle the response
  // there should only ever be one instance of this class.
  class dispatcher
  {
    public:
      // this is a singleton class, so hide the constructor behind this call.
      // NOTE! This is NOT thread safe, it MUST be called from a single thread first.
      static dispatcher * get_dispatcher();

      // create a message instance of the appropriate type
      message *create_message(uint8_t *mbuf, uint32_t size);

      // register a rendezvous for a request, which fills in the req's rendezvous marker and
      // ensures that req->request_rendezvous(response) will be called when a response shows up
      void register_rendezvous(request *req);

      // register a periodic rendezvous for a response, which fills in the req's return rendezvous marker
      // and ensures that req->request_rendezvous(periodic) will be called when a periodic message shows up
      void register_periodic_rendezvous(request *req);

      // find and call the rendezvous method for this resp
      bool call_rendezvous(response *resp);

      // find and call the rendezvous method for this periodic message
      bool call_rendezvous(periodic *per);

      // remove a response rendezvous for a request, which resets the req's rendezvous marker
      void clear_rendezvous(request *req);

      // remove a periodic rendezvous for a request, which resets the req's return rendezvous marker
      void clear_periodic_rendezvous(request *req);

    private:
      dispatcher();
      ~dispatcher();
      dispatcher(const dispatcher&);
      dispatcher& operator=(const dispatcher&);
      static dispatcher* the_dispatcher;

      pthread_mutex_t seq_lock;
      pthread_mutex_t map_lock; //JP added 9_4_2012 to try and fix seg fault
      std::map<rendezvous, request *> rend_map;
      uint32_t next_seq[256];

      // this is all a bit weird, so i'll try to lay out how this works.

      // this typdef is not like a normal typedef.  instead, it declares the identifier
      // 'contructor_template' to be a function pointer type that returns a message pointer
      // and takes a uint8_t pointer and uint32_t as arguments.
      // it is used only twice, iin the next two lines to declare arrays of such function
      // pointers.
    public:
      typedef message * (*constructor_template)(uint8_t *mbuf, uint32_t size);
      static constructor_template req_map[256];
      static constructor_template resp_map[256];

  }; //class dispatcher

      // this is a template function that matches the constructor_template signature.  when
      // a new mapping is registered,  a pointer to this function with the correct class
      // type is added to the one of the map arrays.  so, the arrays store a pointer to a concrete
      // instance of this indirect_constructor for the correct message class.  this in turn
      // allows us to call this function directly form the map arrays to generate an instance 
      // of the desired message class and return a pointer that new object.
      template<class T>
      message *indirect_constructor(uint8_t *mbuf, uint32_t size)
      {
        return new T(mbuf, size);
      }

      // register a new mapping from operation 'op' to request class T
      template<class T>
      bool register_req(uint8_t op)
      {
        if(dispatcher::req_map[op] != NULL) { return false; }
        dispatcher::req_map[op] = indirect_constructor<T>;
        return true;
      }

      // register a new mapping from operation 'op' to response class T
      template<class T>
      bool register_resp(uint8_t op)
      {
        if(dispatcher::resp_map[op] != NULL) { return false; }
        dispatcher::resp_map[op] = indirect_constructor<T>;
        return true;
      }
};

#endif // _DISPATCHER_H
