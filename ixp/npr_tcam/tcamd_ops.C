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
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <exception>

#include <unistd.h> 
#include <stdlib.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>

#include <npr/nprd_types.h>

#include "tcamd_logfile.h"
#include "tcamd_util.h"
#include "tcamd_cmds.h"
#include "tcamd_ops.h"

#include <tcamLib/tcamlib.h>
#include <wulib/wulog.h>
#include <wuPP/token.h>
#include <wuPP/scanner.h>
#include <wuPP/conf.h>
#include <wuPP/error.h>

#include <tcamLib/Database.h>
#include <tcamLib/SearchMachine.h>

namespace tcamd
{
  SearchMachine *sm;

  DBParams *npra_route_params;
  DBParams *npra_pfilter_params;
  DBParams *npra_afilter_params;
  DBParams *npra_arp_params;

  DBParams *nprb_route_params;
  DBParams *nprb_pfilter_params;
  DBParams *nprb_afilter_params;
  DBParams *nprb_arp_params;

  Database *npra_route;
  Database *npra_pfilter;
  Database *npra_afilter;
  Database *npra_arp;

  Database *nprb_route;
  Database *nprb_pfilter;
  Database *nprb_afilter;
  Database *nprb_arp;

  unsigned int npra_tcam_state;
  unsigned int nprb_tcam_state;

  int inittcam(std::string cf)
  {
    npra_route_params = npra_pfilter_params = npra_afilter_params = npra_arp_params = NULL;
    nprb_route_params = nprb_pfilter_params = nprb_afilter_params = nprb_arp_params = NULL;
    npra_route = npra_pfilter = npra_afilter = npra_arp = NULL;
    nprb_route = nprb_pfilter = nprb_afilter = nprb_arp = NULL;

    wuconf::confTable ctable;
    wuconf::valTable gparams;

    std::vector<DBParams *> dbps;
    std::vector<DBParams *>::iterator iter;

    try {
      // get the database conf from the config file
      wuconf::mkconf(ctable, cf, "global", gparams);
    } catch(...) {
      write_log(LOG_NORMAL, "inittcam: error with mkconf!");
      return -1;
    }

    try {
      // initialize the search machine and TCAM library
      sm = new SearchMachine();
      sm->init(ctable);
    } catch(...) {
      write_log(LOG_NORMAL, "inittcam: error with search machine init!");
      return -1;
    }

    try {
      // parse the config file to get the database parameters
      dbps = sm->parse_conf_table(ctable);
    } catch(...) {
      write_log(LOG_NORMAL, "inittcam: error with search machine parse_conf_table!");
      return -1;
    }

    // get the dbparams objects out
    for(iter = dbps.begin(); iter != dbps.end(); ++iter)
    {
      if((*iter)->name == "npra_route")        { npra_route_params = *iter; }
      else if((*iter)->name == "npra_pfilter") { npra_pfilter_params = *iter; }
      else if((*iter)->name == "npra_afilter") { npra_afilter_params = *iter; }
      else if((*iter)->name == "npra_arp")     { npra_arp_params = *iter; }
      else if((*iter)->name == "nprb_route")   { nprb_route_params = *iter; }
      else if((*iter)->name == "nprb_pfilter") { nprb_pfilter_params = *iter; }
      else if((*iter)->name == "nprb_afilter") { nprb_afilter_params = *iter; }
      else if((*iter)->name == "nprb_arp")     { nprb_arp_params = *iter; }
    }
 
    // go ahead and clear out the vector
    for(iter = dbps.begin(); iter != dbps.end(); ++iter)
    {
      dbps.erase(iter);
    }

    // check that each dbparams is set
    if(npra_route_params == NULL)   { write_log(LOG_NORMAL, "inittcam: npra_route_params is still NULL!"); return -1; }
    if(npra_pfilter_params == NULL) { write_log(LOG_NORMAL, "inittcam: npra_pfilter_params is still NULL!"); return -1; }
    if(npra_afilter_params == NULL) { write_log(LOG_NORMAL, "inittcam: npra_afilter_params is still NULL!"); return -1; }
    if(npra_arp_params == NULL)     { write_log(LOG_NORMAL, "inittcam: npra_arp_params is still NULL!"); return -1; }
    if(nprb_route_params == NULL)   { write_log(LOG_NORMAL, "inittcam: nprb_route_params is still NULL!"); return -1; }
    if(nprb_pfilter_params == NULL) { write_log(LOG_NORMAL, "inittcam: nprb_pfilter_params is still NULL!"); return -1; }
    if(nprb_afilter_params == NULL) { write_log(LOG_NORMAL, "inittcam: nprb_afilter_params is still NULL!"); return -1; }
    if(nprb_arp_params == NULL)     { write_log(LOG_NORMAL, "inittcam: nprb_arp_params is still NULL!"); return -1; }

    npra_tcam_state = STOP;
    nprb_tcam_state = STOP;

    return 0;
  }

