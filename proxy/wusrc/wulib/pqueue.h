/* 
 * Description: Priority Queues
 *
 * $Author: fredk $
 * $Date: 2008/03/11 22:04:07 $
 * $Revision: 1.5 $
 *
 * Auther: Fred Kuhns
 * Organization: Applied Research Laboratory
 *               Computer Science Department
 *               Washington University in St. Louis
 *
 * */

#ifndef _WU_PQUEUE_H
#define _WU_PQUEUE_H

#define PQ_INVALID_KEY   -1

#include <wulib/syn.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct PQItem_t {
  int             key;
  void            *value;
  struct PQItem_t *next;
  struct PQItem_t **prev;
} PQItem_t;

typedef struct {
  PQItem_t *head;
  void *lock;
  void *attr;
  wusynops_mutex_t *synops;
} PQueue_t;

int pqinit (PQueue_t *pq);
int pqfree (PQueue_t *pq);

PQItem_t *pqadd(PQueue_t *pq, int key, void *value);
int pqrem(PQueue_t *pq, PQItem_t *node);
void *pqget(PQueue_t *pq, int max);
void pqprint(char *str, PQueue_t *pq);

#ifdef __cplusplus
}
#endif
#endif /* _WU_QUEUE_H */
