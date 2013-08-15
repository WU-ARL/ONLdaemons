/* 
 * $Source: /project/arl/arlcvs/cvsroot/wu_arl/wusrc/wulib/ipnet.h,v $
 * $Author: fredk $
 * $Date: 2007/01/17 01:35:08 $
 * $Revision: 1.21 $
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

#ifndef _IPNET_H
#define _IPNET_H
#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#ifdef __linux__
#define __FAVOR_BSD
#endif
#include <netinet/tcp.h>
#include <netinet/udp.h>
#ifdef __linux__
#undef __FAVOR_BSD
#endif

#include <sys/param.h>
#include <netdb.h>
#include <arpa/inet.h>
// #include <netinet/in_systm.h>

#include <wulib/wunet.h>

typedef struct wuip_flow_t {
  uint32_t saddr, daddr;
  uint16_t sport, dport;
  uint16_t flags;
  uint8_t  proto;
#define WUIP_FLOW_TOS_DEF    0
  uint8_t  tos;
} wuip_flow_t;

#ifdef __CYGWIN__
#include <wulib/bsdnet.h>
#else
typedef struct icmp icmphdr_t;
typedef struct ip iphdr_t;
typedef struct udphdr udphdr_t;
typedef struct tcphdr tcphdr_t;
typedef struct ip_timestamp ip_tstamp_t;
#endif

typedef struct PsuedoHdr_t{
  uint32_t saddr;
  uint32_t daddr;
  uint8_t  pad;
  uint8_t  proto;
  uint16_t tlen;
} PsuedoHdr_t;

#define WUUSEPORTS(p) ((p) == IPPROTO_UDP || (p) == IPPROTO_TCP)
typedef struct wu_ip_tport {
  uint16_t sport;
  uint16_t dport;
} wu_ip_tport_t;
typedef struct wu_psuedoip_t {
  iphdr_t       iph;
  wu_ip_tport_t tph;
} wu_psuedoip_t;

/* Real Functions declarations */
#define WUIP_IPHDR_MAXSTR    255
char *wu_iph2str(char *str, int len, iphdr_t *iph);
char *wu_iph2strV2(char *str, int len, iphdr_t *iph);
char *wu_udp2str(char *str, int len, udphdr_t *udph);
int  wu_ip2num(char *str, uint32_t *addr);
char *wu_num2ip(uint32_t addr, char *str, int n);
void wu_udph_print(udphdr_t *udph);
void wu_iph_print(iphdr_t *iph);
void wu_icmp_print(iphdr_t *);

/* ***** Calculate TCP Checksup ***** */

/* ***** Access IP header fields, return local host byte ordering ***** */
static inline int      wu_iplen(iphdr_t *iph);
static inline int      wu_iphlen(iphdr_t *iph);
static inline uint32_t wu_ipdaddr(iphdr_t *iph);
static inline uint32_t wu_ipsaddr(iphdr_t *iph);
static inline int      wu_ipproto(iphdr_t *iph);
static inline int      wu_ipttl(iphdr_t *iph);
static inline uint16_t wu_ipdport(iphdr_t *iph);
static inline uint16_t wu_ipsport(iphdr_t *iph);

#define wu_iplen_hbo   wu_iplen
#define wu_ipdaddr_hbo wu_ipdaddr
#define wu_ipsaddr_hbo wu_ipsaddr
#define wu_ipdport_hbo wu_ipdport
#define wu_ipsport_hbo wu_ipsport

/* ***** Access IP header fields, return NBO ***** */
static inline uint16_t wu_get_dport(iphdr_t *iph);
static inline uint16_t wu_get_sport(iphdr_t *iph);

static inline int      wu_iplen_nbo(iphdr_t *iph);
static inline uint32_t wu_ipdaddr_nbo(iphdr_t *iph);
static inline uint32_t wu_ipsaddr_nbo(iphdr_t *iph);
static inline uint16_t wu_ipdport_nbo(iphdr_t *iph);
static inline uint16_t wu_ipsport_nbo(iphdr_t *iph);

