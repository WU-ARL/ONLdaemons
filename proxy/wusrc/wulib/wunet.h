/* 
 * $Source: /project/arl/arlcvs/cvsroot/wu_arl/wusrc/wulib/wunet.h,v $
 * $Author: fredk $
 * $Date: 2007/01/14 17:24:48 $
 * $Revision: 1.13 $
 * $Name:  $
 *
 * File:         
 * Author:       Fred Kuhns <fredk@arl.wustl.edu>
 * Organization: Applied Research Laboratory,
 *               Washington University in St. Louis
 * Description:
 *
 *
 */

#ifndef _WUNET_H
#define _WUNET_H
#ifdef __cplusplus
extern "C" {
#endif

// all the includes needed
#include <sys/param.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>

// xxx.xxx.xxx.xxx\0 == 16 chars
//    ^   ^   ^   ^
// 0  3   7  11   15
// So max strlen(ip) == 15 < MaxIPStringLen
#ifdef INET_ADDRSTRLEN
#  define MaxIPStringLen     INET_ADDRSTRLEN
#else
#  define MaxIPStringLen     16
#endif
#ifdef MAXHOSTNAMELEN
#  define MaxHostLength  MAXHOSTNAMELEN
#else
#  define MaxHostLength      64
#endif
// port and proto should have enough to hold a 16-bit in (0 ... 65536-1)
// or 5 chars plus the terminating null char.  I make it 8 for alignment
// reasons.
#define MaxPortString       8
// valid protocols: tcp, udp, icmp, ...
#define MaxProtoString      8
#define MaxPacketSize    2048

/* The purpose of this struct is to:
 *   1) cache address mappings
 *   2) record local address mappings
 *   */
typedef struct WUHostAddr_t {
  uint32_t addr;
  char     host[MaxHostLength];
  char     ip[MaxIPStringLen];
} WUHostAddr_t;
static inline void wunet_initHAddr(WUHostAddr_t *);
static inline void wunet_initHAddr(WUHostAddr_t *wna)
{
  wna->addr    = htonl(INADDR_ANY);;
  wna->host[0] = '\0';
  wna->ip[0]   = '\0';
  return;
}

typedef struct WUSinAddr_t {
  WUHostAddr_t       haddr;
  struct sockaddr_in   sin;
} WUSinAddr_t;
static inline void wunet_initSinAddr(WUSinAddr_t *);
static inline void wunet_initSinAddr(WUSinAddr_t *saddr)
{
  wunet_initHAddr(&saddr->haddr);
  memset((void *)&saddr->sin, 0, sizeof(struct sockaddr_in));
  // Assume it is AF_INIT
  saddr->sin.sin_family = AF_INET;
  saddr->sin.sin_addr.s_addr = htonl(INADDR_ANY);
};

typedef struct WUAddr_t {
  uint32_t addr;
  uint16_t port;
  int      proto;
} WUAddr_t;

typedef struct WUAddrStr_t {
  WUHostAddr_t haddr;
  char         port[MaxPortString];
  char         proto[MaxProtoString];
} WUAddrStr_t;
static inline void wunet_initAddrStr(WUAddrStr_t *);
static inline void wunet_initAddrStr(WUAddrStr_t *astr)
{
  wunet_initHAddr(&(astr->haddr));
  astr->port[0] = '\0';
  astr->proto[0] = '\0';
}

int wunet_getHost(WUHostAddr_t *myaddr);
int wunet_getAddr(const char *host, WUHostAddr_t *myaddr);
void wunet_Addr2String(WUAddr_t *waddr, WUAddrStr_t *astr);
int wunet_getSAddr(const char *, int port, WUSinAddr_t*);

uint16_t wunet_cksum16(uint16_t *addr, int len);

ssize_t wunet_readn(int sd, void *buf, size_t len);
ssize_t wunet_writen(int sd, void *buf, size_t len);

#ifdef __cplusplus
}
#endif
#endif /* _WUNET_H */
