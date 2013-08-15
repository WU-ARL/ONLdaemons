#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "queue.h"

struct MyNode {
  int value;
  QNODE(MyNode) alist;
};

struct MyNode root, *rptr;

int
main(int argc, char **argv)
{
  struct MyNode Nodes[10], *nptr;
  int i;

  rptr = &root;
  QNODE_INIT(rptr, alist);
  for (i = 0, nptr = Nodes; i < 10; i++, nptr++) {
    QNODE_INIT(nptr, alist);
    nptr->value = i;
    QINSERT_BEFORE(rptr, nptr, alist);
  }

  QFOREACH(rptr, nptr, alist) {
    printf("Node %d\n", nptr->value);
  }

  for (i = 0, nptr = Nodes; i < 10; i++, nptr++) {
    QREMOVE(nptr, alist);
  }
  printf("Removed nodes ... \n");
  QFOREACH(rptr, nptr, alist) {
    printf("Node %d\n", nptr->value);
  }
  QINSERT_BEFORE(rptr, &Nodes[0], alist);
  QINSERT_AFTER(&Nodes[0], &Nodes[1], alist);
  QINSERT_BEFORE(&Nodes[1], &Nodes[2], alist);
  QINSERT_AFTER(&Nodes[2], &Nodes[3], alist);
  QINSERT_BEFORE(&Nodes[3], &Nodes[4], alist);
  QINSERT_AFTER(&Nodes[4], &Nodes[5], alist);
  QINSERT_BEFORE(&Nodes[5], &Nodes[6], alist);
  QINSERT_AFTER(&Nodes[6], &Nodes[7], alist);
  QINSERT_BEFORE(&Nodes[7], &Nodes[8], alist);
  QINSERT_AFTER(&Nodes[8], &Nodes[9], alist);

  QFOREACH(rptr, nptr, alist) {
    printf("Node %d\n", nptr->value);
  }

  printf("Removed nodes ... \n");
  for (i = 0, nptr = Nodes; i < 10; i++, nptr++) {
    QREMOVE(nptr, alist);
  }
  QFOREACH(rptr, nptr, alist) {
    printf("Node %d\n", nptr->value);
  }

  exit(0);
}
