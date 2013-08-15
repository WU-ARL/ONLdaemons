/*
 * $Author: fredk $
 * $Date: 2007/01/17 02:00:46 $
 * $Revision: 1.14 $
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
#include <sys/param.h>
#include <sys/types.h>
#include <ctype.h>
#include <string.h>

#include <wulib/ipnet.h>
#include <wulib/valTypes.h>

char *
wu_iph2str(char *str, int len, iphdr_t *iph)
{
  snprintf(str, len, 
        "Src 0x%08x/%d, Dst 0x%08x/%d, Proto 0x%x, Plen %d, Hlen %d V %u, TOS %u, ID %u, OFF %u:%u, TTL %u, SUM %u",
        (unsigned int)wu_ipsaddr(iph), (int)wu_ipsport(iph),
        (unsigned int)wu_ipdaddr(iph), (int)wu_ipdport(iph),
        wu_ipproto(iph), wu_iplen(iph), wu_iphlen(iph),
        (unsigned int)iph->ip_v, (unsigned int)iph->ip_tos,
        (unsigned int)ntohs(iph->ip_id), 
        (unsigned int)(ntohs(iph->ip_off) & IP_OFFMASK),
        (unsigned int)(ntohs(iph->ip_off) & (IP_RF | IP_DF | IP_MF)),
        (unsigned int)iph->ip_ttl, (unsigned int)iph->ip_sum);
  str[len-1] = '\0';
  return str;
}

char *
wu_iph2strV2(char *str, int len, iphdr_t *iph)
{
  snprintf(str, len,
        "Src 0x%08x, Dst 0x%08x, Proto 0x%x, Plen %d, Hlen %d, Ver %u, TOS %u, ID %u, Off %u:%u, TTL %u, CSum %u",
        (unsigned int)wu_ipsaddr(iph), (unsigned int)wu_ipdaddr(iph),
        wu_ipproto(iph), wu_iplen(iph), wu_iphlen(iph),
        (unsigned int)iph->ip_v, (unsigned int)iph->ip_tos,
        (unsigned int)ntohs(iph->ip_id),
        (unsigned int)(ntohs(iph->ip_off) & IP_OFFMASK),
        (unsigned int)(ntohs(iph->ip_off) & (IP_RF | IP_DF | IP_MF)),
        (unsigned int)iph->ip_ttl,
        (unsigned int)ntohs(iph->ip_sum));
  str[len-1] = '\0';
  return str;
}

void wu_iph_print(iphdr_t *iph)
{
  char sip[MaxIPStringLen], dip[MaxIPStringLen];
  if (iph == NULL) return;
  sip[0] = dip[0] = '\0';

  printf("iph: ver %x hlen %d tos %x tlen %d id %x flags %x foff %d ttl %d pr %d cksum %d"
         "\n  saddr 0x%08x (%s) daddr 0x%08x (%s)\n",
         (unsigned int)iph->ip_v,
         wu_iphlen(iph),
         (unsigned int)iph->ip_tos,
         wu_iplen(iph),
         (unsigned int)ntohs(iph->ip_id),
         (unsigned int)(ntohs(iph->ip_off) & (IP_RF | IP_DF | IP_MF)),
         (unsigned int)(ntohs(iph->ip_off) & IP_OFFMASK),
         (unsigned int)iph->ip_ttl,
         wu_ipproto(iph),
         (unsigned int)ntohs(iph->ip_sum),
         (unsigned int)wu_ipsaddr(iph), wu_num2ip(wu_ipsaddr_nbo(iph), sip, MaxIPStringLen),
         (unsigned int)wu_ipdaddr(iph), wu_num2ip(wu_ipdaddr_nbo(iph), dip, MaxIPStringLen));
  return;
}

void wu_udph_print(udphdr_t *udph)
{
  if (udph == NULL) return;
  printf("udph: sport %d dport %d dlen %d cksum %d\n",
      (int)ntohs(udph->uh_sport),
      (int)ntohs(udph->uh_dport),
      (int)ntohs(udph->uh_ulen),
      (int)ntohs(udph->uh_sum));

  return;
}

void wu_icmp_print(iphdr_t *iph)
{
  int type, code;
  char *data;
  size_t dlen;
  icmphdr_t *ich;

  if (iph == NULL) return;
  dlen = wu_iplen(iph) - wu_iphlen(iph);
  if (dlen < ICMP_MINLEN) return;

  ich = (icmphdr_t *)((char *)iph + wu_iphlen(iph));
  type = (int)ich->icmp_type;
  code = (int)ich->icmp_code;
  printf("icmp: type %d, code %d, cksum %d -- Len %d\n",
         type, code, (int)ntohs(ich->icmp_cksum), dlen);

  data = (char *)ich + ICMP_MINLEN;
  dlen = dlen - ICMP_MINLEN;
  switch((int)(int)ich->icmp_type) {
    case ICMP_ECHOREPLY:     /*  0 */
      printf("  Echo Reply: Id %d, Seq %d\n", (int)ntohs(ich->icmp_id), (int)ntohs(ich->icmp_seq));
      val_print_data(data, dw1Type, dlen);
      break;
    case ICMP_UNREACH:       /*  3 */
      printf("  Unreachable: \n");
      break;
    case ICMP_SOURCEQUENCH:  /*  4 */
      printf("  ICMP_SOURCEQUENCH:\n");
      break;
    case ICMP_REDIRECT:      /*  5 */
      printf("  ICMP_REDIRECT:\n");
      break;
    case ICMP_ECHO:          /*  8 */
      printf("  Echo: Id %d, Seq %d\n", (int)ntohs(ich->icmp_id), (int)ntohs(ich->icmp_seq));
      val_print_data(data, dw1Type, dlen);
      break;
    case ICMP_ROUTERADVERT:  /*  9 */
    printf("  ICMP_ROUTERADVERT:\n");
      break;
    case ICMP_ROUTERSOLICIT: /* 10 */
      printf("  ICMP_ROUTERSOLICIT:\n");
      break;
    case ICMP_TIMXCEED:      /* 11 */
      printf("  ICMP_TIMXCEED:\n");
      {
        iphdr_t *iph = (iphdr_t *)data;
        if (dlen >= sizeof(iphdr_t) && dlen > (size_t)wu_iphlen(iph)) {
          printf(" Offending packet:\n");
          wu_iph_print((iphdr_t *)data);
        }
      }
      val_print_data(data, dw1Type, dlen);
      break;
    case ICMP_PARAMPROB:     /* 12 */
      printf("  ICMP_PARAMPROB:\n");
      break;
    case ICMP_TSTAMP:        /* 13 */
      printf("  ICMP_TSTAMP:\n");
      break;
    case ICMP_TSTAMPREPLY:   /* 14 */
      printf("  ICMP_TSTAMPREPLY:\n");
      break;
    case ICMP_IREQ:          /* 15 */
      printf("  ICMP_IREQ:\n");
      break;
    case ICMP_IREQREPLY:     /* 16 */
      printf("  ICMP_IREQREPLY:\n");
      break;
    case ICMP_MASKREQ:       /* 17 */
      printf("  ICMP_MASKREQ:\n");
      break;
    case ICMP_MASKREPLY:     /* 18 */
      printf("  ICMP_MASKREPLY:\n");
      break;
    default:
      printf("  Invalid type %d\n", type);
      break;
  }

  return;
}

