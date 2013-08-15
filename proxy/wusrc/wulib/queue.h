/* 
 * Description: Priority Queues
 *
 * $Author: fredk $
 * $Date: 2006/11/19 13:41:46 $
 * $Revision: 1.5 $
 *
 * Auther: Fred Kuhns
 * Organization: Applied Research Laboratory
 *               Computer Science Department
 *               Washington University in St. Louis
 *
 * */

#ifndef _WU_QUEUE_H
#define _WU_QUEUE_H

#ifdef __cplusplus
extern "C" {
#endif

#define Q_INVALID_KEY   -1

#define QNODE(type)   \
struct {              \
  struct type *next;  \
  struct type **prev; \
}

#define QNODE_INIT(node, field)               \
  do {                                        \
    (node)->field.next = (node);              \
    (node)->field.prev = &(node)->field.next; \
  } while ( /* */ 0 );

#define QINSERT_BEFORE(loc, node, field)        \
  do {                                          \
    *(loc)->field.prev = (node);                \
    (node)->field.prev = (loc)->field.prev;     \
    (loc)->field.prev  = &((node)->field.next); \
    (node)->field.next = (loc);                 \
  } while (/* */0)

#define QINSERT_AFTER(loc, node, field)                    \
  do {                                                     \
    ((loc)->field.next)->field.prev = &(node)->field.next; \
    (node)->field.next = (loc)->field.next;                \
    (loc)->field.next = (node);                            \
    (node)->field.prev = &(loc)->field.next;               \
  } while ( /* */ 0)

#define QREMOVE(node, field)                               \
  do {                                                     \
    *((node)->field.prev) = (node)->field.next;            \
    ((node)->field.next)->field.prev = (node)->field.prev; \
    (node)->field.next = (node);                           \
    (node)->field.prev = &((node)->field.next);            \
  } while ( /* */ 0)

#define QFIRST2(head, field) (((head)->field.next == (head)) ? NULL : ((head)->field.next))
#define QFIRST(head, field) ((head)->field.next)
#define QNEXT(node, field)  ((node)->field.next)
#define QEMPTY(head, field) ((head)->field.next == (head))
#define ISQTAIL(head, node, field)  ((node)->field.next == (head))
#define ONLIST(node, field) ((node)->field.next != (node))
#define QFOREACH(head, var, field) \
  for ((var) = (head)->field.next; (var) != (head); (var) = (var)->field.next)

#ifdef __cplusplus
}
#endif

#endif
