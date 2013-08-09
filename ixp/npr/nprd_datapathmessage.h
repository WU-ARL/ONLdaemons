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

#ifndef _NPRD_DATAPATHMESSAGE_H
#define _NPRD_DATAPATHMESSAGE_H

namespace npr
{
  class datapathmessage_exception: public std::runtime_error
  {
    public:
      datapathmessage_exception(const std::string& e): std::runtime_error("Datapathmessage exception: " + e) {}
  };

  class DataPathMessage
  {
    public:
      // these are the type of message
      #define DP_TTL_EXPIRED    0
      #define DP_IP_OPTIONS     1
      #define DP_NO_ROUTE       2
      #define DP_NH_INVALID     3
      #define DP_ARP_NEEDED_DIP 4
      #define DP_ARP_NEEDED_NHR 5
      #define DP_ARP_PACKET     6
      #define DP_LD_PACKET      7
      #define DP_GENERAL_ERROR  8
      #define DP_PASS_THROUGH   9
      #define DP_RECLASSIFY     10
      #define DP_DEST_PLUGIN_0  11
      #define DP_DEST_PLUGIN_1  12
      #define DP_DEST_PLUGIN_2  13
      #define DP_DEST_PLUGIN_3  14
      #define DP_DEST_PLUGIN_4  15
      #define DP_DROP           16
      #define DP_MAX_TYPE       16

      // these are associations of this message
      #define MSG_FROM_PLC    0
      #define MSG_TO_MUX      1
      #define MSG_TO_QM       2
      #define MSG_TO_PLUGIN_0 3
      #define MSG_TO_PLUGIN_1 4
      #define MSG_TO_PLUGIN_2 5
      #define MSG_TO_PLUGIN_3 6
      #define MSG_TO_PLUGIN_4 7
      #define MSG_TO_FLM      8
      #define MAX_MSG_ASSOC   8

      DataPathMessage(unsigned int *, unsigned int) throw(datapathmessage_exception);
      ~DataPathMessage() throw();

      unsigned int type() throw();
      unsigned int getnumwords() throw();
      unsigned int getmsgassociation() throw();
      unsigned int getword(unsigned int) throw(datapathmessage_exception);
      unsigned int *getmsg() throw();

    private:
      unsigned int *msg;
      unsigned int association;
  };
};

#endif // _NPRD_DATAPATHMESSAGE_H
