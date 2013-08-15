/*
 * $Source: /project/arl/arlcvs/cvsroot/wu_arl/wusrc/wuPP/wusock.cc,v $
 * $Author: fredk $
 * $Date: 2008/02/22 16:38:51 $
 * $Revision: 1.10 $
 * $Name:  $
 *
 * File:         net.cc
 * Created:      01/05/2005 04:32:38 PM CST
 * Author:       Fred Kunhns, <fredk@cse.wustl.edu>
 * Organization: Applied Research Laboratory,
 *               Washington University in St. Louis
 *
 * Description:
 *
 */
#include <iostream>
#include <vector>
#include <sstream>

#include <cstdio>
#include <cstdlib>
#include <stdint.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <netdb.h>
#include <wulib/wulog.h>

#include <wuPP/net.h>
#include <wuPP/addr.h>
#include <wuPP/wusock.h>
#include <wuPP/timer.h>

wunet::sockType::sockType() {init(-1, -1, -1);}
wunet::sockType::sockType(sockType &oldsock) {init(oldsock.sock_);}
wunet::sockType::sockType(wusock_t &oldsock) {init(oldsock);}
wunet::sockType::sockType(int domain, int stype, int proto) {init(domain, stype, proto);}
wunet::sockType::sockType(int sd) {init(sd);}

wunet::sockType::~sockType() {clean();}

int wunet::sockType::rxselect(struct timeval rxtout)
{
  int status;
  fd_set rset;
  FD_ZERO(&rset);
  FD_SET(sd(), &rset);

  if ((status = select (sd()+1, &rset, NULL, NULL, &rxtout)) == -1)
    sok_cksockerr(&sock_, errno, "rxselect");

  check_state();
  return status;
}

void wunet::sockType::init(wusock_t &oldsock)
{
  sok_copy(&oldsock, &sock_);
  check_state();
}

void wunet::sockType::init(int domain, int stype, int proto)
{
  sok_init(&sock_, domain, stype, proto);
  check_state();
}

void wunet::sockType::init(int sd)
{
  sok_assign(&sock_, sd);
  check_state();
}

int  wunet::sockType::open()
{
  sok_open(&sock_);
  check_state();
  return sock_.sd_;
}

void wunet::sockType::close()
{
  sok_close(&sock_);
  check_state();
}

int wunet::sockType::bind(const struct sockaddr *saddr)
{
  int res = sok_bind(&sock_, saddr);
  check_state();
  return res;
}

int wunet::sockType::connect(const struct sockaddr *serv)
{
  int res = sok_connect(&sock_, serv);
  check_state();
  return res;
}

int wunet::sockType::listen()
{
  // XXX should verify that this is a connection oriented protocol
  int res = sok_listen(&sock_);
  check_state();
  return res;
}

wunet::sockType *wunet::sockType::clone()
{
  return new sockType(domain(), stype(), proto());
}

wunet::sockType *wunet::sockType::accept()
{
  sockType *newSock = clone();
  // XXX should verify that this is a connection oriented protocol
  sok_accept2(&sock_, &newSock->sock_);
  check_state();
  return newSock;
}

wunet::sockType &wunet::sockType::accept(sockType &newSock)
{
  // XXX should verify that this is a connection oriented protocol
  sok_accept2(&sock_, &newSock.sock_);
  check_state();
  return newSock;
}

void wunet::sockType::reuseaddr(int val) {sok_reuseaddr(&sock_, val); check_state();}
// TCP specific option!
void wunet::sockType::nodelay(int val) {sok_nodelay(&sock_, val); check_state();}
// on == true => turn on nonblocking
void wunet::sockType::nonblock(bool on) {sok_nonblock(&sock_, on); check_state();}
void wunet::sockType::recvbuf(int sz)   {sok_recvbuf(&sock_, sz); check_state();}
void wunet::sockType::sendbuf(int sz)   {sok_sendbuf(&sock_, sz); check_state();}

void wunet::sockType::set_rxtout(const struct timeval &tval) { sok_rxtout(&sock_, &tval); check_state(); }
void wunet::sockType::set_rxtout(unsigned int msec)          { struct timeval tval; wupp::msec2tval(msec, tval); set_rxtout(tval); }
void wunet::sockType::set_txtout(const struct timeval &tval) { sok_rxtout(&sock_, &tval); check_state(); }
void wunet::sockType::set_txtout(unsigned int msec)          { struct timeval tval; wupp::msec2tval(msec, tval); set_txtout(tval); }

