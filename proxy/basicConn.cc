/*
 * Copyright (c) 2009-2013 Fred Kuhns, Charlie Wiseman, Jyoti Parwatikar, John DeHart 
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

/*
 * $Author: fredk $
 * $Date: 2006/12/08 20:34:59 $
 * $Revision: 1.4 $
 * $Name:  $
 *
 * File:         session.cc.c
 * Created:      12/29/2004 10:56:15 AM CST
 * Author:       Fred Kunhns, <fredk@cse.wustl.edu>
 * Organization: Applied Research Laboratory,
 *               Washington University in St. Louis
 *
 * Description:
 *
 */
#include <iostream>
#include <sstream>
#include <vector>

#include <iostream>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <stdint.h>

#include <sys/select.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/uio.h>

#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>
#include <getopt.h>

#include <pthread.h>  /* pthread */
#include <wulib/wulog.h>


#include <wuPP/net.h>
#include <wuPP/addr.h>
#include <wuPP/sock.h>
#include <wuPP/buf.h>
#include <wuPP/msg.h>

#include "control.h"
#include "cmd.h"
#include "nccp.h"
#include "basicConn.h"

wucmd::flags_t wunet::BasicConn::conn2Flags(wunet::BasicConn *conn)
{
  wucmd::flags_t st = 0;

  if (conn == NULL)
    return 0;

  if (wupp::isError(conn->etype()))
    st = wucmd::connError;
  if (conn->isOpen())
    st |= wucmd::connOpen;
  if (conn->isRxPending() || conn->isTxPending())
    st |= wucmd::connPending;

  return st;
}
