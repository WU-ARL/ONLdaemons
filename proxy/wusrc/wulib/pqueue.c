/*
 * Description: Priority Queues
 * $Author: fredk $
 * $Date: 2008/03/11 22:04:07 $
 * $Revision: 1.6 $
 *
 * Organization: Applied Research Laboratory
 *               Computer Science Department
 *               Washington University in St. Louis
 *
 * */
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <wulib/wulog.h>
#include <wulib/pqueue.h>

#define pqfirst(pq) ((pq)->head->next)
#define pqhead(pq)  ((pq)->head)
#define pqnext(n)   ((n)->next)

static inline void pqbefore_i (PQItem_t *node, PQItem_t *loc);
static inline void pqafter_i (PQItem_t *node, PQItem_t *loc);
static inline void pqrem_i (PQItem_t *node);
static inline PQItem_t *pqitem_alloc(int key, void *value);
static inline void pqitem_free(PQItem_t *node);
static inline int pqlock(PQueue_t *pq);
static inline int pqunlock(PQueue_t *pq);

int
pqfree (PQueue_t *pq)
{
  WUASSERT(pq && pq->head);

  if (pq->head->next != pq->head) {
    printf("pqfree: Error, pq is not empty!\n");
    return -1;
  }

  pqitem_free(pq->head);

  if (pq->lock && pq->synops->lock_destroy) {
    if (pq->synops->lock_destroy(pq->lock) < 0) {
      printf("pqfree: Error destroying mutex\n");
      return -1;
    }
  }

  return 0;
}

int
pqinit (PQueue_t *pq)
{
  PQItem_t *node;

  WUASSERT(pq);

  if (pq->lock && pq->synops->lock_init) {
    if (pq->synops->lock_init(pq->lock, pq->attr) < 0) {
      printf("pqinit: unable to initialize the mutex!\n");
      return -1;
    }
  }

  if ((node = pqitem_alloc(PQ_INVALID_KEY, NULL)) == NULL) {
    printf("pqadd: Unable to allocate memory for node queue node\n");
    return -1;
  }

  pq->head = node;

  return 0;
}

static inline void
pqbefore_i (PQItem_t *node, PQItem_t *loc)
{
  /* Put node before loc */
  WUASSERT(loc && node);

  *(loc->prev) = node;
  node->prev   = loc->prev;
  loc->prev    = &(node->next);
  node->next   = loc;

  return;
}

static inline void
pqafter_i (PQItem_t *node, PQItem_t *loc)
{
  WUASSERT(loc && node);

  (loc->next)->prev = &(node->next);
  node->next = loc->next;
  loc->next  = node;
  node->prev = &(loc->next);

  return;
}

static inline void
pqrem_i (PQItem_t *node)
{
  if (node == NULL) return;

  *(node->prev) = node->next;
  (node->next)->prev = node->prev;

  node->next = node;
  node->prev = &(node->next);

  return;
}

static inline PQItem_t *
pqitem_alloc(int key, void *value)
{
  PQItem_t *node;

  if ((node = (PQItem_t *)calloc(1, sizeof(PQItem_t))) == NULL) {
    printf("pqitem_alloc: Unable to allocate memory for node queue node\n");
    return NULL;
  }

  node->key = key;
  node->value = value;
  node->next = node;
  node->prev = &(node->next);

  return node;
}

static inline void
pqitem_free (PQItem_t *node)
{
  if (node == NULL) return;

  free(node);
  return;
}

static inline int
pqlock(PQueue_t *pq)
{
  WUASSERT(pq);
  return (pq->lock ? pq->synops->lockit(pq->lock) : 0);
}

static inline int
pqunlock(PQueue_t *pq)
{
  WUASSERT(pq);
  return (pq->lock ? pq->synops->unlockit(pq->lock) : 0);
}


PQItem_t *
pqadd(PQueue_t *pq, int key, void *value)
{
  PQItem_t *node, *cur;

  if (pqlock(pq) < 0)
    return NULL;

  if ((node = pqitem_alloc(key, value)) == NULL) {
    wulog(wulogQ, wulogError,
          "pqadd: Unable to allocate memory for node queue node\n");
    pqunlock(pq);
    return NULL;
  }

  for (cur = pqfirst(pq); cur != pqhead(pq); cur = pqnext(cur)) {
    if (cur->key > node->key)
      break;
  }

  pqbefore_i(node, cur);

  pqunlock(pq);
  return node;
}

int
pqrem(PQueue_t *pq, PQItem_t *node)
{
  WUASSERT(node);
  if (node == pq->head) {
    wulog(wulogQ, wulogError, "pqrem: Error, attempting to remove pq head!\n");
    return -1;
  }

  if (pqlock(pq) < 0)
    return -1;

  pqrem_i(node);
  pqitem_free(node);

  pqunlock(pq);

  return 0;
}

void pqprint(char *str, PQueue_t *pq)
{
  int i;
  PQItem_t *cur;

  if (pqlock(pq) < 0)
    return;

  if (pqfirst(pq) == NULL) {
    printf("pqprint: Queue empty\n");
    pqunlock(pq);
    return;
  }

  printf("%s Priority Queue:\n",
      (str == NULL) ? "pqprint" : str);
  for (i = 0, cur = pqfirst(pq); cur != pqhead(pq); cur = pqnext(cur), i++) {
    printf("\t%d key %d, Value %p\n",
        i, (int)cur->key, cur->value);
  }

  pqunlock(pq);
  return;
}

void
*pqget(PQueue_t *pq, int max)
{
  PQItem_t *node;
  void *result;

  if (pqlock(pq) < 0)
    return NULL;

  result = NULL;
  node   = pqfirst(pq);
  if (node != pqhead(pq) && node->key <= max) {
    /* got one */
    pqrem_i(node);
    result = node->value;
    pqitem_free(node);
  }

  pqunlock(pq);
  return result;
}
