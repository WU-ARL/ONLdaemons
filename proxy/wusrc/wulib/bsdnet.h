/* 
 * $Source: /project/arl/arlcvs/cvsroot/wu_arl/wusrc/wulib/bsdnet.h,v $
 * $Author: fredk $
 * $Date: 2007/01/20 21:47:06 $
 * $Revision: 1.2 $
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

#ifndef _BSDIPNET_H
#define _BSDIPNET_H
#ifdef __cplusplus
extern "C" {
#endif

#include <inttypes.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netinet/tcp.h>
#include <netinet/udp.h>

#include <sys/param.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in_systm.h>

#ifndef __USE_BSD
/*
 * Copyright (c) 1982, 1986, 1993
 *      The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *      @(#)ip.h        8.2 (Berkeley) 6/1/94
 * $FreeBSD: src/sys/netinet/ip.h,v 1.17 1999/12/22 19:13:20 shin Exp $
 */

/* IP Header manipulation methods */
  typedef struct iphdr_t {
#  if __BYTE_ORDER == __LITTLE_ENDIAN
    unsigned int ip_hl:4;            /* header length */
    unsigned int ip_v:4;             /* version */
#  endif
#  if __BYTE_ORDER == __BIG_ENDIAN
    unsigned int ip_v:4;             /* version */
    unsigned int ip_hl:4;            /* header length */
#  endif
    uint8_t  ip_tos;                 /* type of service */
    uint16_t ip_len;                 /* total length */
    uint16_t ip_id;                  /* identification */
    uint16_t ip_off;                 /* fragment offset field */
#  define IP_RF 0x8000               /* reserved fragment flag */
#  define IP_DF 0x4000               /* dont fragment flag */
#  define IP_MF 0x2000               /* more fragments flag */
#  define IP_OFFMASK 0x1fff          /* mask for fragmenting bits */
    uint8_t   ip_ttl;                /* time to live */
    uint8_t   ip_p;                  /* protocol */
    uint16_t ip_sum;                 /* checksum */
    struct in_addr ip_src, ip_dst;   /* source and dest address */
  } iphdr_t;

  typedef struct ip_tstamp_t {
    uint8_t ipt_code;                /* IPOPT_TS */
    uint8_t ipt_len;                 /* size of structure (variable) */
    uint8_t ipt_ptr;                 /* index of current entry */
#  if __BYTE_ORDER == __LITTLE_ENDIAN
    unsigned int ipt_flg:4;          /* flags, see below */
    unsigned int ipt_oflw:4;         /* overflow counter */
#  endif
#  if __BYTE_ORDER == __BIG_ENDIAN
    unsigned int ipt_oflw:4;         /* overflow counter */
    unsigned int ipt_flg:4;          /* flags, see below */
#  endif
    uint32_t data[9];
  } ip_tstamp_t;

