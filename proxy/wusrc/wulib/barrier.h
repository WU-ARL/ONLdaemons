/*
 * $Source: /project/arl/arlcvs/cvsroot/wu_arl/wusrc/wulib/barrier.h,v $
 * $Author: fredk $
 * $Date: 2005/06/16 22:23:23 $
 * $Revision: 1.6 $
 *
 * Author: Fred Kuhns
 *         fredk@arl.wustl.edu
 * Organization: Applied Research Laboratory
 *               Washington University in St. Louis
 * */

#ifndef _WU_BARRIER_H
#define _WU_BARRIER_H

#include <wulib/syn.h>

#ifdef __cplusplus
extern "C" {
#endif

/* ****************** Barrier ******************* 
 * This is a *very* simple barrier and does not address more general
 * concerns like re-use
 * */
typedef struct {
  wusynops_t *sops;
  void       *lock;
  void         *cv;
  int        count;
  int        cycle;
  int     nthreads;
} wubarrier_t;

int init_barrier(int count, wubarrier_t *barrier, int syncType);
int free_barrier(wubarrier_t *barrier);
int barrier_wait (wubarrier_t *barrier);

/* ******** Done with Barriers **************** */

#ifdef __cplusplus
extern "C" {
#endif

#endif
