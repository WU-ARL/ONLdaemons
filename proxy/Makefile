# Copyright (c) 2009-2013 Fred Kuhns, Charlie Wiseman, Jyoti Parwatikar, John DeHart 
# and Washington University in St. Louis
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
#    limitations under the License.
#
#
# $Author: cgw1 $
# $Date: 2007/03/23 21:05:04 $
# $Revision: 1.14 $
#
# Author:       Fred Kuhns
#               fredk@arl.wustl.edu
# Organization: Applied Research Laboratory
#               Washington UNiversity in St. Louis
#
WUSRC = ${WUARL}/wusrc
include ${WUSRC}/Makefile.inc

# Thread and Synchronization specific flags, for us it
# # will always be PTHREADS
THRFLAGS := -DPTHREADS

CXX      = ${WUCXX}
## CXXFLAGS = ${WUCXXFLAGS_OP} 
CXXFLAGS = ${WUCXXFLAGS_DB} 
INCLUDES = ${WUINCLUDES}
LIBS     = $(WUCXXLIBS) $(WUCLIBS)


LIBS+= -lpthread

HFILES := basicConn.h streamConn.h cmd.h nccp.h session.h proxy.h control.h
CSRC   := serv.cc onlProxy.cc testFwd.cc pcmd.cc kcmd.cc pingProxy.cc
#CSRC   := serv.cc onlProxy.cc pcmd.cc kcmd.cc
SRCS   := basicConn.cc streamConn.cc cmd.cc session.cc proxy.cc control.cc
SOBJS=$(addprefix ${OBJDIR}/, $(patsubst %.cc,%.o,$(SRCS)))
COBJS=$(addprefix ${OBJDIR}/, $(patsubst %.cc,%.o,$(CSRC)))
CMDS=$(addprefix ${OBJDIR}/, $(basename $(CSRC)))
OBJS = $(SOBJS) $(COBJS)
#CFLAGS+= -DDEBUG

ifeq ($(MyOS), SunOS)
LIBS+= -lrt
endif

all: $(OBJDIR) $(CMDS) $(OBJS)

ixp : 
	${MAKE}  OBJDIR="${IXP_TARGET}" PREFIX="${IXP_TARGET}-" USRINC= USRFLG= -e all

$(OBJDIR) :
	mkdir $(OBJDIR)

$(SOBJS) : ${OBJDIR}/%.o : %.cc $(HFILES) Makefile
	${CXX} ${CXXFLAGS} ${INCLUDES} -o $@ -c $<

$(COBJS) : ${OBJDIR}/%.o : %.cc $(HFILES) Makefile
	${CXX} ${CXXFLAGS} ${INCLUDES} -o $@ -c $<

$(CMDS) : ${OBJDIR}/% : $(OBJS)
	${CXX} ${CXXFLAGS} -o $@ $@.o ${SOBJS} ${LIBS}
	chmod 0755 $@