/*
 * Copyright (c) 1982, 1986, 1993
 *      The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *      @(#)ip_icmp.h   8.1 (Berkeley) 6/10/93
 */

  struct icmp_ra_addr
  {
    uint32_t ira_addr;
    uint32_t ira_preference;
  };

  typedef struct icmphdr_t {
    uint8_t  icmp_type;  /* type of message, see below */
    uint8_t  icmp_code;  /* type sub code */
    uint16_t icmp_cksum; /* ones complement checksum of struct */
    union {
      uint8_t ih_pptr;             /* ICMP_PARAMPROB */
      struct in_addr ih_gwaddr;   /* gateway address */
      struct ih_idseq {           /* echo datagram */
        uint16_t icd_id;
        uint16_t icd_seq;
      } ih_idseq;
      uint32_t ih_void;
      /* ICMP_UNREACH_NEEDFRAG -- Path MTU Discovery (RFC1191) */
      struct ih_pmtu {
        uint16_t ipm_void;
        uint16_t ipm_nextmtu;
      } ih_pmtu;
      struct ih_rtradv {
        uint8_t irt_num_addrs;
        uint8_t irt_wpa;
        uint16_t irt_lifetime;
      } ih_rtradv;
    } icmp_hun;
#  define icmp_pptr       icmp_hun.ih_pptr
#  define icmp_gwaddr     icmp_hun.ih_gwaddr
#  define icmp_id         icmp_hun.ih_idseq.icd_id
#  define icmp_seq        icmp_hun.ih_idseq.icd_seq
#  define icmp_void       icmp_hun.ih_void
#  define icmp_pmvoid     icmp_hun.ih_pmtu.ipm_void
#  define icmp_nextmtu    icmp_hun.ih_pmtu.ipm_nextmtu
#  define icmp_num_addrs  icmp_hun.ih_rtradv.irt_num_addrs
#  define icmp_wpa        icmp_hun.ih_rtradv.irt_wpa
#  define icmp_lifetime   icmp_hun.ih_rtradv.irt_lifetime
    union { 
      struct {
        uint32_t its_otime;
        uint32_t its_rtime;
        uint32_t its_ttime;
      } id_ts;
    struct {
      struct ip idi_ip;
      /* options and then 64 bits of data */
      } id_ip;
      struct icmp_ra_addr id_radv;
      uint32_t   id_mask;
      uint8_t    id_data[1];
    } icmp_dun;
#  define icmp_otime      icmp_dun.id_ts.its_otime
#  define icmp_rtime      icmp_dun.id_ts.its_rtime
#  define icmp_ttime      icmp_dun.id_ts.its_ttime
#  define icmp_ip         icmp_dun.id_ip.idi_ip
#  define icmp_radv       icmp_dun.id_radv
#  define icmp_mask       icmp_dun.id_mask
#  define icmp_data       icmp_dun.id_data
  } icmphdr_t;

#ifndef ICMP_MINLEN
#define ICMP_MINLEN     8
#endif
#endif /* ! __USE_BSD */


/*
 * Copyright (c) 1982, 1986, 1993
 *      The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the University of
 *      California, Berkeley and its contributors.
 * 4. Neither the name of the University nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 *
 *      @(#)tcp.h       8.1 (Berkeley) 6/10/93
 * $FreeBSD: src/sys/netinet/tcp.h,v 1.13 2000/01/09 19:17:25 shin Exp $
 */

/* Copied from tcp.h ... for compatibility with code written for both newer
 * systems and legacy NetBSD.
 */
#ifndef __FAVOR_BSD
  typedef struct tcphdr_t {
    uint16_t th_sport;    /* source port */
    uint16_t th_dport;    /* destination port */
    uint32_t th_seq;    /* sequence number */
    uint32_t th_ack;    /* acknowledgement number */
#  if __BYTE_ORDER == __LITTLE_ENDIAN
    uint8_t th_x2:4;    /* (unused) */
    uint8_t th_off:4;    /* data offset */
#  endif
#  if __BYTE_ORDER == __BIG_ENDIAN
    uint8_t th_off:4;    /* data offset */
    uint8_t th_x2:4;    /* (unused) */
#  endif
    uint8_t th_flags;
#  define TH_FIN  0x01
#  define TH_SYN  0x02
#  define TH_RST  0x04
#  define TH_PUSH  0x08
#  define TH_ACK  0x10
#  define TH_URG  0x20
    uint16_t th_win;    /* window */
    uint16_t th_sum;    /* checksum */
    uint16_t th_urp;    /* urgent pointer */
  } tcphdr_t;
  typedef struct udphdr_t {
    uint16_t uh_sport;           /* source port */
    uint16_t uh_dport;           /* destination port */
    uint16_t uh_ulen;            /* udp length */
    uint16_t uh_sum;             /* udp checksum */
  } udphdr_t;
#endif /* ! __FAVOR_BSD */

#ifndef IP_MAXPACKET
#  define IP_MAXPACKET                     65535
#endif

