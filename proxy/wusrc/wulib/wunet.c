/*
 * $Author: fredk $
 * $Date: 2007/01/14 17:02:06 $
 * $Revision: 1.11 $
 *
 * File:
 * Author:       Fred Kuhns <fredk@arl.wustl.edu>
 * Organization: Applied Research Laboratory,
 *               Washington University in St. Louis
 * Description:
 *
 * */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <net/if.h>
#include <netinet/in.h>
#include <ctype.h>
#include <unistd.h>
#include <arpa/inet.h>

#include <errno.h>
#include <wulib/wulog.h>
#include <wulib/wunet.h>

#include <netdb.h>

/* right out of RFC 1071 and Stevens */
uint16_t wunet_cksum16(uint16_t *addr, int len)
{
  register uint32_t sum = 0;
  int nleft = len;
  uint16_t *w = addr;
  uint16_t answer = 0;

  while (nleft > 1) {
    sum += *w++;
    nleft -= 2;
  }

  /* mop up an odd byte, if necessary */
  if (nleft == 1) {
    *(uint8_t *)&answer = *(uint8_t *)w;
    sum += answer;
  }

  /* add back carry outs from top 16 bits to low 16 bits */
  sum = (sum >> 16) + (sum & 0xffff); /* Add hi 16 to lo 16 */
  sum += (sum >> 16);  /* add cary */
  answer = ~sum; /* truncate to 16 bits */
  return answer;
}

void wunet_Addr2String(WUAddr_t *waddr, WUAddrStr_t *astr)
{
  struct protoent *pent;

  wunet_initAddrStr(astr);
  astr->haddr.addr = waddr->addr;

  // Host
  wunet_getHost(&astr->haddr);

  // port
  sprintf(astr->port, "%d", waddr->port);

  // proto
  pent = getprotobynumber(waddr->proto);
  if (pent && pent->p_name)
    strcpy(astr->proto, pent->p_name);
  else
    sprintf(astr->proto, "%d", waddr->proto);

  return;
}

int wunet_getHost(struct WUHostAddr_t *myaddr)
{
  struct hostent *hp;
  struct in_addr inaddr;

  inaddr.s_addr = myaddr->addr;

  myaddr->host[0] = '\0';
  myaddr->ip[0]   = '\0';

  if (myaddr->addr != 0) {
    if ((hp = gethostbyaddr((const void *)&inaddr, sizeof(inaddr), AF_INET)) == NULL)
      wulog(wulogLib, wulogInfo, "wunet_getHost: unable to lookup address 0X%08x\n", myaddr->addr);
    else
      wustrncpy(myaddr->host, hp->h_name, MaxHostLength-1);
  }
  strcpy(myaddr->ip, inet_ntoa(inaddr));
  return 0;
}

int wunet_getAddr(const char *host, struct WUHostAddr_t *myaddr)
{
  struct hostent *hp;
  struct in_addr inaddr;

  if (myaddr == NULL)
    return -1;

  wunet_initHAddr(myaddr);
  if (host == NULL || *host == '\0')
    return 0;

  if (isdigit((int)*host)) {
    /* Got an IP dotted address X.X.X.X */
    if (strlen(host) < MaxIPStringLen) {
      strcpy(myaddr->ip, host);
    } else {
      wulog(wulogLib, wulogError, "wunet_getAddr: Invalid IP address: %s.\n", host);
      return -1;
    }
    if ((hp = gethostbyname(host)) == NULL) {
      wulog(wulogLib, wulogInfo, "wunet_getAddr: Lookup of IP failed: %s.\n", host);
    } else {
      wustrncpy(myaddr->host, hp->h_name, MaxHostLength);
      if (inet_aton((const char *)host, &inaddr) == 0) {
        wulog(wulogLib, wulogError, "Unable to convert IP address %s!\n", host);
        return -1;
      }
    }
    myaddr->addr = inaddr.s_addr;
  } else {
    /* got a hostname, we need to do a DNS/hosts lookup */
    if ((hp = gethostbyname(host)) == NULL) {
      wulog(wulogLib, wulogError, "wunet_getAddr: Lookup of Hostname failed %s, h_errno = %d.\n", host, h_errno);
      return -1;
    }
    wustrncpy(myaddr->host, hp->h_name, MaxHostLength);

    inaddr.s_addr = ((struct in_addr *)hp->h_addr)->s_addr;
    strcpy(myaddr->ip, inet_ntoa(inaddr));
    myaddr->addr = inaddr.s_addr;
  }
  return 0;
}

int wunet_getSAddr(const char *host, int port, struct WUSinAddr_t *mySin)
{
  wunet_initSinAddr(mySin);
  if (wunet_getAddr(host, &mySin->haddr) < 0) {
    wulog(wulogLib, wulogError, "wunet_getSAddr: Error resolving host %s\n", host);
    return -1;
  }
  // should already be in NBO
  mySin->sin.sin_addr.s_addr = mySin->haddr.addr;
  mySin->sin.sin_port        = htons(port);
  return 0;
}

ssize_t wunet_readn(int sd, void *buf, size_t len)
{
  // n == 0 on EOF
  int nleft = len, n;
  char *ptr = (char *)buf;

  while (nleft > 0) {
#ifdef __CYGWIN__
    // cygwin doesn't support the MSG_WAITALL flag
    n = recv(sd, buf, nleft, 0);
#else
    n = recv(sd, buf, nleft, MSG_WAITALL);
#endif
    if (n < 0) {
      if (errno == EINTR) {
        wulog(wulogLib, wulogInfo, "readn: Errno == EINTR ... retrying (sd = %d)\n", sd);
        continue;
      }
      wulog(wulogLib, wulogError, "Error reading data, errno = %d\n", errno);
      return n;
    } else if (n == 0) {
      wulog(wulogLib, wulogVerb, "readn: Got EOF on sd = %d\n", sd); 
      return 0;
    }
    nleft -= n;
    ptr   += n;
  }
  return len;
}

ssize_t wunet_writen(int sd, void *buf, size_t len)
{
  int nleft = len, n;
  const char *ptr = (char *)buf;

  while (nleft > 0) {
    if ((n = write(sd, buf, nleft)) < 0) {
      if (errno == EINTR) {
        wulog(wulogLib, wulogInfo, "writen: Errno == EINTR ... retrying (sd = %d)\n", sd);
        continue;
      }
      wulog(wulogLib, wulogError, "writen: Error writing to socket (sd = %d), errno = %d\n",
            sd, errno);
      return -1;
    }
    nleft -= n;
    ptr   += n;
  }
  return len;
}