  int routerinit(int npu, char *rb)
  {
    int error = 0;
    unsigned int *response;

    if(rb != NULL) { response = (unsigned int *)&rb[0]; }

    if(npu == NPRA)
    {
      write_log(LOG_VERBOSE, "routerinit(NPRA): initializing for NPRA");
      if(npra_tcam_state == INIT)
      {
        write_log(LOG_NORMAL, "routerinit(NPRA): called for NPRA, but NPRA is already INITed");
        if(rb != NULL) { *response = SUCCESS; }
        return sizeof(tcam_response);
      }

      write_log(LOG_VERBOSE, "routerinit(NPRA): about to call create_db");

      try {
        npra_route = sm->create_db(npra_route_params);
        npra_pfilter = sm->create_db(npra_pfilter_params);
        npra_afilter = sm->create_db(npra_afilter_params);
        npra_arp = sm->create_db(npra_arp_params);
      } catch(...) {
        write_log(LOG_NORMAL, "routerinit(NPRA): error creating databases for NPRA!");
        error = 1;
      }

      write_log(LOG_VERBOSE, "routerinit(NPRA): after calls to create_db");

      if(error == 0)
      {
        if(npra_route == NULL)
        {
          write_log(LOG_NORMAL, "routerinit(NPRA): after create_db, npra_route is still NULL!");
          error = 1;
        }
        if(npra_pfilter == NULL)
        {
          write_log(LOG_NORMAL, "routerinit(NPRA): after create_db, npra_pfilter is still NULL!");
          error = 1;
        }
        if(npra_afilter == NULL)
        {
          write_log(LOG_NORMAL, "routerinit(NPRA): after create_db, npra_afilter is still NULL!");
          error = 1;
        }
        if(npra_arp == NULL)
        {
          write_log(LOG_NORMAL, "routerinit(NPRA): after create_db, npra_arp is still NULL!");
          error = 1;
        }
      }

      if(error == 0)
      {
        write_log(LOG_NORMAL, "routerinit(NPRA): initialization was successful");
        npra_tcam_state = INIT;
        if(rb != NULL) { *response = SUCCESS; }
        return sizeof(tcam_response);
      }

      if(npra_route) { sm->destroy_db(npra_route->id()); npra_route = NULL; }
      if(npra_pfilter) { sm->destroy_db(npra_pfilter->id()); npra_pfilter = NULL; }
      if(npra_afilter) { sm->destroy_db(npra_afilter->id()); npra_afilter = NULL; }
      if(npra_arp) { sm->destroy_db(npra_arp->id()); npra_arp = NULL; }
      if(rb != NULL) { *response = FAILURE; }
      return sizeof(tcam_response);
    }
    else if(npu == NPRB)
    {
      write_log(LOG_VERBOSE, "routerinit(NPRB): initializing for NPRB");
      if(nprb_tcam_state == INIT)
      {
        write_log(LOG_NORMAL, "routerinit(NPRB): called for NPRB, but NPRB is already INITed");
        if(rb != NULL) { *response = SUCCESS; }
        return sizeof(tcam_response);
      }

      write_log(LOG_VERBOSE, "routerinit(NPRB): about to call create_db");

      try {
        nprb_route = sm->create_db(nprb_route_params);
        nprb_pfilter = sm->create_db(nprb_pfilter_params);
        nprb_afilter = sm->create_db(nprb_afilter_params);
        nprb_arp = sm->create_db(nprb_arp_params);
      } catch(...) {
        write_log(LOG_NORMAL, "routerinit(NPRB): error creating databases for NPRB!");
        error = 1;
      }

      write_log(LOG_VERBOSE, "routerinit(NPRB): after calls to create_db");

      if(error == 0)
      {
        if(nprb_route == NULL)
        {
          write_log(LOG_NORMAL, "routerinit(NPRB): after create_db, nprb_route is still NULL!");
          error = 1;
        }
        if(nprb_pfilter == NULL)
        {
          write_log(LOG_NORMAL, "routerinit(NPRB): after create_db, nprb_pfilter is still NULL!");
          error = 1;
        }
        if(nprb_afilter == NULL)
        {
          write_log(LOG_NORMAL, "routerinit(NPRB): after create_db, nprb_afilter is still NULL!");
          error = 1;
        }
        if(nprb_arp == NULL)
        {
          write_log(LOG_NORMAL, "routerinit(NPRB): after create_db, nprb_arp is still NULL!");
          error = 1;
        }
      }

      if(error == 0)
      {
        write_log(LOG_NORMAL, "routerinit(NPRB): initialization was successful");
        nprb_tcam_state = INIT;
        if(rb != NULL) { *response = SUCCESS; }
        return sizeof(tcam_response);
      }

      if(nprb_route) { sm->destroy_db(nprb_route->id()); nprb_route = NULL; }
      if(nprb_pfilter) { sm->destroy_db(nprb_pfilter->id()); nprb_pfilter = NULL; }
      if(nprb_afilter) { sm->destroy_db(nprb_afilter->id()); nprb_afilter = NULL; }
      if(nprb_arp) { sm->destroy_db(nprb_arp->id()); nprb_arp = NULL; }
      if(rb != NULL) { *response = FAILURE; }
      return sizeof(tcam_response);
    } 
    else
    {
      write_log(LOG_NORMAL, "routerinit: something is broken, npu is not NPRA or NPRAB");
      return -1;
    }
  }