clean:
	/bin/rm -f $(OBJDIR)/*

realclean:
	/bin/rm -f *~
	/bin/rm -R $(OBJDIR)
	/bin/rm -R $(CLASSDIR)

depend:
	makedepend -- $(CXXFLAGS) $(INCLUDES) -- $(SRCS) $(CSRC)

# DO NOT DELETE

basicConn.o: /usr/include/stdint.h /usr/include/features.h
basicConn.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
basicConn.o: /usr/include/bits/wchar.h /usr/include/bits/wordsize.h
basicConn.o: /usr/include/sys/select.h /usr/include/bits/types.h
basicConn.o: /usr/lib/gcc/i386-redhat-linux/3.4.5/include/stddef.h
basicConn.o: /usr/include/bits/typesizes.h /usr/include/bits/select.h
basicConn.o: /usr/include/bits/sigset.h /usr/include/time.h
basicConn.o: /usr/include/bits/time.h /usr/include/sys/time.h
basicConn.o: /usr/include/sys/types.h /usr/include/endian.h
basicConn.o: /usr/include/bits/endian.h /usr/include/sys/sysmacros.h
basicConn.o: /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h
basicConn.o: /usr/include/unistd.h /usr/include/bits/posix_opt.h
basicConn.o: /usr/include/bits/environments.h /usr/include/bits/confname.h
basicConn.o: /usr/include/getopt.h /usr/include/sys/uio.h
basicConn.o: /usr/include/bits/uio.h /usr/include/sys/socket.h
basicConn.o: /usr/include/bits/socket.h /usr/include/limits.h
basicConn.o: /usr/lib/gcc/i386-redhat-linux/3.4.5/include/limits.h
basicConn.o: /usr/include/bits/posix1_lim.h /usr/include/bits/local_lim.h
basicConn.o: /usr/include/linux/limits.h /usr/include/bits/posix2_lim.h
basicConn.o: /usr/include/bits/xopen_lim.h /usr/include/bits/stdio_lim.h
basicConn.o: /usr/include/bits/sockaddr.h /usr/include/asm/socket.h
basicConn.o: /usr/include/asm/sockios.h /usr/include/arpa/inet.h
basicConn.o: /usr/include/netinet/in.h /usr/include/bits/in.h
basicConn.o: /usr/include/bits/byteswap.h /usr/include/netdb.h
basicConn.o: /usr/include/rpc/netdb.h /usr/include/bits/siginfo.h
basicConn.o: /usr/include/bits/netdb.h /usr/include/pthread.h
basicConn.o: /usr/include/sched.h /usr/include/signal.h
basicConn.o: /usr/include/bits/initspin.h /usr/include/bits/sigthread.h
basicConn.o: /home/arl/student/cgw1/wu_arl/wusrc/wulib/wulog.h
basicConn.o: /usr/include/string.h /usr/include/xlocale.h
basicConn.o: /home/arl/student/cgw1/wu_arl/wusrc/wuPP/net.h
basicConn.o: /home/arl/student/cgw1/wu_arl/wusrc/wuPP/error.h
basicConn.o: /usr/include/errno.h /usr/include/bits/errno.h
basicConn.o: /usr/include/linux/errno.h /usr/include/asm/errno.h
basicConn.o: /home/arl/student/cgw1/wu_arl/wusrc/wulib/wunet.h
basicConn.o: /usr/include/sys/param.h /usr/include/linux/param.h
basicConn.o: /usr/include/asm/param.h
basicConn.o: /home/arl/student/cgw1/wu_arl/wusrc/wuPP/addr.h
basicConn.o: /home/arl/student/cgw1/wu_arl/wusrc/wuPP/sock.h
basicConn.o: /home/arl/student/cgw1/wu_arl/wusrc/wuPP/buf.h
basicConn.o: /home/arl/student/cgw1/wu_arl/wusrc/wuPP/msg.h control.h cmd.h
basicConn.o: nccp.h basicConn.h
streamConn.o: /usr/include/stdint.h /usr/include/features.h
streamConn.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
streamConn.o: /usr/include/bits/wchar.h /usr/include/bits/wordsize.h
streamConn.o: /usr/include/sys/select.h /usr/include/bits/types.h
streamConn.o: /usr/lib/gcc/i386-redhat-linux/3.4.5/include/stddef.h
streamConn.o: /usr/include/bits/typesizes.h /usr/include/bits/select.h
streamConn.o: /usr/include/bits/sigset.h /usr/include/time.h
streamConn.o: /usr/include/bits/time.h /usr/include/sys/time.h
streamConn.o: /usr/include/sys/types.h /usr/include/endian.h
streamConn.o: /usr/include/bits/endian.h /usr/include/sys/sysmacros.h
streamConn.o: /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h
streamConn.o: /usr/include/unistd.h /usr/include/bits/posix_opt.h
streamConn.o: /usr/include/bits/environments.h /usr/include/bits/confname.h
streamConn.o: /usr/include/getopt.h /usr/include/sys/uio.h
streamConn.o: /usr/include/bits/uio.h /usr/include/sys/socket.h
streamConn.o: /usr/include/bits/socket.h /usr/include/limits.h
streamConn.o: /usr/lib/gcc/i386-redhat-linux/3.4.5/include/limits.h
streamConn.o: /usr/include/bits/posix1_lim.h /usr/include/bits/local_lim.h
streamConn.o: /usr/include/linux/limits.h /usr/include/bits/posix2_lim.h
streamConn.o: /usr/include/bits/xopen_lim.h /usr/include/bits/stdio_lim.h
streamConn.o: /usr/include/bits/sockaddr.h /usr/include/asm/socket.h
streamConn.o: /usr/include/asm/sockios.h /usr/include/arpa/inet.h
streamConn.o: /usr/include/netinet/in.h /usr/include/bits/in.h
streamConn.o: /usr/include/bits/byteswap.h /usr/include/netdb.h
streamConn.o: /usr/include/rpc/netdb.h /usr/include/bits/siginfo.h
streamConn.o: /usr/include/bits/netdb.h /usr/include/pthread.h
streamConn.o: /usr/include/sched.h /usr/include/signal.h
streamConn.o: /usr/include/bits/initspin.h /usr/include/bits/sigthread.h
streamConn.o: /home/arl/student/cgw1/wu_arl/wusrc/wulib/wulog.h
streamConn.o: /usr/include/string.h /usr/include/xlocale.h
streamConn.o: /home/arl/student/cgw1/wu_arl/wusrc/wuPP/net.h
streamConn.o: /home/arl/student/cgw1/wu_arl/wusrc/wuPP/error.h
streamConn.o: /usr/include/errno.h /usr/include/bits/errno.h
streamConn.o: /usr/include/linux/errno.h /usr/include/asm/errno.h
streamConn.o: /home/arl/student/cgw1/wu_arl/wusrc/wulib/wunet.h
streamConn.o: /usr/include/sys/param.h /usr/include/linux/param.h
streamConn.o: /usr/include/asm/param.h
streamConn.o: /home/arl/student/cgw1/wu_arl/wusrc/wuPP/addr.h
streamConn.o: /home/arl/student/cgw1/wu_arl/wusrc/wuPP/sock.h
streamConn.o: /home/arl/student/cgw1/wu_arl/wusrc/wuPP/buf.h
streamConn.o: /home/arl/student/cgw1/wu_arl/wusrc/wuPP/msg.h control.h cmd.h
streamConn.o: nccp.h basicConn.h streamConn.h session.h
streamConn.o: /home/arl/student/cgw1/wu_arl/wusrc/wuPP/util.h proxy.h
cmd.o: /usr/include/stdint.h /usr/include/features.h /usr/include/sys/cdefs.h
cmd.o: /usr/include/gnu/stubs.h /usr/include/bits/wchar.h
cmd.o: /usr/include/bits/wordsize.h /usr/include/unistd.h
cmd.o: /usr/include/bits/posix_opt.h /usr/include/bits/environments.h
cmd.o: /usr/include/bits/types.h
cmd.o: /usr/lib/gcc/i386-redhat-linux/3.4.5/include/stddef.h
cmd.o: /usr/include/bits/typesizes.h /usr/include/bits/confname.h
cmd.o: /usr/include/getopt.h /usr/include/sys/types.h /usr/include/time.h
cmd.o: /usr/include/endian.h /usr/include/bits/endian.h
cmd.o: /usr/include/sys/select.h /usr/include/bits/select.h
cmd.o: /usr/include/bits/sigset.h /usr/include/bits/time.h
cmd.o: /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h
cmd.o: /usr/include/bits/sched.h /usr/include/sys/socket.h
cmd.o: /usr/include/sys/uio.h /usr/include/bits/uio.h
cmd.o: /usr/include/bits/socket.h /usr/include/limits.h
cmd.o: /usr/lib/gcc/i386-redhat-linux/3.4.5/include/limits.h
cmd.o: /usr/include/bits/posix1_lim.h /usr/include/bits/local_lim.h
cmd.o: /usr/include/linux/limits.h /usr/include/bits/posix2_lim.h
cmd.o: /usr/include/bits/xopen_lim.h /usr/include/bits/stdio_lim.h
cmd.o: /usr/include/bits/sockaddr.h /usr/include/asm/socket.h
cmd.o: /usr/include/asm/sockios.h /usr/include/arpa/inet.h
cmd.o: /usr/include/netinet/in.h /usr/include/bits/in.h
cmd.o: /usr/include/bits/byteswap.h /usr/include/netdb.h
cmd.o: /usr/include/rpc/netdb.h /usr/include/bits/siginfo.h
cmd.o: /usr/include/bits/netdb.h
cmd.o: /home/arl/student/cgw1/wu_arl/wusrc/wuPP/net.h
cmd.o: /home/arl/student/cgw1/wu_arl/wusrc/wuPP/error.h /usr/include/errno.h
cmd.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
cmd.o: /usr/include/asm/errno.h
cmd.o: /home/arl/student/cgw1/wu_arl/wusrc/wulib/wulog.h
cmd.o: /usr/include/string.h /usr/include/xlocale.h
cmd.o: /home/arl/student/cgw1/wu_arl/wusrc/wulib/wunet.h
cmd.o: /usr/include/sys/param.h /usr/include/linux/param.h
cmd.o: /usr/include/asm/param.h
cmd.o: /home/arl/student/cgw1/wu_arl/wusrc/wuPP/buf.h
cmd.o: /home/arl/student/cgw1/wu_arl/wusrc/wuPP/msg.h
cmd.o: /home/arl/student/cgw1/wu_arl/wusrc/wuPP/addr.h
cmd.o: /home/arl/student/cgw1/wu_arl/wusrc/wuPP/sock.h control.h cmd.h
session.o: /usr/include/stdint.h /usr/include/features.h
session.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
session.o: /usr/include/bits/wchar.h /usr/include/bits/wordsize.h
session.o: /usr/include/sys/select.h /usr/include/bits/types.h
session.o: /usr/lib/gcc/i386-redhat-linux/3.4.5/include/stddef.h
session.o: /usr/include/bits/typesizes.h /usr/include/bits/select.h
session.o: /usr/include/bits/sigset.h /usr/include/time.h
session.o: /usr/include/bits/time.h /usr/include/sys/time.h
session.o: /usr/include/sys/types.h /usr/include/endian.h
session.o: /usr/include/bits/endian.h /usr/include/sys/sysmacros.h
session.o: /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h
session.o: /usr/include/unistd.h /usr/include/bits/posix_opt.h
session.o: /usr/include/bits/environments.h /usr/include/bits/confname.h
session.o: /usr/include/getopt.h /usr/include/sys/uio.h
session.o: /usr/include/bits/uio.h /usr/include/sys/socket.h
session.o: /usr/include/bits/socket.h /usr/include/limits.h
session.o: /usr/lib/gcc/i386-redhat-linux/3.4.5/include/limits.h
session.o: /usr/include/bits/posix1_lim.h /usr/include/bits/local_lim.h
session.o: /usr/include/linux/limits.h /usr/include/bits/posix2_lim.h
session.o: /usr/include/bits/xopen_lim.h /usr/include/bits/stdio_lim.h
session.o: /usr/include/bits/sockaddr.h /usr/include/asm/socket.h
session.o: /usr/include/asm/sockios.h /usr/include/arpa/inet.h
session.o: /usr/include/netinet/in.h /usr/include/bits/in.h
session.o: /usr/include/bits/byteswap.h /usr/include/netdb.h
session.o: /usr/include/rpc/netdb.h /usr/include/bits/siginfo.h
session.o: /usr/include/bits/netdb.h /usr/include/pthread.h
session.o: /usr/include/sched.h /usr/include/signal.h
session.o: /usr/include/bits/initspin.h /usr/include/bits/sigthread.h
session.o: /home/arl/student/cgw1/wu_arl/wusrc/wulib/wulog.h
session.o: /usr/include/string.h /usr/include/xlocale.h
session.o: /home/arl/student/cgw1/wu_arl/wusrc/wuPP/net.h
session.o: /home/arl/student/cgw1/wu_arl/wusrc/wuPP/error.h
session.o: /usr/include/errno.h /usr/include/bits/errno.h
session.o: /usr/include/linux/errno.h /usr/include/asm/errno.h
session.o: /home/arl/student/cgw1/wu_arl/wusrc/wulib/wunet.h
session.o: /usr/include/sys/param.h /usr/include/linux/param.h
session.o: /usr/include/asm/param.h
session.o: /home/arl/student/cgw1/wu_arl/wusrc/wuPP/addr.h
session.o: /home/arl/student/cgw1/wu_arl/wusrc/wuPP/sock.h
session.o: /home/arl/student/cgw1/wu_arl/wusrc/wuPP/buf.h
session.o: /home/arl/student/cgw1/wu_arl/wusrc/wuPP/msg.h control.h cmd.h
session.o: nccp.h basicConn.h streamConn.h session.h
session.o: /home/arl/student/cgw1/wu_arl/wusrc/wuPP/util.h proxy.h
proxy.o: /usr/include/stdint.h /usr/include/features.h
proxy.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
proxy.o: /usr/include/bits/wchar.h /usr/include/bits/wordsize.h
proxy.o: /usr/include/sys/select.h /usr/include/bits/types.h
proxy.o: /usr/lib/gcc/i386-redhat-linux/3.4.5/include/stddef.h
proxy.o: /usr/include/bits/typesizes.h /usr/include/bits/select.h
proxy.o: /usr/include/bits/sigset.h /usr/include/time.h
proxy.o: /usr/include/bits/time.h /usr/include/sys/time.h
proxy.o: /usr/include/sys/types.h /usr/include/endian.h
proxy.o: /usr/include/bits/endian.h /usr/include/sys/sysmacros.h
proxy.o: /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h
proxy.o: /usr/include/unistd.h /usr/include/bits/posix_opt.h
proxy.o: /usr/include/bits/environments.h /usr/include/bits/confname.h
proxy.o: /usr/include/getopt.h /usr/include/sys/uio.h /usr/include/bits/uio.h
proxy.o: /usr/include/sys/socket.h /usr/include/bits/socket.h
proxy.o: /usr/include/limits.h
proxy.o: /usr/lib/gcc/i386-redhat-linux/3.4.5/include/limits.h
proxy.o: /usr/include/bits/posix1_lim.h /usr/include/bits/local_lim.h
proxy.o: /usr/include/linux/limits.h /usr/include/bits/posix2_lim.h
proxy.o: /usr/include/bits/xopen_lim.h /usr/include/bits/stdio_lim.h
proxy.o: /usr/include/bits/sockaddr.h /usr/include/asm/socket.h
proxy.o: /usr/include/asm/sockios.h /usr/include/arpa/inet.h
proxy.o: /usr/include/netinet/in.h /usr/include/bits/in.h
proxy.o: /usr/include/bits/byteswap.h /usr/include/netdb.h
proxy.o: /usr/include/rpc/netdb.h /usr/include/bits/siginfo.h
proxy.o: /usr/include/bits/netdb.h /usr/include/signal.h
proxy.o: /usr/include/pthread.h /usr/include/sched.h
proxy.o: /usr/include/bits/initspin.h /usr/include/bits/sigthread.h
proxy.o: /home/arl/student/cgw1/wu_arl/wusrc/wulib/wulog.h
proxy.o: /usr/include/string.h /usr/include/xlocale.h
proxy.o: /home/arl/student/cgw1/wu_arl/wusrc/wuPP/net.h
proxy.o: /home/arl/student/cgw1/wu_arl/wusrc/wuPP/error.h
proxy.o: /usr/include/errno.h /usr/include/bits/errno.h
proxy.o: /usr/include/linux/errno.h /usr/include/asm/errno.h
proxy.o: /home/arl/student/cgw1/wu_arl/wusrc/wulib/wunet.h
proxy.o: /usr/include/sys/param.h /usr/include/linux/param.h
proxy.o: /usr/include/asm/param.h
proxy.o: /home/arl/student/cgw1/wu_arl/wusrc/wuPP/addr.h
proxy.o: /home/arl/student/cgw1/wu_arl/wusrc/wuPP/sock.h
proxy.o: /home/arl/student/cgw1/wu_arl/wusrc/wuPP/buf.h
proxy.o: /home/arl/student/cgw1/wu_arl/wusrc/wuPP/msg.h control.h cmd.h
proxy.o: nccp.h basicConn.h streamConn.h session.h
proxy.o: /home/arl/student/cgw1/wu_arl/wusrc/wuPP/util.h proxy.h
control.o: /usr/include/stdint.h /usr/include/features.h
control.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
control.o: /usr/include/bits/wchar.h /usr/include/bits/wordsize.h
control.o: /usr/include/unistd.h /usr/include/bits/posix_opt.h
control.o: /usr/include/bits/environments.h /usr/include/bits/types.h
control.o: /usr/lib/gcc/i386-redhat-linux/3.4.5/include/stddef.h
control.o: /usr/include/bits/typesizes.h /usr/include/bits/confname.h
control.o: /usr/include/getopt.h /usr/include/sys/types.h /usr/include/time.h
control.o: /usr/include/endian.h /usr/include/bits/endian.h
control.o: /usr/include/sys/select.h /usr/include/bits/select.h
control.o: /usr/include/bits/sigset.h /usr/include/bits/time.h
control.o: /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h
control.o: /usr/include/bits/sched.h /usr/include/sys/socket.h
control.o: /usr/include/sys/uio.h /usr/include/bits/uio.h
control.o: /usr/include/bits/socket.h /usr/include/limits.h
control.o: /usr/lib/gcc/i386-redhat-linux/3.4.5/include/limits.h
control.o: /usr/include/bits/posix1_lim.h /usr/include/bits/local_lim.h
control.o: /usr/include/linux/limits.h /usr/include/bits/posix2_lim.h
control.o: /usr/include/bits/xopen_lim.h /usr/include/bits/stdio_lim.h
control.o: /usr/include/bits/sockaddr.h /usr/include/asm/socket.h
control.o: /usr/include/asm/sockios.h /usr/include/arpa/inet.h
control.o: /usr/include/netinet/in.h /usr/include/bits/in.h
control.o: /usr/include/bits/byteswap.h /usr/include/netdb.h
control.o: /usr/include/rpc/netdb.h /usr/include/bits/siginfo.h
control.o: /usr/include/bits/netdb.h
control.o: /home/arl/student/cgw1/wu_arl/wusrc/wuPP/net.h
control.o: /home/arl/student/cgw1/wu_arl/wusrc/wuPP/error.h
control.o: /usr/include/errno.h /usr/include/bits/errno.h
control.o: /usr/include/linux/errno.h /usr/include/asm/errno.h
control.o: /home/arl/student/cgw1/wu_arl/wusrc/wulib/wulog.h
control.o: /usr/include/string.h /usr/include/xlocale.h
control.o: /home/arl/student/cgw1/wu_arl/wusrc/wulib/wunet.h
control.o: /usr/include/sys/param.h /usr/include/linux/param.h
control.o: /usr/include/asm/param.h
control.o: /home/arl/student/cgw1/wu_arl/wusrc/wuPP/buf.h
control.o: /home/arl/student/cgw1/wu_arl/wusrc/wuPP/msg.h
control.o: /home/arl/student/cgw1/wu_arl/wusrc/wuPP/addr.h
control.o: /home/arl/student/cgw1/wu_arl/wusrc/wuPP/sock.h control.h cmd.h
serv.o: /usr/include/stdio.h /usr/include/features.h /usr/include/sys/cdefs.h
serv.o: /usr/include/gnu/stubs.h
serv.o: /usr/lib/gcc/i386-redhat-linux/3.4.5/include/stddef.h
serv.o: /usr/include/bits/types.h /usr/include/bits/wordsize.h
serv.o: /usr/include/bits/typesizes.h /usr/include/libio.h
serv.o: /usr/include/_G_config.h /usr/include/wchar.h
serv.o: /usr/include/bits/wchar.h /usr/include/gconv.h
serv.o: /usr/lib/gcc/i386-redhat-linux/3.4.5/include/stdarg.h
serv.o: /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h
serv.o: /usr/include/stdlib.h /usr/include/bits/waitflags.h
serv.o: /usr/include/bits/waitstatus.h /usr/include/endian.h
serv.o: /usr/include/bits/endian.h /usr/include/xlocale.h
serv.o: /usr/include/sys/types.h /usr/include/time.h
serv.o: /usr/include/sys/select.h /usr/include/bits/select.h
serv.o: /usr/include/bits/sigset.h /usr/include/bits/time.h
serv.o: /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h
serv.o: /usr/include/bits/sched.h /usr/include/alloca.h
serv.o: /usr/include/strings.h /usr/include/unistd.h
serv.o: /usr/include/bits/posix_opt.h /usr/include/bits/environments.h
serv.o: /usr/include/bits/confname.h /usr/include/getopt.h
serv.o: /usr/include/sys/time.h /usr/include/sys/param.h
serv.o: /usr/include/limits.h
serv.o: /usr/lib/gcc/i386-redhat-linux/3.4.5/include/limits.h
serv.o: /usr/include/bits/posix1_lim.h /usr/include/bits/local_lim.h
serv.o: /usr/include/linux/limits.h /usr/include/bits/posix2_lim.h
serv.o: /usr/include/bits/xopen_lim.h /usr/include/linux/param.h
serv.o: /usr/include/asm/param.h /usr/include/sys/socket.h
serv.o: /usr/include/sys/uio.h /usr/include/bits/uio.h
serv.o: /usr/include/bits/socket.h /usr/include/bits/sockaddr.h
serv.o: /usr/include/asm/socket.h /usr/include/asm/sockios.h
serv.o: /usr/include/sys/file.h /usr/include/fcntl.h
serv.o: /usr/include/bits/fcntl.h /usr/include/sys/stat.h
serv.o: /usr/include/bits/stat.h /usr/include/sys/mman.h
serv.o: /usr/include/bits/mman.h /usr/include/net/if.h
serv.o: /usr/include/netinet/in_systm.h /usr/include/netinet/in.h
serv.o: /usr/include/stdint.h /usr/include/bits/in.h
serv.o: /usr/include/bits/byteswap.h /usr/include/arpa/inet.h
serv.o: /usr/include/netinet/ip.h /usr/include/netdb.h
serv.o: /usr/include/rpc/netdb.h /usr/include/bits/siginfo.h
serv.o: /usr/include/bits/netdb.h /usr/include/errno.h
serv.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
serv.o: /usr/include/asm/errno.h /usr/include/pthread.h /usr/include/sched.h
serv.o: /usr/include/signal.h /usr/include/bits/initspin.h
serv.o: /usr/include/bits/sigthread.h
serv.o: /home/arl/student/cgw1/wu_arl/wusrc/wulib/wulog.h
serv.o: /usr/include/string.h
serv.o: /home/arl/student/cgw1/wu_arl/wusrc/wulib/wunet.h
serv.o: /home/arl/student/cgw1/wu_arl/wusrc/wulib/keywords.h
onlProxy.o: /usr/include/stdint.h /usr/include/features.h
onlProxy.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
onlProxy.o: /usr/include/bits/wchar.h /usr/include/bits/wordsize.h
onlProxy.o: /usr/include/unistd.h /usr/include/bits/posix_opt.h
onlProxy.o: /usr/include/bits/environments.h /usr/include/bits/types.h
onlProxy.o: /usr/lib/gcc/i386-redhat-linux/3.4.5/include/stddef.h
onlProxy.o: /usr/include/bits/typesizes.h /usr/include/bits/confname.h
onlProxy.o: /usr/include/getopt.h /usr/include/sys/types.h
onlProxy.o: /usr/include/time.h /usr/include/endian.h
onlProxy.o: /usr/include/bits/endian.h /usr/include/sys/select.h
onlProxy.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
onlProxy.o: /usr/include/bits/time.h /usr/include/sys/sysmacros.h
onlProxy.o: /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h
onlProxy.o: /usr/include/sys/socket.h /usr/include/sys/uio.h
onlProxy.o: /usr/include/bits/uio.h /usr/include/bits/socket.h
onlProxy.o: /usr/include/limits.h
onlProxy.o: /usr/lib/gcc/i386-redhat-linux/3.4.5/include/limits.h
onlProxy.o: /usr/include/bits/posix1_lim.h /usr/include/bits/local_lim.h
onlProxy.o: /usr/include/linux/limits.h /usr/include/bits/posix2_lim.h
onlProxy.o: /usr/include/bits/xopen_lim.h /usr/include/bits/stdio_lim.h
onlProxy.o: /usr/include/bits/sockaddr.h /usr/include/asm/socket.h
onlProxy.o: /usr/include/asm/sockios.h /usr/include/arpa/inet.h
onlProxy.o: /usr/include/netinet/in.h /usr/include/bits/in.h
onlProxy.o: /usr/include/bits/byteswap.h /usr/include/netdb.h
onlProxy.o: /usr/include/rpc/netdb.h /usr/include/bits/siginfo.h
onlProxy.o: /usr/include/bits/netdb.h
onlProxy.o: /home/arl/student/cgw1/wu_arl/wusrc/wulib/wulog.h
onlProxy.o: /usr/include/string.h /usr/include/xlocale.h
onlProxy.o: /usr/include/signal.h
onlProxy.o: /home/arl/student/cgw1/wu_arl/wusrc/wuPP/net.h
onlProxy.o: /home/arl/student/cgw1/wu_arl/wusrc/wuPP/error.h
onlProxy.o: /usr/include/errno.h /usr/include/bits/errno.h
onlProxy.o: /usr/include/linux/errno.h /usr/include/asm/errno.h
onlProxy.o: /home/arl/student/cgw1/wu_arl/wusrc/wulib/wunet.h
onlProxy.o: /usr/include/sys/param.h /usr/include/linux/param.h
onlProxy.o: /usr/include/asm/param.h
onlProxy.o: /home/arl/student/cgw1/wu_arl/wusrc/wuPP/addr.h
onlProxy.o: /home/arl/student/cgw1/wu_arl/wusrc/wuPP/sock.h
onlProxy.o: /home/arl/student/cgw1/wu_arl/wusrc/wuPP/buf.h
onlProxy.o: /home/arl/student/cgw1/wu_arl/wusrc/wuPP/msg.h control.h cmd.h
onlProxy.o: nccp.h basicConn.h streamConn.h session.h
onlProxy.o: /home/arl/student/cgw1/wu_arl/wusrc/wuPP/util.h proxy.h
onlProxy.o: /usr/include/pthread.h /usr/include/sched.h
onlProxy.o: /usr/include/bits/initspin.h /usr/include/bits/sigthread.h
testFwd.o: /usr/include/stdint.h /usr/include/features.h
testFwd.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
testFwd.o: /usr/include/bits/wchar.h /usr/include/bits/wordsize.h
testFwd.o: /usr/include/unistd.h /usr/include/bits/posix_opt.h
testFwd.o: /usr/include/bits/environments.h /usr/include/bits/types.h
testFwd.o: /usr/lib/gcc/i386-redhat-linux/3.4.5/include/stddef.h
testFwd.o: /usr/include/bits/typesizes.h /usr/include/bits/confname.h
testFwd.o: /usr/include/getopt.h /usr/include/sys/types.h /usr/include/time.h
testFwd.o: /usr/include/endian.h /usr/include/bits/endian.h
testFwd.o: /usr/include/sys/select.h /usr/include/bits/select.h
testFwd.o: /usr/include/bits/sigset.h /usr/include/bits/time.h
testFwd.o: /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h
testFwd.o: /usr/include/bits/sched.h /usr/include/sys/socket.h
testFwd.o: /usr/include/sys/uio.h /usr/include/bits/uio.h
testFwd.o: /usr/include/bits/socket.h /usr/include/limits.h
testFwd.o: /usr/lib/gcc/i386-redhat-linux/3.4.5/include/limits.h
testFwd.o: /usr/include/bits/posix1_lim.h /usr/include/bits/local_lim.h
testFwd.o: /usr/include/linux/limits.h /usr/include/bits/posix2_lim.h
testFwd.o: /usr/include/bits/xopen_lim.h /usr/include/bits/stdio_lim.h
testFwd.o: /usr/include/bits/sockaddr.h /usr/include/asm/socket.h
testFwd.o: /usr/include/asm/sockios.h /usr/include/arpa/inet.h
testFwd.o: /usr/include/netinet/in.h /usr/include/bits/in.h
testFwd.o: /usr/include/bits/byteswap.h /usr/include/netdb.h
testFwd.o: /usr/include/rpc/netdb.h /usr/include/bits/siginfo.h
testFwd.o: /usr/include/bits/netdb.h
testFwd.o: /home/arl/student/cgw1/wu_arl/wusrc/wulib/wulog.h
testFwd.o: /usr/include/string.h /usr/include/xlocale.h /usr/include/signal.h
testFwd.o: /home/arl/student/cgw1/wu_arl/wusrc/wuPP/net.h
testFwd.o: /home/arl/student/cgw1/wu_arl/wusrc/wuPP/error.h
testFwd.o: /usr/include/errno.h /usr/include/bits/errno.h
testFwd.o: /usr/include/linux/errno.h /usr/include/asm/errno.h
testFwd.o: /home/arl/student/cgw1/wu_arl/wusrc/wulib/wunet.h
testFwd.o: /usr/include/sys/param.h /usr/include/linux/param.h
testFwd.o: /usr/include/asm/param.h
testFwd.o: /home/arl/student/cgw1/wu_arl/wusrc/wuPP/addr.h
testFwd.o: /home/arl/student/cgw1/wu_arl/wusrc/wuPP/sock.h
testFwd.o: /home/arl/student/cgw1/wu_arl/wusrc/wuPP/buf.h
testFwd.o: /home/arl/student/cgw1/wu_arl/wusrc/wuPP/msg.h control.h cmd.h
testFwd.o: nccp.h basicConn.h streamConn.h session.h
testFwd.o: /home/arl/student/cgw1/wu_arl/wusrc/wuPP/util.h proxy.h
testFwd.o: /usr/include/pthread.h /usr/include/sched.h
testFwd.o: /usr/include/bits/initspin.h /usr/include/bits/sigthread.h
pcmd.o: /usr/include/stdint.h /usr/include/features.h
pcmd.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
pcmd.o: /usr/include/bits/wchar.h /usr/include/bits/wordsize.h
pcmd.o: /usr/include/unistd.h /usr/include/bits/posix_opt.h
pcmd.o: /usr/include/bits/environments.h /usr/include/bits/types.h
pcmd.o: /usr/lib/gcc/i386-redhat-linux/3.4.5/include/stddef.h
pcmd.o: /usr/include/bits/typesizes.h /usr/include/bits/confname.h
pcmd.o: /usr/include/getopt.h /usr/include/sys/types.h /usr/include/time.h
pcmd.o: /usr/include/endian.h /usr/include/bits/endian.h
pcmd.o: /usr/include/sys/select.h /usr/include/bits/select.h
pcmd.o: /usr/include/bits/sigset.h /usr/include/bits/time.h
pcmd.o: /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h
pcmd.o: /usr/include/bits/sched.h /usr/include/sys/socket.h
pcmd.o: /usr/include/sys/uio.h /usr/include/bits/uio.h
pcmd.o: /usr/include/bits/socket.h /usr/include/limits.h
pcmd.o: /usr/lib/gcc/i386-redhat-linux/3.4.5/include/limits.h
pcmd.o: /usr/include/bits/posix1_lim.h /usr/include/bits/local_lim.h
pcmd.o: /usr/include/linux/limits.h /usr/include/bits/posix2_lim.h
pcmd.o: /usr/include/bits/xopen_lim.h /usr/include/bits/stdio_lim.h
pcmd.o: /usr/include/bits/sockaddr.h /usr/include/asm/socket.h
pcmd.o: /usr/include/asm/sockios.h /usr/include/arpa/inet.h
pcmd.o: /usr/include/netinet/in.h /usr/include/bits/in.h
pcmd.o: /usr/include/bits/byteswap.h /usr/include/netdb.h
pcmd.o: /usr/include/rpc/netdb.h /usr/include/bits/siginfo.h
pcmd.o: /usr/include/bits/netdb.h
pcmd.o: /home/arl/student/cgw1/wu_arl/wusrc/wulib/wulog.h
pcmd.o: /usr/include/string.h /usr/include/xlocale.h /usr/include/signal.h
pcmd.o: /home/arl/student/cgw1/wu_arl/wusrc/wuPP/net.h
pcmd.o: /home/arl/student/cgw1/wu_arl/wusrc/wuPP/error.h /usr/include/errno.h
pcmd.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
pcmd.o: /usr/include/asm/errno.h
pcmd.o: /home/arl/student/cgw1/wu_arl/wusrc/wulib/wunet.h
pcmd.o: /usr/include/sys/param.h /usr/include/linux/param.h
pcmd.o: /usr/include/asm/param.h
pcmd.o: /home/arl/student/cgw1/wu_arl/wusrc/wuPP/addr.h
pcmd.o: /home/arl/student/cgw1/wu_arl/wusrc/wuPP/sock.h
pcmd.o: /home/arl/student/cgw1/wu_arl/wusrc/wuPP/buf.h
pcmd.o: /home/arl/student/cgw1/wu_arl/wusrc/wuPP/msg.h control.h cmd.h nccp.h
pcmd.o: basicConn.h streamConn.h session.h
pcmd.o: /home/arl/student/cgw1/wu_arl/wusrc/wuPP/util.h proxy.h
pcmd.o: /usr/include/pthread.h /usr/include/sched.h
pcmd.o: /usr/include/bits/initspin.h /usr/include/bits/sigthread.h
kcmd.o: /usr/include/stdint.h /usr/include/features.h
kcmd.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
kcmd.o: /usr/include/bits/wchar.h /usr/include/bits/wordsize.h
kcmd.o: /usr/include/unistd.h /usr/include/bits/posix_opt.h
kcmd.o: /usr/include/bits/environments.h /usr/include/bits/types.h
kcmd.o: /usr/lib/gcc/i386-redhat-linux/3.4.5/include/stddef.h
kcmd.o: /usr/include/bits/typesizes.h /usr/include/bits/confname.h
kcmd.o: /usr/include/getopt.h /usr/include/sys/types.h /usr/include/time.h
kcmd.o: /usr/include/endian.h /usr/include/bits/endian.h
kcmd.o: /usr/include/sys/select.h /usr/include/bits/select.h
kcmd.o: /usr/include/bits/sigset.h /usr/include/bits/time.h
kcmd.o: /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h
kcmd.o: /usr/include/bits/sched.h /usr/include/sys/socket.h
kcmd.o: /usr/include/sys/uio.h /usr/include/bits/uio.h
kcmd.o: /usr/include/bits/socket.h /usr/include/limits.h
kcmd.o: /usr/lib/gcc/i386-redhat-linux/3.4.5/include/limits.h
kcmd.o: /usr/include/bits/posix1_lim.h /usr/include/bits/local_lim.h
kcmd.o: /usr/include/linux/limits.h /usr/include/bits/posix2_lim.h
kcmd.o: /usr/include/bits/xopen_lim.h /usr/include/bits/stdio_lim.h
kcmd.o: /usr/include/bits/sockaddr.h /usr/include/asm/socket.h
kcmd.o: /usr/include/asm/sockios.h /usr/include/arpa/inet.h
kcmd.o: /usr/include/netinet/in.h /usr/include/bits/in.h
kcmd.o: /usr/include/bits/byteswap.h /usr/include/netdb.h
kcmd.o: /usr/include/rpc/netdb.h /usr/include/bits/siginfo.h
kcmd.o: /usr/include/bits/netdb.h
kcmd.o: /home/arl/student/cgw1/wu_arl/wusrc/wulib/wulog.h
kcmd.o: /usr/include/string.h /usr/include/xlocale.h /usr/include/signal.h
kcmd.o: /home/arl/student/cgw1/wu_arl/wusrc/wuPP/net.h
kcmd.o: /home/arl/student/cgw1/wu_arl/wusrc/wuPP/error.h /usr/include/errno.h
kcmd.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
kcmd.o: /usr/include/asm/errno.h
kcmd.o: /home/arl/student/cgw1/wu_arl/wusrc/wulib/wunet.h
kcmd.o: /usr/include/sys/param.h /usr/include/linux/param.h
kcmd.o: /usr/include/asm/param.h
kcmd.o: /home/arl/student/cgw1/wu_arl/wusrc/wuPP/addr.h
kcmd.o: /home/arl/student/cgw1/wu_arl/wusrc/wuPP/sock.h
kcmd.o: /home/arl/student/cgw1/wu_arl/wusrc/wuPP/buf.h
kcmd.o: /home/arl/student/cgw1/wu_arl/wusrc/wuPP/msg.h control.h cmd.h nccp.h
kcmd.o: basicConn.h streamConn.h session.h
kcmd.o: /home/arl/student/cgw1/wu_arl/wusrc/wuPP/util.h proxy.h
kcmd.o: /usr/include/pthread.h /usr/include/sched.h
kcmd.o: /usr/include/bits/initspin.h /usr/include/bits/sigthread.h
pingProxy.o: /usr/include/stdint.h /usr/include/features.h
pingProxy.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
pingProxy.o: /usr/include/bits/wchar.h /usr/include/bits/wordsize.h
pingProxy.o: /usr/include/unistd.h /usr/include/bits/posix_opt.h
pingProxy.o: /usr/include/bits/environments.h /usr/include/bits/types.h
pingProxy.o: /usr/lib/gcc/i386-redhat-linux/3.4.5/include/stddef.h
pingProxy.o: /usr/include/bits/typesizes.h /usr/include/bits/confname.h
pingProxy.o: /usr/include/getopt.h /usr/include/sys/types.h
pingProxy.o: /usr/include/time.h /usr/include/endian.h
pingProxy.o: /usr/include/bits/endian.h /usr/include/sys/select.h
pingProxy.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
pingProxy.o: /usr/include/bits/time.h /usr/include/sys/sysmacros.h
pingProxy.o: /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h
pingProxy.o: /usr/include/sys/socket.h /usr/include/sys/uio.h
pingProxy.o: /usr/include/bits/uio.h /usr/include/bits/socket.h
pingProxy.o: /usr/include/limits.h
pingProxy.o: /usr/lib/gcc/i386-redhat-linux/3.4.5/include/limits.h
pingProxy.o: /usr/include/bits/posix1_lim.h /usr/include/bits/local_lim.h
pingProxy.o: /usr/include/linux/limits.h /usr/include/bits/posix2_lim.h
pingProxy.o: /usr/include/bits/xopen_lim.h /usr/include/bits/stdio_lim.h
pingProxy.o: /usr/include/bits/sockaddr.h /usr/include/asm/socket.h
pingProxy.o: /usr/include/asm/sockios.h /usr/include/arpa/inet.h
pingProxy.o: /usr/include/netinet/in.h /usr/include/bits/in.h
pingProxy.o: /usr/include/bits/byteswap.h /usr/include/netdb.h
pingProxy.o: /usr/include/rpc/netdb.h /usr/include/bits/siginfo.h
pingProxy.o: /usr/include/bits/netdb.h
pingProxy.o: /home/arl/student/cgw1/wu_arl/wusrc/wulib/wulog.h
pingProxy.o: /usr/include/string.h /usr/include/xlocale.h
pingProxy.o: /usr/include/signal.h
pingProxy.o: /home/arl/student/cgw1/wu_arl/wusrc/wuPP/net.h
pingProxy.o: /home/arl/student/cgw1/wu_arl/wusrc/wuPP/error.h
pingProxy.o: /usr/include/errno.h /usr/include/bits/errno.h
pingProxy.o: /usr/include/linux/errno.h /usr/include/asm/errno.h
pingProxy.o: /home/arl/student/cgw1/wu_arl/wusrc/wulib/wunet.h
pingProxy.o: /usr/include/sys/param.h /usr/include/linux/param.h
pingProxy.o: /usr/include/asm/param.h
pingProxy.o: /home/arl/student/cgw1/wu_arl/wusrc/wuPP/addr.h
pingProxy.o: /home/arl/student/cgw1/wu_arl/wusrc/wuPP/sock.h
pingProxy.o: /home/arl/student/cgw1/wu_arl/wusrc/wuPP/buf.h
pingProxy.o: /home/arl/student/cgw1/wu_arl/wusrc/wuPP/msg.h control.h cmd.h
pingProxy.o: nccp.h basicConn.h streamConn.h session.h
pingProxy.o: /home/arl/student/cgw1/wu_arl/wusrc/wuPP/util.h proxy.h
pingProxy.o: /usr/include/pthread.h /usr/include/sched.h
pingProxy.o: /usr/include/bits/initspin.h /usr/include/bits/sigthread.h
