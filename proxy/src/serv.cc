/*
 * serv.c
 *
 * $Source: /project/arl/arlcvs/cvsroot/wu_arl/openNetLab/proxy/serv.cc,v $
 *
 * $Author: fredk $ 
 * $Date: 2006/04/28 16:49:19 $ 
 * $Revision: 1.9 $
 */

#include <stdio.h>
#include <stdlib.h>
#ifdef __linux__
#include <string.h>
#else
#include <strings.h>
#endif
#include <unistd.h>

#include <sys/types.h>
#include <sys/time.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/file.h>
#include <sys/mman.h>
/* #include <stropts.h> */
#include <sys/stat.h>
#include <fcntl.h>

#include <net/if.h>
#include <netinet/in_systm.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netinet/ip.h>
#include <netdb.h>

#include <errno.h>

#include <pthread.h>
#include <wulib/wulog.h>
#include <wulib/wunet.h>
#include <wulib/keywords.h>

int port=6060;
int echoCnt = 1;

enum sendMode {fullSend, twoPart, byteWise, sendCnt};

KeyInfo_t modeArgs[] = {
  // flag, *name, *desc
  {(uint32_t)fullSend, "full", "send reply as one contiguous message (one send)"},
  {(uint32_t)twoPart,  "two",  "send reply in two steps: first header then payload"},
  {(uint32_t)byteWise, "byte", "send message one byte at a time"},
  {(uint32_t)sendCnt,  NULL,   NULL}
};

extern char *optarg;
extern int optind, opterr, optopt;

void syntax(void);

void
syntax(char *cmd)
{
  printf("Syntax: %s [-v] [-h] [-l local_address] [-p port] [-P (t|u)]" 
         "\n\t-w : logging level mode"
         "\n\t-l addr : set local address (i.e. the ip source address)"
         "\n\t-m mode : sending mode [full, two, byte]"
         "\n\t-e n: echo packet n times back to sender.  Default = %d"
         "\n\t-h : print this message"
         "\n\t-p port => listen port number.  Default = %d\n", cmd,
         echoCnt, port);
}

sendMode mode = fullSend;

void *doServ(void *arg)
{
  int fd = *(int *)arg;
  char Buf[MaxPacketSize];
  char *pkt = Buf;

  int *plen = (int *)pkt;
  int  hlen = sizeof(int);
  int  n, dlen;

#define doSendExit() {close (fd); delete (int *)arg; pthread_exit(NULL);}

  for (;;) {

    // Header header (first 4-byte word)
    if ((n = wunet_readn(fd, (void *)plen, hlen)) == 0) {
      wulog(wulogServ, wulogInfo, "Connection %d closed ...\n", fd);
      break;
    } else if (n < 0) {
      debugLog(wulogServ, wulogError, "Trouble receiving packets ... exiting.", errno);
      break;
    }

    dlen = ntohl(*plen);
    wulog(wulogServ, wulogVerb, "read header: Got %d bytes, payload is %d bytes\n", n, dlen);

    if ((n = wunet_readn(fd, (void *)(pkt+hlen), dlen)) == 0) {
      wulog(wulogServ, wulogError, "Connection closed mid message, fd %d\n", fd);
      break;
    } else if (n < 0) {
      debugLog(wulogServ, wulogError, "Trouble mid-message ... exiting.", errno);
      break;
    }

    if (n != dlen) {
      wulog(wulogServ, wulogError, "Read payload: Wanted %d Got %d\n", dlen, n);
      break;
    }

    if (echoCnt > 0) {

      wulog(wulogServ, wulogLoud, "\t--> Sending %d bytes packet back to source - %d Times.\n",
            dlen+hlen, echoCnt);

      if (mode == byteWise) {
        wulog(wulogServ, wulogLoud, "\t\t Sending Byte-by-Byte\n");
      } else if (mode == twoPart) {
        wulog(wulogServ, wulogLoud, "\t\t Sending header first then payload\n");
      }

      for (int cnt = 0; cnt < echoCnt; ++cnt) {
        if (twoPart) {
          if ((n = wunet_writen(fd, (char *)pkt, hlen)) < 0 || n != hlen) {
            debugLog(wulogServ, wulogError, "Unable to send header back", errno);
            doSendExit();
          }

          // add some delay
          //for (int x = 0; x < 1000000;++x) ;

          if ((n = wunet_writen(fd, (char *)pkt+hlen, dlen)) < 0 || n != dlen) {
            debugLog(wulogServ, wulogError, "Unable to send payload back", errno);
            doSendExit();
          }
        } else if (byteWise) {
          for (int b = 0; b < dlen+hlen; ++b) {
            if ((n = write(fd, (char *)(pkt + b), 1)) < 0 || n != 1) {
              debugLog(wulogServ, wulogError, "Unable to send byte back", errno);
              doSendExit();
            }
          }
        } else {
          if ((n = wunet_writen(fd, (char *)pkt, dlen+hlen)) < 0 || n != (dlen+hlen)) {
            debugLog(wulogServ, wulogError, "Unable to send message back", errno);
            doSendExit();
          }
        } // mode
      } // for cnt
    } /* do Echo */
  } /* while connection open */
  doSendExit();
}