  int routerstop(int npu, char *rb)
  {
    unsigned int *response;

    if(rb != NULL) { response = (unsigned int *)&rb[0]; }

    if(npu == NPRA)
    {
      write_log(LOG_VERBOSE, "routerstop: stopping router for NPRA");
      if(npra_tcam_state == STOP)
      {
        write_log(LOG_NORMAL, "routerstop(NPRA): called for NPRA, but NPRA is already STOPed");
        if(rb != NULL) { *response = SUCCESS; }
        return sizeof(tcam_response);
      }

      write_log(LOG_NORMAL, "routerstop(NPRA): destroying databases for NPRA");
      if(npra_route) { sm->destroy_db(npra_route->id()); npra_route = NULL; }
      if(npra_pfilter) { sm->destroy_db(npra_pfilter->id()); npra_pfilter = NULL; }
      if(npra_afilter) { sm->destroy_db(npra_afilter->id()); npra_afilter = NULL; }
      if(npra_arp) { sm->destroy_db(npra_arp->id()); npra_arp = NULL; }

      npra_tcam_state = STOP;
      if(rb != NULL) { *response = SUCCESS; }
      return sizeof(tcam_response);
    }
    else if(npu == NPRB)
    {
      write_log(LOG_VERBOSE, "routerstop: stopping router for NPRB");
      if(nprb_tcam_state == STOP)
      {
        write_log(LOG_NORMAL, "routerstop(NPRB): called for NPRB, but NPRB is already STOPed");
        if(rb != NULL) { *response = SUCCESS; }
        return sizeof(tcam_response);
      }

      write_log(LOG_NORMAL, "routerstop(NPRB): destroying databases for NPRB");
      if(nprb_route) { sm->destroy_db(nprb_route->id()); nprb_route = NULL; }
      if(nprb_pfilter) { sm->destroy_db(nprb_pfilter->id()); nprb_pfilter = NULL; }
      if(nprb_afilter) { sm->destroy_db(nprb_afilter->id()); nprb_afilter = NULL; }
      if(nprb_arp) { sm->destroy_db(nprb_arp->id()); nprb_arp = NULL; }

      nprb_tcam_state = STOP;
      if(rb != NULL) { *response = SUCCESS; }
      return sizeof(tcam_response);
    }
    else
    {
      write_log(LOG_NORMAL, "routerstop: something is broken, npu is not NPRA or NPRAB");
      return -1;
    }
  }

  int addroute(int npu, int bsize, char *b, char *rb)
  {
    Database *r;
    std::string npu_str;

    route_info *route;
    fltr_t *new_fltr;

    wuparam_t status;

    unsigned int *response;

    if(rb != NULL) { response = (unsigned int *)&rb[0]; }

    if(npu == NPRA)
    {
      r = npra_route;
      npu_str = "NPRA";
    }
    else if(npu == NPRB)
    {
      r = nprb_route;
      npu_str = "NPRB";
    }
    else
    {
      write_log(LOG_NORMAL, "addroute: something is broken, npu is not NPRA or NPRAB");
      return -1;
    } 

    if(bsize < (int)(sizeof(tcam_command) + sizeof(route_info)))
    {
      write_log(LOG_NORMAL, "addroute(" + npu_str + "): input buffer is too small!");
      if(rb != NULL) { *response = FAILURE; }
      return sizeof(tcam_response);
    }

    route = (route_info *)&b[4];

    new_fltr = r->alloc_fltr();
    if(new_fltr == NULL)
    {
      write_log(LOG_NORMAL, "addroute(" + npu_str + "): allocating a new filter failed!");
      if(rb != NULL) { *response = FAILURE; }
      return sizeof(tcam_response);
    }
   
    new_fltr->key = &(route->key.c[0]);
    new_fltr->mask = &(route->mask.c[0]);
    new_fltr->result = (unsigned char *)(&(route->result));

    if((status = r->write_fltr(route->index, *new_fltr)) != WUIDT_SUCCESS)
    {
      if(status == IDT_ERROR_RESOURCE_IN_USE)
      {
        write_log(LOG_VERBOSE, "addroute(" + npu_str + "): filter index already used");
        if(rb != NULL) { *response = INUSE; }
      }
      else
      {
        write_log(LOG_NORMAL, "addroute(" + npu_str + "): writing new filter failed!");
        if(rb != NULL) { *response = FAILURE; }
      }
      return sizeof(tcam_response);
    }

    if(rb != NULL) { *response = SUCCESS; }
    return sizeof(tcam_response);
  }

