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
 * $Date: 2006/03/20 22:23:45 $
 * $Revision: 1.4 $
 * $Name:  $
 *
 * File:         cmd.h
 * Created:      01/19/2005 02:02:46 PM CST
 * Author:       Fred Kunhns, <fredk@cse.wustl.edu>
 * Organization: Applied Research Laboratory,
 *               Washington University in St. Louis
 *
 * Description:
 *
 */

#ifndef _WU_NCCP_H
#define _WU_NCCP_H

// #define WU_NCCP_DEBUG

  struct nccp_t {
#define wunccp_hlen sizeof(uint32_t)
    uint32_t len_;

    static size_t hdrlen() {return (size_t)wunccp_hlen;}
    size_t hlen() const {return nccp_t::hdrlen();}
    size_t dlen() const {return ntohl(len_);}
    void   dlen(uint32_t len) {len_ = htonl(len);}
    size_t mlen() const {return hlen() + dlen();}
    void   reset() {len_ = 0;}

  };

#endif
