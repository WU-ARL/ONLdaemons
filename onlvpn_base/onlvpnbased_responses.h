/*
 * Copyright (c) 2022 Jyoti Parwatikar, John DeHart 
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

#ifndef _ONLVPNBASED_RESPONSES_H
#define _ONLVPNBASED_RESPONSES_H

namespace onlvpnbased
{
  class rli_relay_resp : public onld::rli_response
  {
    public:
      rli_relay_resp(rli_response* r, rli_relay_req* rq);
      virtual ~rli_relay_resp();

      virtual void write();

    protected:
      rli_response* orig_resp;
  }; // class rli_relay_req

  class crd_relay_resp : public onld::crd_response
  {
    public:
      crd_relay_resp(crd_response* r, crd_relay_req* rq);
      virtual ~crd_relay_resp();

      virtual void write();

    protected:
      crd_response* orig_resp;
  }; // class crd_relay_req

};

#endif // _ONLVPNBASED_RESPONSES_H