#ifndef IPTOS_TOS_MASK
#  define IPTOS_TOS_MASK                  0x1E
#endif
#ifndef IPTOS_LOWDELAY
/* assume none are defined */
#  define IPTOS_LOWDELAY                  0x10
#  define IPTOS_THROUGHPUT                0x08
#  define IPTOS_RELIABILITY               0x04
#  define IPTOS_LOWCOST                   0x02
#  define IPTOS_MINCOST           IPTOS_LOWCOST
#endif
#ifndef IPTOS_CE
#  define IPTOS_CE                        0x01    /* congestion experienced */
#  define IPTOS_ECT                       0x02    /* ECN-capable transport */
#endif

#ifndef IPTOS_PREC_MASK
#  define IPTOS_PREC_MASK                 0xe0
#endif
#ifndef IPTOS_PREC_NETCONTROL
#  define IPTOS_PREC_NETCONTROL           0xe0
#  define IPTOS_PREC_INTERNETCONTROL      0xc0
#  define IPTOS_PREC_CRITIC_ECP           0xa0
#  define IPTOS_PREC_FLASHOVERRIDE        0x80
#  define IPTOS_PREC_FLASH                0x60
#  define IPTOS_PREC_IMMEDIATE            0x40
#  define IPTOS_PREC_PRIORITY             0x20
#  define IPTOS_PREC_ROUTINE              0x00
#endif

#ifndef ICMP_ECHOREPLY
#  define ICMP_ECHOREPLY          0       /* Echo Reply                   */
#  define ICMP_DEST_UNREACH       3       /* Destination Unreachable      */
#  define ICMP_SOURCE_QUENCH      4       /* Source Quench                */
#  define ICMP_REDIRECT           5       /* Redirect (change route)      */
#  define ICMP_ECHO               8       /* Echo Request                 */
#  define ICMP_TIME_EXCEEDED      11      /* Time Exceeded                */
#  define ICMP_PARAMETERPROB      12      /* Parameter Problem            */
#  define ICMP_TIMESTAMP          13      /* Timestamp Request            */
#  define ICMP_TIMESTAMPREPLY     14      /* Timestamp Reply              */
#  define ICMP_INFO_REQUEST       15      /* Information Request          */
#  define ICMP_INFO_REPLY         16      /* Information Reply            */
#  define ICMP_ADDRESS            17      /* Address Mask Request         */
#  define ICMP_ADDRESSREPLY       18      /* Address Mask Reply           */
#  define NR_ICMP_TYPES           18


/* Codes for UNREACH. */
#  define ICMP_NET_UNREACH        0       /* Network Unreachable          */
#  define ICMP_HOST_UNREACH       1       /* Host Unreachable             */
#  define ICMP_PROT_UNREACH       2       /* Protocol Unreachable         */
#  define ICMP_PORT_UNREACH       3       /* Port Unreachable             */
#  define ICMP_FRAG_NEEDED        4       /* Fragmentation Needed/DF set  */
#  define ICMP_SR_FAILED          5       /* Source Route failed          */
#  define ICMP_NET_UNKNOWN        6
#  define ICMP_HOST_UNKNOWN       7
#  define ICMP_HOST_ISOLATED      8
#  define ICMP_NET_ANO            9
#  define ICMP_HOST_ANO           10
#  define ICMP_NET_UNR_TOS        11
#  define ICMP_HOST_UNR_TOS       12
#  define ICMP_PKT_FILTERED       13      /* Packet filtered */
#  define ICMP_PREC_VIOLATION     14      /* Precedence violation */
#  define ICMP_PREC_CUTOFF        15      /* Precedence cut off */
#  define NR_ICMP_UNREACH         15      /* instead of hardcoding immediate value */

/* Codes for REDIRECT. */
#  define ICMP_REDIR_NET          0       /* Redirect Net                 */
#  define ICMP_REDIR_HOST         1       /* Redirect Host                */
#  define ICMP_REDIR_NETTOS       2       /* Redirect Net for TOS         */
#  define ICMP_REDIR_HOSTTOS      3       /* Redirect Host for TOS        */