  int delroute(int npu, int bsize, char *b, char *rb)
  {
    Database *r;
    std::string npu_str;

    unsigned int *index;

    wuparam_t status;

    unsigned int *response;

    if(rb != NULL) { response = (unsigned int *)&rb[0]; }

    if(npu == NPRA)
    {
      r = npra_route;
      npu_str = "NPRA";
    }
    else if(npu == NPRB)
    {
      r = nprb_route;
      npu_str = "NPRB";
    }
    else
    {
      write_log(LOG_NORMAL, "delroute: something is broken, npu is not NPRA or NPRAB");
      return -1;
    }

    write_log(LOG_VERBOSE, "delroute: entering delroute for " + npu_str);

    if(bsize < (int)(sizeof(tcam_command) + sizeof(tcam_index)))
    {
      write_log(LOG_NORMAL, "delroute(" + npu_str + "): input buffer is too small");
      if(rb != NULL) { *response = FAILURE; }
      return sizeof(tcam_response);
    }
   
    write_log(LOG_VERBOSE, "delroute(" + npu_str + "): input buffer size is ok");

    index = (unsigned int *)&b[4];

    write_log(LOG_VERBOSE, "delroute(" + npu_str + "): index to delete is: " + tcamlog->int2str(*index));
    if((status = r->rem_fltr(*index)) != WUIDT_SUCCESS)
    {
      if(status == IDT_ERROR_ALREADY_INVALID)
      {
        write_log(LOG_VERBOSE, "delroute(" + npu_str + "): filter already invalid");
        if(rb != NULL) { *response = ALREADYINVALID; }
      }
      else
      {
        write_log(LOG_NORMAL, "delroute(" + npu_str + "): rem_fltr failed with status " + tcamlog->int2str(status));
        if(rb != NULL) { *response = FAILURE; }
      }
      return sizeof(tcam_response);
    }

    write_log(LOG_VERBOSE, "delroute(" + npu_str + "): done with rem_fltr");

    if(rb != NULL) { *response = SUCCESS; }
    return sizeof(tcam_response);
  }

  int queryroute(int npu, int bsize, char *b, char *rb)
  {
    Database *r;
    std::string npu_str;

    npr::route_key *query_key;
    unsigned char query_result[4];

    wuparam_t status;

    unsigned int *response;

    if(rb != NULL) { response = (unsigned int *)&rb[0]; }

    if(npu == NPRA)
    {
      r = npra_route;
      npu_str = "NPRA";
    }
    else if(npu == NPRB)
    {
      r = nprb_route;
      npu_str = "NPRB";
    }
    else
    {
      write_log(LOG_NORMAL, "queryroute: something is broken, npu is not NPRA or NPRAB");
      return -1;
    }

    if(bsize < (int)(sizeof(tcam_command) + sizeof(npr::route_key)))
    {
      write_log(LOG_NORMAL, "queryroute(" + npu_str + "): input buffer is too small!");
      if(rb != NULL) { *response = FAILURE; }
      return sizeof(tcam_response);
    }

    query_key = (npr::route_key *)&b[4];
    if((status = r->lookup_fltr(&(query_key->c[0]), query_result)) != WUIDT_SUCCESS)
    {
      if(status == WUIDT_ERROR)
      {
        if(rb != NULL) { *response = NOTFOUND; }
        return sizeof(tcam_response);
      }
      write_log(LOG_NORMAL, "queryroute(" + npu_str + "): lookup filter failed!");
      if(rb != NULL) { *response = FAILURE; }
      return sizeof(tcam_response);
    }
    
    if(rb != NULL) { *response = SUCCESS; }
    rb[4] = query_result[0];
    rb[5] = query_result[1];
    rb[6] = query_result[2];
    rb[7] = query_result[3];
    return (sizeof(tcam_response) + sizeof(tcam_result));
  }