char *
wu_udp2str(char *str, int len, udphdr_t *udph)
{
  snprintf(str, len, 
      "SPort %x (%d), DPort %x (%d), DLen %d, CSum %x (%d)",
      (unsigned int)ntohs(udph->uh_sport), (int)ntohs(udph->uh_sport),
      (unsigned int)ntohs(udph->uh_dport), (int)ntohs(udph->uh_dport),
      (int)ntohs(udph->uh_ulen),
      (unsigned int)ntohs(udph->uh_sum), (int)ntohs(udph->uh_sum));
  str[len-1] = '\0';
  return str;
}

int
wu_ip2num(char *str, uint32_t *addr)
{
  uint32_t ipa;
  uint8_t  bytes[4];
  int i, j;

  if (str == NULL || addr == NULL)
    return -1;

  while (isspace(*((unsigned char *)str))) str++;

  for (j = 0; j < 4; j++) {
    for (i = 0;str[i] != '\0' && str[i] != '.'; i++);
    if (str[i] == '\0' && j != 3) {
      return -1;
    }
    str[i] = '\0';
#if defined(linux) && defined (__KERNEL__)
    bytes[j] = (uint8_t)simple_strtol(str, NULL, 0);
#else
    bytes[j] = (uint8_t)strtol(str, NULL, 0);
#endif
    str = str + i + 1;
  }

  /* put in local byte ordering */
  ipa = ((uint32_t)bytes[0] << 24) |
        ((uint32_t)bytes[1] << 16) |
        ((uint32_t)bytes[2] <<  8) |
        ((uint32_t)bytes[3]);
  *addr = htonl(ipa);

  return 0;
}

char *
wu_num2ip(uint32_t addr, char *str, int n)
{
  uint32_t ipa;

  if (addr == 0 || str == NULL)
    return NULL;

  ipa = ntohl(addr); /* I know what the local byte ordering is */
  snprintf(str, n, "%d.%d.%d.%d", 
           (int)((ipa >> 24) & 0x0ff), /* MSB */
           (int)((ipa >> 16) & 0x0ff),
           (int)((ipa >>  8) & 0x0ff),
           (int)(ipa & 0x0ff));        /* LSB */

  return str;
}