/* --------------------------- inline'd functions --------------------------- */
static inline int assign_tcpCksums(iphdr_t *iph);
static inline int assign_udpCksums(iphdr_t *iph);
static inline uint16_t calc_cksums(iphdr_t *iph);
/* 
 *  Layout of an IP Packet:
 *  0 |V(4)|hl-4|  TOS-8 |   Dgram Len-16 |
 *  1 |      ID-16       |F-3| Offset-13  |
 *  2 | TTL-8   | Proto-8| Hdr cksum-16   |
 *  3 |        Source address-32          |
 *  4 |       Destintion address-32       |
 *       -- Options, Word aligned --
 * UDP
 *  0 |   Src Port-16    |   Dest Port-16 |
 *  1 |   Length-16      | Cksum-16       |
 *
 * TCP
 *  0 |   Src Port-16    |  Dest Port-16  |
 *  1 |      Sequence Number-32           |
 *  2 |         Ack Number-32             |
 *  3 |hl-4|res-6 |flgs-6|   Window-16    |
 *  4 |   Cksum-16       |  Urgent-16     |
 *          -- Options, TCP ---
 *
 * UDP and TCP calculate cksum over a prepended psuedo-header, their own
 * header with the cksum zero'ed and the payload. The data is padded to be
 * a multiple of 16-bit words
 *
 * Psuedo Hdr |    IP Source Address-32        |
 *            | IP Destination Address-32      |
 *            |00000000| Proto-8| Length-16    |
 *            |  -- UDP or TCP Header   --     |
 *            | < Payload >     |
 *
 * */
static inline int assign_tcpCksums(iphdr_t *iph)
{
  tcphdr_t *tcph;
  
  if (iph == NULL)
    return -1;
  
  tcph = (tcphdr_t *)((char *)iph + wu_iphlen(iph));
  tcph->th_sum = 0;
  tcph->th_sum = calc_cksums(iph);
  iph->ip_sum  = 0;
  iph->ip_sum  = wunet_cksum16((uint16_t *)iph, wu_iphlen(iph));
  return 0;
}

static inline int assign_udpCksums(iphdr_t *iph)
{
  udphdr_t *udph;
  
  if (iph == NULL)
    return -1;
  
  udph = (udphdr_t *)((char *)iph + wu_iphlen(iph));
  udph->uh_sum = 0;
  udph->uh_sum = calc_cksums(iph);
  if (udph->uh_sum == 0)
    udph->uh_sum = 0xffff;
  iph->ip_sum  = 0;
  iph->ip_sum  = wunet_cksum16((uint16_t *)iph, sizeof(iphdr_t));
  return 0;
}

/*
 * In order to keep this generic (non TCP or UDP specific)
 * the caller should zero out the transport headers checksum
 * field before calling.
 *
 * Example for tcp checksum:
 *   pkt->tcph.th_sum = 0;
 *   pkt->tcph.th_sum = do_cksums(pkt);
 * */
static inline uint16_t
calc_cksums(iphdr_t *iph)
{
  PsuedoHdr_t *phdr, origHdr;
  int cklen, tplen;
  uint8_t proto;
  uint16_t tpcksum;
  uint32_t saddr, daddr;

  if (iph == NULL)
    return 0;

  // The overlay (psuedo) header will overwrite the last 3 words
  // of the IP header, so cache them.
  proto = iph->ip_p;
  saddr = iph->ip_src.s_addr;
  daddr = iph->ip_dst.s_addr;
  tplen = wu_iplen(iph) - wu_iphlen(iph);
  cklen = tplen + sizeof(PsuedoHdr_t);

  /* Fill in the extended header, just in case there are options ... */
  phdr = (PsuedoHdr_t *)((char *)iph + wu_iphlen(iph) - sizeof(PsuedoHdr_t));
  origHdr.saddr = phdr->saddr; // actual IP: TTL, Proto, Checksum
  origHdr.daddr = phdr->daddr; // actual IP: Source addr
  origHdr.pad   = phdr->pad;   // acutal IP: MSB of dest addr
  origHdr.proto = phdr->proto; // actual IP: 2nd byte of dest addr
  origHdr.tlen  = phdr->tlen;  // actual IP: Last 2 Bytes of dest addr

  phdr->saddr = saddr;
  phdr->daddr = daddr;
  phdr->pad   = 0;
  phdr->proto = proto;
  phdr->tlen  = htons(tplen);

  /* do TCP/UDP checksum -- assume the appropriate cksum field has already
   * been zero'ed out */
  tpcksum = wunet_cksum16((uint16_t *)phdr, cklen);

  /* Now, overwrite extended header with the correct 4th and 5th words of the
   * IP header */
  phdr->saddr = origHdr.saddr;
  phdr->daddr = origHdr.daddr;
  phdr->pad   = origHdr.pad; // This is the TTL value
  phdr->proto = origHdr.proto;
  phdr->tlen  = origHdr.tlen;

  return tpcksum;
}