  int addpfilter(int npu, int bsize, char *b, char *rb)
  {
    Database *pf;
    std::string npu_str;

    pfilter_info *pfilter;
    fltr_t *new_fltr;

    wuparam_t status;

    unsigned int *response;

    if(rb != NULL) { response = (unsigned int *)&rb[0]; }

    if(npu == NPRA)
    {
      pf = npra_pfilter;
      npu_str = "NPRA";
    }
    else if(npu == NPRB)
    {
      pf = nprb_pfilter;
      npu_str = "NPRB";
    }
    else
    {
      write_log(LOG_NORMAL, "addpfilter: something is broken, npu is not NPRA or NPRAB");
      return -1;
    } 

    write_log(LOG_NORMAL, "addpfilter(" + npu_str + "): adding new filter");

    if(bsize < (int)(sizeof(tcam_command) + sizeof(pfilter_info)))
    {
      write_log(LOG_NORMAL, "addpfilter(" + npu_str + "): input buffer is too small!");
      if(rb != NULL) { *response = FAILURE; }
      return sizeof(tcam_response);
    }

    pfilter = (pfilter_info *)&b[4];

    new_fltr = pf->alloc_fltr();
    if(new_fltr == NULL)
    {
      write_log(LOG_NORMAL, "addpfilter(" + npu_str + "): allocating a new filter failed!");
      if(rb != NULL) { *response = FAILURE; }
      return sizeof(tcam_response);
    }
   
    new_fltr->key = &(pfilter->key.c[0]);
    new_fltr->mask = &(pfilter->mask.c[0]);
    new_fltr->result = (unsigned char *)(&(pfilter->result));

    if((status = pf->write_fltr(pfilter->index, *new_fltr)) != WUIDT_SUCCESS)
    {
      if(status == IDT_ERROR_RESOURCE_IN_USE)
      {
        write_log(LOG_VERBOSE, "addpfilter(" + npu_str + "): filter index already used");
        if(rb != NULL) { *response = INUSE; }
      }
      else
      {
        write_log(LOG_NORMAL, "addpfilter(" + npu_str + "): writing new filter failed!");
        if(rb != NULL) { *response = FAILURE; }
      }
      return sizeof(tcam_response);
    }

    if(rb != NULL) { *response = SUCCESS; }
    return sizeof(tcam_response);
  }

  int delpfilter(int npu, int bsize, char *b, char *rb)
  {
    Database *pf;
    std::string npu_str;

    unsigned int *index;

    wuparam_t status;

    unsigned int *response;

    if(rb != NULL) { response = (unsigned int *)&rb[0]; }

    if(npu == NPRA)
    {
      pf = npra_pfilter;
      npu_str = "NPRA";
    }
    else if(npu == NPRB)
    {
      pf = nprb_pfilter;
      npu_str = "NPRB";
    }
    else
    {
      write_log(LOG_NORMAL, "delpfilter: something is broken, npu is not NPRA or NPRAB");
      return -1;
    }

    write_log(LOG_VERBOSE, "delpfilter: entering delpfilter for " + npu_str);

    if(bsize < (int)(sizeof(tcam_command) + sizeof(tcam_index)))
    {
      write_log(LOG_NORMAL, "delpfilter(" + npu_str + "): input buffer is too small");
      if(rb != NULL) { *response = FAILURE; }
      return sizeof(tcam_response);
    }
   
    write_log(LOG_VERBOSE, "delpfilter(" + npu_str + "): input buffer size is ok");

    index = (unsigned int *)&b[4];

    write_log(LOG_VERBOSE, "delpfilter(" + npu_str + "): index to delete is: " + tcamlog->int2str(*index));
    if((status = pf->rem_fltr(*index)) != WUIDT_SUCCESS)
    {
      if(status == IDT_ERROR_ALREADY_INVALID)
      {
        write_log(LOG_VERBOSE, "delpfilter(" + npu_str + "): filter already invalid");
        if(rb != NULL) { *response = ALREADYINVALID; }
      }
      else
      {
        write_log(LOG_NORMAL, "delpfilter(" + npu_str + "): rem_fltr failed with status " + tcamlog->int2str(status));
        if(rb != NULL) { *response = FAILURE; }
      }
      return sizeof(tcam_response);
    }

    write_log(LOG_VERBOSE, "delpfilter(" + npu_str + "): done with rem_fltr");

    if(rb != NULL) { *response = SUCCESS; }
    return sizeof(tcam_response);
  }