/* Codes for TIME_EXCEEDED. */
#  define ICMP_EXC_TTL            0       /* TTL count exceeded           */
#  define ICMP_EXC_FRAGTIME       1       /* Fragment Reass time exceeded */
#endif

#  ifndef ICMP_UNREACH
/* Definition of type and code fields. */
/* defined above: ICMP_ECHOREPLY, ICMP_REDIRECT, ICMP_ECHO */
#    define ICMP_UNREACH            3               /* dest unreachable, codes: */
#    define ICMP_SOURCEQUENCH       4               /* packet lost, slow down */
#    define ICMP_ROUTERADVERT       9               /* router advertisement */
#    define ICMP_ROUTERSOLICIT      10              /* router solicitation */
#    define ICMP_TIMXCEED           11              /* time exceeded, code: */
#    define ICMP_PARAMPROB          12              /* ip header bad */
#    define ICMP_TSTAMP             13              /* timestamp request */
#    define ICMP_TSTAMPREPLY        14              /* timestamp reply */
#    define ICMP_IREQ               15              /* information request */
#    define ICMP_IREQREPLY          16              /* information reply */
#    define ICMP_MASKREQ            17              /* address mask request */
#    define ICMP_MASKREPLY          18              /* address mask reply */

#    define ICMP_MAXTYPE            18

/* UNREACH codes */
#    define ICMP_UNREACH_NET                0       /* bad net */
#    define ICMP_UNREACH_HOST               1       /* bad host */
#    define ICMP_UNREACH_PROTOCOL           2       /* bad protocol */
#    define ICMP_UNREACH_PORT               3       /* bad port */
#    define ICMP_UNREACH_NEEDFRAG           4       /* IP_DF caused drop */
#    define ICMP_UNREACH_SRCFAIL            5       /* src route failed */
#    define ICMP_UNREACH_NET_UNKNOWN        6       /* unknown net */
#    define ICMP_UNREACH_HOST_UNKNOWN       7       /* unknown host */
#    define ICMP_UNREACH_ISOLATED           8       /* src host isolated */
#    define ICMP_UNREACH_NET_PROHIB         9       /* net denied */
#    define ICMP_UNREACH_HOST_PROHIB        10      /* host denied */
#    define ICMP_UNREACH_TOSNET             11      /* bad tos for net */
#    define ICMP_UNREACH_TOSHOST            12      /* bad tos for host */
#    define ICMP_UNREACH_FILTER_PROHIB      13      /* admin prohib */
#    define ICMP_UNREACH_HOST_PRECEDENCE    14      /* host prec vio. */
#    define ICMP_UNREACH_PRECEDENCE_CUTOFF  15      /* prec cutoff */

/* REDIRECT codes */
#    define ICMP_REDIRECT_NET       0               /* for network */
#    define ICMP_REDIRECT_HOST      1               /* for host */
#    define ICMP_REDIRECT_TOSNET    2               /* for tos and net */
#    define ICMP_REDIRECT_TOSHOST   3               /* for tos and host */

/* TIMEXCEED codes */
#    define ICMP_TIMXCEED_INTRANS   0               /* ttl==0 in transit */
#    define ICMP_TIMXCEED_REASS     1               /* ttl==0 in reass */

/* PARAMPROB code */
#    define ICMP_PARAMPROB_OPTABSENT 1              /* req. opt. absent */

#    define ICMP_INFOTYPE(type) \
          ((type) == ICMP_ECHOREPLY || (type) == ICMP_ECHO || \
                   (type) == ICMP_ROUTERADVERT || (type) == ICMP_ROUTERSOLICIT || \
                   (type) == ICMP_TSTAMP || (type) == ICMP_TSTAMPREPLY || \
                   (type) == ICMP_IREQ || (type) == ICMP_IREQREPLY || \
                   (type) == ICMP_MASKREQ || (type) == ICMP_MASKREPLY)

#  endif /* ! ICMP_UNREACH */

#ifdef __cplusplus
}
#endif
#endif /* _WUNET_H */