int makeThread(pthread_t *tid, int fd)
{
  int r;
  pthread_attr_t tattr;

  if ((r = pthread_attr_init(&tattr)) != 0) {
    debugLog(wulogServ, wulogError, "pthread_attr_init failed", r);
    return -1;
  }

  if ((r = pthread_attr_setdetachstate (&tattr, PTHREAD_CREATE_DETACHED)) != 0) {
    debugLog(wulogServ, wulogError, "pthread_attr_setdetachstate failed", r);
    return -1;
  }

  if ((r = pthread_attr_setscope(&tattr, PTHREAD_SCOPE_SYSTEM)) != 0) {
    debugLog(wulogServ, wulogError, "pthread_attr_setscope failed", r);
    return -1;
  }

  int *args = new int(fd);
  if (pthread_create(tid,  &tattr, doServ, (void *)args) != 0) {
    delete args;
    debugLog(wulogServ, wulogError, "pthread_create failed", r);
    return -1;
  }

  return 0;
}

int
main(int argc, char **argv)
{
  int sd, fd;
  int sockLen = sizeof(struct sockaddr_in);
  struct sockaddr_in sin, from;
  int c;
  wulogFlags_t logFlags = wulog_useLocal | wulog_useTStamp;
  wulogLvl_t   logLvl   = wulogInfo;
  pthread_t tid;

  bzero((char *)&sin, sizeof(struct sockaddr));
  sin.sin_family = AF_INET;                 /* IPv4 */
  sin.sin_addr.s_addr = htonl(INADDR_ANY);  /* pick right src address */
  sin.sin_port = htons(port);               /* set port */

  wulogInit(NULL, 0, logLvl, "Echo", logFlags);

  while ((c = getopt(argc, argv, "m:e:l:p:hw:")) != -1) {
    switch ((char) c) {
      case 'm':
        switch (str2flag(optarg, modeArgs)) {
          case (uint32_t)fullSend:
              mode = fullSend;
              break;
          case (uint32_t)twoPart:
            mode = twoPart;
            break;
          case (uint32_t)byteWise:
            mode = byteWise;
            break;
          default:
            wulog(wulogServ, wulogError, "Invalid send mode\n");
            syntax(argv[0]);
            return 1;
        }
        break;
      case 'h':
        syntax(argv[0]);
        return 1;
        break;
      case 'e':
        if ((echoCnt = atoi(optarg)) < 0) {
          syntax(argv[0]);
          return 1;
        }
        break;
      case 'w':
        logLvl = wulogName2Lvl(optarg);
        if (logLvl == wulogInvLvl) {
          printf("Invalid wulog level specified: %s\n", optarg);
          wulogPrintLvls();
          return 1;
        }
        wulogSetDef(logLvl);
        break;
      case 'l':
        {
          struct hostent *hp;
          if ((hp = gethostbyname(optarg)) == 0) {
            printf("Unable to resolve address %s\n", optarg);
            syntax(argv[0]);
            return 1;
          }
          memcpy(&sin.sin_addr, *(hp->h_addr_list), sizeof (sin.sin_addr));
        }
        break;
      case 'p':
        if ((port = atoi(optarg)) <= 0) {
          printf("\nInvalid port number!\n\n");
          syntax(argv[0]);
          return 1;
        }
        sin.sin_port = htons(port);
        break;
      default:
        printf("Unrecognized option!\n");
        syntax(argv[0]);
        return 1;
        break;
    }
  }

  if ((sd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
    printf("\nUnable to create receive socket!\n\n");
    return 1;
  }

  {
    int val = 1;
    if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (char *)&val, sizeof(val)) < 0) {
      printf("\nError setting SO_REUSEADDR\n");
      return 1;
    }
  }

  setlinebuf(stdout);

  if (bind(sd, (struct sockaddr *)&sin, sizeof(struct sockaddr_in)) != 0) {
    printf("\nError binding socket!\n\n");
    return 1;
  }

  if (listen(sd, 10) != 0) {
    printf("\nlisten failed\n");
    return 1;
  }

  printf("Waiting for a connection request ...\n");

  sockLen = sizeof(struct sockaddr_in);
  while ((fd = accept(sd, (struct sockaddr *)&from, (socklen_t *)&sockLen)) > 0) {
    printf("Got connection on socket %d\n", fd);
    if (makeThread(&tid, fd) < 0) {
      wulog(wulogServ, wulogError, "makeThread returned error, exiting");
      return 1;
    }
  }

  close(sd);
  return 0;
}