  int querypfilter(int npu, int bsize, char *b, char *rb)
  {
    Database *pf;
    std::string npu_str;

    npr::pfilter_key *query_key;
    unsigned char query_result[4];

    wuparam_t status;

    unsigned int *response;

    if(rb != NULL) { response = (unsigned int *)&rb[0]; }

    if(npu == NPRA)
    {
      pf = npra_pfilter;
      npu_str = "NPRA";
    }
    else if(npu == NPRB)
    {
      pf = nprb_pfilter;
      npu_str = "NPRB";
    }
    else
    {
      write_log(LOG_NORMAL, "querypfilter: something is broken, npu is not NPRA or NPRAB");
      return -1;
    }

    if(bsize < (int)(sizeof(tcam_command) + sizeof(npr::pfilter_key)))
    {
      write_log(LOG_NORMAL, "querypfilter(" + npu_str + "): input buffer is too small!");
      if(rb != NULL) { *response = FAILURE; }
      return sizeof(tcam_response);
    }

    query_key = (npr::pfilter_key *)&b[4];
    if((status = pf->lookup_fltr(&(query_key->c[0]), query_result)) != WUIDT_SUCCESS)
    {
      if(status == WUIDT_ERROR)
      {
        if(rb != NULL) { *response = NOTFOUND; }
        return sizeof(tcam_response);
      }
      write_log(LOG_NORMAL, "querypfilter(" + npu_str + "): lookup filter failed!");
      if(rb != NULL) { *response = FAILURE; }
      return sizeof(tcam_response);
    }
    
    if(rb != NULL) { *response = SUCCESS; }
    rb[4] = query_result[0];
    rb[5] = query_result[1];
    rb[6] = query_result[2];
    rb[7] = query_result[3];
    return (sizeof(tcam_response) + sizeof(tcam_result));
  }

  int addafilter(int npu, int bsize, char *b, char *rb)
  {
    Database *af;
    std::string npu_str;

    afilter_info *afilter;
    fltr_t *new_fltr;

    wuparam_t status;

    unsigned int *response;

    if(rb != NULL) { response = (unsigned int *)&rb[0]; }

    if(npu == NPRA)
    {
      af = npra_afilter;
      npu_str = "NPRA";
    }
    else if(npu == NPRB)
    {
      af = nprb_afilter;
      npu_str = "NPRB";
    }
    else
    {
      write_log(LOG_NORMAL, "addafilter: something is broken, npu is not NPRA or NPRAB");
      return -1;
    } 

    if(bsize < (int)(sizeof(tcam_command) + sizeof(afilter_info)))
    {
      write_log(LOG_NORMAL, "addafilter(" + npu_str + "): input buffer is too small!");
      if(rb != NULL) { *response = FAILURE; }
      return sizeof(tcam_response);
    }

    afilter = (afilter_info *)&b[4];

    new_fltr = af->alloc_fltr();
    if(new_fltr == NULL)
    {
      write_log(LOG_NORMAL, "addafilter(" + npu_str + "): allocating a new filter failed!");
      if(rb != NULL) { *response = FAILURE; }
      return sizeof(tcam_response);
    }
   
    new_fltr->key = &(afilter->key.c[0]);
    new_fltr->mask = &(afilter->mask.c[0]);
    new_fltr->result = (unsigned char *)(&(afilter->result));

    if((status = af->write_fltr(afilter->index, *new_fltr)) != WUIDT_SUCCESS)
    {
      if(status == IDT_ERROR_RESOURCE_IN_USE)
      {
        write_log(LOG_VERBOSE, "addafilter(" + npu_str + "): filter index already used");
        if(rb != NULL) { *response = INUSE; }
      }
      else
      {
        write_log(LOG_NORMAL, "addafilter(" + npu_str + "): writing new filter failed!");
        if(rb != NULL) { *response = FAILURE; }
      }
      return sizeof(tcam_response);
    }

    if(rb != NULL) { *response = SUCCESS; }
    return sizeof(tcam_response);
  }