static inline uint16_t
wu_get_dport(iphdr_t *iph)
{
  struct wu_ip_tport *tp;
  uint16_t pn;
  if (iph->ip_p == IPPROTO_UDP || iph->ip_p == IPPROTO_TCP) {
    tp = (struct wu_ip_tport *)((char *)iph + wu_iphlen(iph));
    pn = tp->dport;
  } else {
    pn = 0;
  }
  return pn;
}

static inline uint16_t
wu_get_sport(iphdr_t *iph)
{
  struct wu_ip_tport *tp;
  uint16_t pn;
  if (iph->ip_p == IPPROTO_UDP || iph->ip_p == IPPROTO_TCP) {
    tp = (struct wu_ip_tport *)((char *)iph + wu_iphlen(iph));
    pn = tp->sport;
  } else {
    pn = 0;
  }
  return pn;
}

static inline int
wu_iplen_nbo(iphdr_t *iph)
{
  return (int)iph->ip_len;
}
static inline uint32_t 
wu_ipdaddr_nbo(iphdr_t *iph)
{
  return (uint32_t)iph->ip_dst.s_addr;
}
static inline uint32_t 
wu_ipsaddr_nbo(iphdr_t *iph)
{
  return (uint32_t)iph->ip_src.s_addr;
}
static inline uint16_t
wu_ipdport_nbo(iphdr_t *iph)
{
  struct wu_ip_tport *tp;
  tp = (struct wu_ip_tport *)((char *)iph + wu_iphlen(iph));
  return tp->dport;
}
static inline uint16_t
wu_ipsport_nbo(iphdr_t *iph)
{
  struct wu_ip_tport *tp;
  tp = (struct wu_ip_tport *)((char *)iph + wu_iphlen(iph));
  return tp->sport;
}

static inline int
wu_iplen(iphdr_t *iph)
{
  return (int)ntohs(iph->ip_len);
}
static inline int 
wu_iphlen(iphdr_t *iph)
{
  return (int)(iph->ip_hl << 2);
}
static inline int
wu_ipttl(iphdr_t *iph)
{
  return (int)(iph->ip_ttl);
}
static inline uint32_t 
wu_ipdaddr(iphdr_t *iph)
{
  return (uint32_t)ntohl(iph->ip_dst.s_addr);
}
static inline uint32_t 
wu_ipsaddr(iphdr_t *iph)
{
  return (uint32_t)ntohl(iph->ip_src.s_addr);
}
static inline uint16_t
wu_ipdport(iphdr_t *iph)
{
  struct wu_ip_tport *tp;
  tp = (struct wu_ip_tport *)((char *)iph + wu_iphlen(iph));
  return ntohs(tp->dport);
}
static inline uint16_t
wu_ipsport(iphdr_t *iph)
{
  struct wu_ip_tport *tp;
  tp = (struct wu_ip_tport *)((char *)iph + wu_iphlen(iph));
  return ntohs(tp->sport);
}
static inline int 
wu_ipproto(iphdr_t *iph)
{
  return (int)(iph->ip_p);
}

// transport
static inline int tcp_hlen(tcphdr_t *tcph);
static inline char *getPayload(iphdr_t *iph);

static inline int tcp_hlen(tcphdr_t *tcph)
{
  if (tcph == NULL)
    return 0;
  // multiply by for, th_off is the number of 4-byte words
  return ((int)tcph->th_off << 2);
}

static inline char *getPayload(iphdr_t *iph)
{
  int offset = wu_iphlen(iph);

  switch(wu_ipproto(iph)) {
    case IPPROTO_UDP:
      offset += sizeof(udphdr_t);
      break;
    case IPPROTO_TCP:
      offset += tcp_hlen((tcphdr_t *)((char *)iph + offset));
      break;
    default:
      offset = 0;
    }
  if (offset >= wu_iplen(iph))
    return NULL;
  return ((char *)iph + offset);
}

#ifdef __cplusplus
}
#endif
#endif /* _WUNET_H */