  int delafilter(int npu, int bsize, char *b, char *rb)
  {
    Database *af;
    std::string npu_str;

    unsigned int *index;

    wuparam_t status;

    unsigned int *response;

    if(rb != NULL) { response = (unsigned int *)&rb[0]; }

    if(npu == NPRA)
    {
      af = npra_afilter;
      npu_str = "NPRA";
    }
    else if(npu == NPRB)
    {
      af = nprb_afilter;
      npu_str = "NPRB";
    }
    else
    {
      write_log(LOG_NORMAL, "delafilter: something is broken, npu is not NPRA or NPRAB");
      return -1;
    }

    write_log(LOG_VERBOSE, "delafilter: entering delafilter for " + npu_str);

    if(bsize < (int)(sizeof(tcam_command) + sizeof(tcam_index)))
    {
      write_log(LOG_NORMAL, "delafilter(" + npu_str + "): input buffer is too small");
      if(rb != NULL) { *response = FAILURE; }
      return sizeof(tcam_response);
    }
   
    write_log(LOG_VERBOSE, "delafilter(" + npu_str + "): input buffer size is ok");

    index = (unsigned int *)&b[4];

    write_log(LOG_VERBOSE, "delafilter(" + npu_str + "): index to delete is: " + tcamlog->int2str(*index));
    if((status = af->rem_fltr(*index)) != WUIDT_SUCCESS)
    {
      if(status == IDT_ERROR_ALREADY_INVALID)
      {
        write_log(LOG_VERBOSE, "delafilter(" + npu_str + "): filter already invalid");
        if(rb != NULL) { *response = ALREADYINVALID; }
      }
      else
      {
        write_log(LOG_NORMAL, "delafilter(" + npu_str + "): rem_fltr failed with status " + tcamlog->int2str(status));
        if(rb != NULL) { *response = FAILURE; }
      }
      return sizeof(tcam_response);
    }

    write_log(LOG_VERBOSE, "delafilter(" + npu_str + "): done with rem_fltr");

    if(rb != NULL) { *response = SUCCESS; }
    return sizeof(tcam_response);
  }

  int queryafilter(int npu, int bsize, char *b, char *rb)
  {
    Database *af;
    std::string npu_str;

    npr::afilter_key *query_key;
    unsigned char query_result[4];

    wuparam_t status;

    unsigned int *response;

    if(rb != NULL) { response = (unsigned int *)&rb[0]; }

    if(npu == NPRA)
    {
      af = npra_afilter;
      npu_str = "NPRA";
    }
    else if(npu == NPRB)
    {
      af = nprb_afilter;
      npu_str = "NPRB";
    }
    else
    {
      write_log(LOG_NORMAL, "queryafilter: something is broken, npu is not NPRA or NPRAB");
      return -1;
    }

    if(bsize < (int)(sizeof(tcam_command) + sizeof(npr::afilter_key)))
    {
      write_log(LOG_NORMAL, "queryafilter(" + npu_str + "): input buffer is too small!");
      if(rb != NULL) { *response = FAILURE; }
      return sizeof(tcam_response);
    }

    query_key = (npr::afilter_key *)&b[4];
    if((status = af->lookup_fltr(&(query_key->c[0]), query_result)) != WUIDT_SUCCESS)
    {
      if(status == WUIDT_ERROR)
      {
        if(rb != NULL) { *response = NOTFOUND; }
        return sizeof(tcam_response);
      }
      write_log(LOG_NORMAL, "queryafilter(" + npu_str + "): lookup filter failed!");
      if(rb != NULL) { *response = FAILURE; }
      return sizeof(tcam_response);
    }
    
    if(rb != NULL) { *response = SUCCESS; }
    rb[4] = query_result[0];
    rb[5] = query_result[1];
    rb[6] = query_result[2];
    rb[7] = query_result[3];
    return (sizeof(tcam_response) + sizeof(tcam_result));
  }

  int addarp(int npu, int bsize, char *b, char *rb)
  {
    Database *r;
    std::string npu_str;

    arp_info *arp;
    fltr_t *new_fltr;

    wuparam_t status;

    unsigned int *response;

    if(rb != NULL) { response = (unsigned int *)&rb[0]; }

    if(npu == NPRA)
    {
      r = npra_arp;
      npu_str = "NPRA";
    }
    else if(npu == NPRB)
    {
      r = nprb_arp;
      npu_str = "NPRB";
    }
    else
    {
      write_log(LOG_NORMAL, "addarp: something is broken, npu is not NPRA or NPRAB");
      return -1;
    } 

    if(bsize < (int)(sizeof(tcam_command) + sizeof(arp_info)))
    {
      write_log(LOG_NORMAL, "addarp(" + npu_str + "): input buffer is too small!");
      if(rb != NULL) { *response = FAILURE; }
      return sizeof(tcam_response);
    }

    arp = (arp_info *)&b[4];

    new_fltr = r->alloc_fltr();
    if(new_fltr == NULL)
    {
      write_log(LOG_NORMAL, "addarp(" + npu_str + "): allocating a new filter failed!");
      if(rb != NULL) { *response = FAILURE; }
      return sizeof(tcam_response);
    }
   
    new_fltr->key = &(arp->key.c[0]);
    new_fltr->mask = &(arp->mask.c[0]);
    new_fltr->result = (unsigned char *)(&(arp->result));

    if((status = r->write_fltr(arp->index, *new_fltr)) != WUIDT_SUCCESS)
    {
      if(status == IDT_ERROR_RESOURCE_IN_USE)
      {
        write_log(LOG_VERBOSE, "addarp(" + npu_str + "): filter index already used");
        if(rb != NULL) { *response = INUSE; }
      }
      else
      {
        write_log(LOG_NORMAL, "addarp(" + npu_str + "): writing new filter failed!");
        if(rb != NULL) { *response = FAILURE; }
      }
      return sizeof(tcam_response);
    }

    if(rb != NULL) { *response = SUCCESS; }
    return sizeof(tcam_response);
  }

  int delarp(int npu, int bsize, char *b, char *rb)
  {
    Database *r;
    std::string npu_str;

    unsigned int *index;

    wuparam_t status;

    unsigned int *response;

    if(rb != NULL) { response = (unsigned int *)&rb[0]; }

    if(npu == NPRA)
    {
      r = npra_arp;
      npu_str = "NPRA";
    }
    else if(npu == NPRB)
    {
      r = nprb_arp;
      npu_str = "NPRB";
    }
    else
    {
      write_log(LOG_NORMAL, "delarp: something is broken, npu is not NPRA or NPRAB");
      return -1;
    }

    write_log(LOG_VERBOSE, "delarp: entering delarp for " + npu_str);

    if(bsize < (int)(sizeof(tcam_command) + sizeof(tcam_index)))
    {
      write_log(LOG_NORMAL, "delarp(" + npu_str + "): input buffer is too small");
      if(rb != NULL) { *response = FAILURE; }
      return sizeof(tcam_response);
    }
   
    write_log(LOG_VERBOSE, "delarp(" + npu_str + "): input buffer size is ok");

    index = (unsigned int *)&b[4];

    write_log(LOG_VERBOSE, "delarp(" + npu_str + "): index to delete is: " + tcamlog->int2str(*index));
    if((status = r->rem_fltr(*index)) != WUIDT_SUCCESS)
    {
      if(status == IDT_ERROR_ALREADY_INVALID)
      {
        write_log(LOG_VERBOSE, "delarp(" + npu_str + "): filter already invalid");
        if(rb != NULL) { *response = ALREADYINVALID; }
      }
      else
      {
        write_log(LOG_NORMAL, "delarp(" + npu_str + "): rem_fltr failed with status " + tcamlog->int2str(status));
        if(rb != NULL) { *response = FAILURE; }
      }
      return sizeof(tcam_response);
    }

    write_log(LOG_VERBOSE, "delarp(" + npu_str + "): done with rem_fltr");

    if(rb != NULL) { *response = SUCCESS; }
    return sizeof(tcam_response);
  }

  int queryarp(int npu, int bsize, char *b, char *rb)
  {
    Database *r;
    std::string npu_str;

    npr::arp_key *query_key;
    unsigned char query_result[4];

    wuparam_t status;

    unsigned int *response;

    if(rb != NULL) { response = (unsigned int *)&rb[0]; }

    if(npu == NPRA)
    {
      r = npra_arp;
      npu_str = "NPRA";
    }
    else if(npu == NPRB)
    {
      r = nprb_arp;
      npu_str = "NPRB";
    }
    else
    {
      write_log(LOG_NORMAL, "queryarp: something is broken, npu is not NPRA or NPRAB");
      return -1;
    }

    if(bsize < (int)(sizeof(tcam_command) + sizeof(npr::arp_key)))
    {
      write_log(LOG_NORMAL, "queryarp(" + npu_str + "): input buffer is too small!");
      if(rb != NULL) { *response = FAILURE; }
      return sizeof(tcam_response);
    }

    query_key = (npr::arp_key *)&b[4];
    if((status = r->lookup_fltr(&(query_key->c[0]), query_result)) != WUIDT_SUCCESS)
    {
      if(status == WUIDT_ERROR)
      {
        if(rb != NULL) { *response = NOTFOUND; }
        return sizeof(tcam_response);
      }
      write_log(LOG_NORMAL, "queryarp(" + npu_str + "): lookup filter failed!");
      if(rb != NULL) { *response = FAILURE; }
      return sizeof(tcam_response);
    }
    
    if(rb != NULL) { *response = SUCCESS; }
    rb[4] = query_result[0];
    rb[5] = query_result[1];
    rb[6] = query_result[2];
    rb[7] = query_result[3];
    return (sizeof(tcam_response) + sizeof(tcam_result));
  }
}; // namespace tcamd
