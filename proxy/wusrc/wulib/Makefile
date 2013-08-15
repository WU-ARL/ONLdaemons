#
# $Source: /project/arlcvs/cvsroot/wu_arl/wusrc/wulib/Makefile,v $
# $Author: fredk $
# $Date: 2008/02/11 16:41:04 $
# $Revision: 1.37 $
#
# Author: Fred Kuhns
#         fredk@arl.wustl.edu
# Organization: Applied Research Laboratory
#               Washington UNiversity in St. Louis
#
WUSRC = ..
include ../Makefile.inc

HDRS  = wulog.h loopit.h  wunet.h ipnet.h  period.h  timer.h stats.h bits.h \
        keywords.h queue.h pqueue.h syn.h barrier.h net_util.h wusock.h valTypes.h \
	bsdnet.h

SRCS  = wulog.c wunet.c ipnet.c period.c pqueue.c stats.c timer.c barrier.c \
        syn.c wusock.c valTypes.c

KSRCS =
OBJS  = $(addprefix ${OBJDIR}/, $(patsubst %.c,%.o,$(SRCS)))
KOBJS = $(addprefix ${OBJDIR}/, $(patsubst %.c,k%.o,$(KSRCS)))

KFLAGS += -DIOSTYLE=${IOSTYLE} -D__KERNEL__

## Use whatever hass been defined for WUINCLUDES
INCLUDES = ${WUINCLUDES}
CFLAGS   = $(WUCFLAGS_DB)

KINCLUDE = ${MODINC} ${WUINC}

LIBDIR	= ../lib/${OBJDIR}

all: user kern
	echo ${OBJDIR}

.PHONY : ixp
ixp :
	@echo "@@@ Building for the IXP target @@@"
	@${MAKE} --no-print-directory WUTARGET="ixp" -e all

ixp-clean :
	@echo "@@@ Cleaning for the IXP target @@@"
	@${MAKE} --no-print-directory WUTARGET="ixp" -e clean

install : all ${LIBDIR}
	install -c -p $(OBJDIR)/$(WULIBNAME) $(LIBDIR)

user : $(OBJDIR) $(OBJDIR)/$(WULIBNAME)

kern : $(OBJDIR) $(OBJDIR)/$(WUKLIBNAME)

$(OBJDIR) :
	$(mk-objdir)

${LIBDIR} :
	mkdir ../lib
	mkdir ../lib/${OBJDIR}

$(KOBJS) : ${OBJDIR}/k%.o : %.c $(HDRS)
	${CC} ${KFLAGS} ${MODCFLGS} ${KINCLUDE} -o $@ -c $<

$(OBJS) : ${OBJDIR}/%.o : %.c $(HDRS) Makefile
	$(mk-cobjs)


#	@echo -e "--- Updating object file $@"
#	${CC} ${CFLAGS} ${INCLUDES} -o $@ -c $<

$(OBJDIR)/$(WUKLIBNAME) : $(KOBJS)
	echo $(OBJDIR)
	$(AR) rc $@ $(KOBJS)
	$(RANLIB) $@

$(OBJDIR)/$(WULIBNAME) : $(OBJS)
	@echo
	@echo "*** Update archive $(WULIBNAME) ***"
	@echo
	@$(AR) rc $@ $(OBJS)
	@$(RANLIB) $@

clean:
	/bin/rm -f $(OBJS)
	/bin/rm -f $(OBJDIR)/$(WULIBNAME)
	/bin/rm -f $(KOBJS)

realclean:
	/bin/rm -f *~
	/bin/rm -R $(OBJDIR)

depend:
	makedepend -- $(CFLAGS) $(INCLUDES) -- $(SRCS)

# DO NOT DELETE

wulog.o: /usr/include/stdio.h /usr/include/features.h
wulog.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
wulog.o: /usr/lib/gcc/i386-redhat-linux/3.4.5/include/stddef.h
wulog.o: /usr/include/bits/types.h /usr/include/bits/wordsize.h
wulog.o: /usr/include/bits/typesizes.h /usr/include/libio.h
wulog.o: /usr/include/_G_config.h /usr/include/wchar.h
wulog.o: /usr/include/bits/wchar.h /usr/include/gconv.h
wulog.o: /usr/lib/gcc/i386-redhat-linux/3.4.5/include/stdarg.h
wulog.o: /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h
wulog.o: /usr/include/stdlib.h /usr/include/bits/waitflags.h
wulog.o: /usr/include/bits/waitstatus.h /usr/include/endian.h
wulog.o: /usr/include/bits/endian.h /usr/include/xlocale.h
wulog.o: /usr/include/sys/types.h /usr/include/time.h
wulog.o: /usr/include/sys/select.h /usr/include/bits/select.h
wulog.o: /usr/include/bits/sigset.h /usr/include/bits/time.h
wulog.o: /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h
wulog.o: /usr/include/bits/sched.h /usr/include/alloca.h
wulog.o: /usr/include/string.h /usr/include/stdint.h /usr/include/syslog.h
wulog.o: /usr/include/sys/syslog.h
wulog.o: /home/arl/student/cgw1/wu_arl/wusrc/wulib/bits.h
wulog.o: /home/arl/student/cgw1/wu_arl/wusrc/wulib/wulog.h
log.o: /usr/include/stdio.h /usr/include/features.h /usr/include/sys/cdefs.h
log.o: /usr/include/gnu/stubs.h
log.o: /usr/lib/gcc/i386-redhat-linux/3.4.5/include/stddef.h
log.o: /usr/include/bits/types.h /usr/include/bits/wordsize.h
log.o: /usr/include/bits/typesizes.h /usr/include/libio.h
log.o: /usr/include/_G_config.h /usr/include/wchar.h
log.o: /usr/include/bits/wchar.h /usr/include/gconv.h
log.o: /usr/lib/gcc/i386-redhat-linux/3.4.5/include/stdarg.h
log.o: /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h
log.o: /usr/include/stdlib.h /usr/include/bits/waitflags.h
log.o: /usr/include/bits/waitstatus.h /usr/include/endian.h
log.o: /usr/include/bits/endian.h /usr/include/xlocale.h
log.o: /usr/include/sys/types.h /usr/include/time.h /usr/include/sys/select.h
log.o: /usr/include/bits/select.h /usr/include/bits/sigset.h
log.o: /usr/include/bits/time.h /usr/include/sys/sysmacros.h
log.o: /usr/include/bits/pthreadtypes.h /usr/include/bits/sched.h
log.o: /usr/include/alloca.h /usr/include/stdint.h /usr/include/string.h
log.o: /home/arl/student/cgw1/wu_arl/wusrc/wulib/keywords.h
log.o: /home/arl/student/cgw1/wu_arl/wusrc/wulib/log.h
log.o: /home/arl/student/cgw1/wu_arl/wusrc/wulib/bits.h
wunet.o: /usr/include/stdio.h /usr/include/features.h
wunet.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
wunet.o: /usr/lib/gcc/i386-redhat-linux/3.4.5/include/stddef.h
wunet.o: /usr/include/bits/types.h /usr/include/bits/wordsize.h
wunet.o: /usr/include/bits/typesizes.h /usr/include/libio.h
wunet.o: /usr/include/_G_config.h /usr/include/wchar.h
wunet.o: /usr/include/bits/wchar.h /usr/include/gconv.h
wunet.o: /usr/lib/gcc/i386-redhat-linux/3.4.5/include/stdarg.h
wunet.o: /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h
wunet.o: /usr/include/stdlib.h /usr/include/bits/waitflags.h
wunet.o: /usr/include/bits/waitstatus.h /usr/include/endian.h
wunet.o: /usr/include/bits/endian.h /usr/include/xlocale.h
wunet.o: /usr/include/sys/types.h /usr/include/time.h
wunet.o: /usr/include/sys/select.h /usr/include/bits/select.h
wunet.o: /usr/include/bits/sigset.h /usr/include/bits/time.h
wunet.o: /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h
wunet.o: /usr/include/bits/sched.h /usr/include/alloca.h
wunet.o: /usr/include/string.h /usr/include/sys/param.h /usr/include/limits.h
wunet.o: /usr/lib/gcc/i386-redhat-linux/3.4.5/include/limits.h
wunet.o: /usr/include/bits/posix1_lim.h /usr/include/bits/local_lim.h
wunet.o: /usr/include/linux/limits.h /usr/include/bits/posix2_lim.h
wunet.o: /usr/include/bits/xopen_lim.h /usr/include/linux/param.h
wunet.o: /usr/include/asm/param.h /usr/include/unistd.h
wunet.o: /usr/include/bits/posix_opt.h /usr/include/bits/environments.h
wunet.o: /usr/include/bits/confname.h /usr/include/getopt.h
wunet.o: /usr/include/sys/socket.h /usr/include/sys/uio.h
wunet.o: /usr/include/bits/uio.h /usr/include/bits/socket.h
wunet.o: /usr/include/bits/sockaddr.h /usr/include/asm/socket.h
wunet.o: /usr/include/asm/sockios.h /usr/include/net/if.h
wunet.o: /usr/include/netinet/in.h /usr/include/stdint.h
wunet.o: /usr/include/bits/in.h /usr/include/bits/byteswap.h
wunet.o: /usr/include/ctype.h /usr/include/errno.h /usr/include/bits/errno.h
wunet.o: /usr/include/linux/errno.h /usr/include/asm/errno.h
wunet.o: /home/arl/student/cgw1/wu_arl/wusrc/wulib/wulog.h
wunet.o: /home/arl/student/cgw1/wu_arl/wusrc/wulib/wunet.h
wunet.o: /usr/include/netdb.h /usr/include/rpc/netdb.h
wunet.o: /usr/include/bits/siginfo.h /usr/include/bits/netdb.h
wunet.o: /usr/include/arpa/inet.h
ipnet.o: /usr/include/stdio.h /usr/include/features.h
ipnet.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
ipnet.o: /usr/lib/gcc/i386-redhat-linux/3.4.5/include/stddef.h
ipnet.o: /usr/include/bits/types.h /usr/include/bits/wordsize.h
ipnet.o: /usr/include/bits/typesizes.h /usr/include/libio.h
ipnet.o: /usr/include/_G_config.h /usr/include/wchar.h
ipnet.o: /usr/include/bits/wchar.h /usr/include/gconv.h
ipnet.o: /usr/lib/gcc/i386-redhat-linux/3.4.5/include/stdarg.h
ipnet.o: /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h
ipnet.o: /usr/include/stdlib.h /usr/include/bits/waitflags.h
ipnet.o: /usr/include/bits/waitstatus.h /usr/include/endian.h
ipnet.o: /usr/include/bits/endian.h /usr/include/xlocale.h
ipnet.o: /usr/include/sys/types.h /usr/include/time.h
ipnet.o: /usr/include/sys/select.h /usr/include/bits/select.h
ipnet.o: /usr/include/bits/sigset.h /usr/include/bits/time.h
ipnet.o: /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h
ipnet.o: /usr/include/bits/sched.h /usr/include/alloca.h
ipnet.o: /usr/include/sys/param.h /usr/include/limits.h
ipnet.o: /usr/lib/gcc/i386-redhat-linux/3.4.5/include/limits.h
ipnet.o: /usr/include/bits/posix1_lim.h /usr/include/bits/local_lim.h
ipnet.o: /usr/include/linux/limits.h /usr/include/bits/posix2_lim.h
ipnet.o: /usr/include/bits/xopen_lim.h /usr/include/linux/param.h
ipnet.o: /usr/include/asm/param.h /usr/include/unistd.h
ipnet.o: /usr/include/bits/posix_opt.h /usr/include/bits/environments.h
ipnet.o: /usr/include/bits/confname.h /usr/include/getopt.h
ipnet.o: /usr/include/netdb.h /usr/include/netinet/in.h /usr/include/stdint.h
ipnet.o: /usr/include/sys/socket.h /usr/include/sys/uio.h
ipnet.o: /usr/include/bits/uio.h /usr/include/bits/socket.h
ipnet.o: /usr/include/bits/sockaddr.h /usr/include/asm/socket.h
ipnet.o: /usr/include/asm/sockios.h /usr/include/bits/in.h
ipnet.o: /usr/include/bits/byteswap.h /usr/include/rpc/netdb.h
ipnet.o: /usr/include/bits/siginfo.h /usr/include/bits/netdb.h
ipnet.o: /usr/include/arpa/inet.h /usr/include/netinet/in_systm.h
ipnet.o: /usr/include/netinet/ip.h /usr/include/netinet/tcp.h
ipnet.o: /usr/include/netinet/udp.h /usr/include/ctype.h
ipnet.o: /home/arl/student/cgw1/wu_arl/wusrc/wulib/ipnet.h
ipnet.o: /home/arl/student/cgw1/wu_arl/wusrc/wulib/wunet.h
ipnet.o: /usr/include/string.h
period.o: /home/arl/student/cgw1/wu_arl/wusrc/wulib/kern.h
period.o: /usr/include/stdio.h /usr/include/features.h
period.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
period.o: /usr/lib/gcc/i386-redhat-linux/3.4.5/include/stddef.h
period.o: /usr/include/bits/types.h /usr/include/bits/wordsize.h
period.o: /usr/include/bits/typesizes.h /usr/include/libio.h
period.o: /usr/include/_G_config.h /usr/include/wchar.h
period.o: /usr/include/bits/wchar.h /usr/include/gconv.h
period.o: /usr/lib/gcc/i386-redhat-linux/3.4.5/include/stdarg.h
period.o: /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h
period.o: /usr/include/stdlib.h /usr/include/bits/waitflags.h
period.o: /usr/include/bits/waitstatus.h /usr/include/endian.h
period.o: /usr/include/bits/endian.h /usr/include/xlocale.h
period.o: /usr/include/sys/types.h /usr/include/time.h
period.o: /usr/include/sys/select.h /usr/include/bits/select.h
period.o: /usr/include/bits/sigset.h /usr/include/bits/time.h
period.o: /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h
period.o: /usr/include/bits/sched.h /usr/include/alloca.h
period.o: /usr/include/inttypes.h /usr/include/stdint.h
period.o: /usr/include/strings.h /usr/include/ctype.h /usr/include/sys/time.h
period.o: /usr/include/unistd.h /usr/include/bits/posix_opt.h
period.o: /usr/include/bits/environments.h /usr/include/bits/confname.h
period.o: /usr/include/getopt.h
period.o: /home/arl/student/cgw1/wu_arl/wusrc/wulib/wulog.h
period.o: /usr/include/string.h
period.o: /home/arl/student/cgw1/wu_arl/wusrc/wulib/period.h
period.o: /home/arl/student/cgw1/wu_arl/wusrc/wulib/timer.h
period.o: /usr/include/sys/param.h /usr/include/limits.h
period.o: /usr/lib/gcc/i386-redhat-linux/3.4.5/include/limits.h
period.o: /usr/include/bits/posix1_lim.h /usr/include/bits/local_lim.h
period.o: /usr/include/linux/limits.h /usr/include/bits/posix2_lim.h
period.o: /usr/include/bits/xopen_lim.h /usr/include/linux/param.h
period.o: /usr/include/asm/param.h
pqueue.o: /home/arl/student/cgw1/wu_arl/wusrc/wulib/kern.h
pqueue.o: /usr/include/stdio.h /usr/include/features.h
pqueue.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
pqueue.o: /usr/lib/gcc/i386-redhat-linux/3.4.5/include/stddef.h
pqueue.o: /usr/include/bits/types.h /usr/include/bits/wordsize.h
pqueue.o: /usr/include/bits/typesizes.h /usr/include/libio.h
pqueue.o: /usr/include/_G_config.h /usr/include/wchar.h
pqueue.o: /usr/include/bits/wchar.h /usr/include/gconv.h
pqueue.o: /usr/lib/gcc/i386-redhat-linux/3.4.5/include/stdarg.h
pqueue.o: /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h
pqueue.o: /usr/include/stdlib.h /usr/include/bits/waitflags.h
pqueue.o: /usr/include/bits/waitstatus.h /usr/include/endian.h
pqueue.o: /usr/include/bits/endian.h /usr/include/xlocale.h
pqueue.o: /usr/include/sys/types.h /usr/include/time.h
pqueue.o: /usr/include/sys/select.h /usr/include/bits/select.h
pqueue.o: /usr/include/bits/sigset.h /usr/include/bits/time.h
pqueue.o: /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h
pqueue.o: /usr/include/bits/sched.h /usr/include/alloca.h
pqueue.o: /usr/include/inttypes.h /usr/include/stdint.h
pqueue.o: /usr/include/strings.h /usr/include/ctype.h /usr/include/sys/time.h
pqueue.o: pqueue.h /home/arl/student/cgw1/wu_arl/wusrc/wulib/syn.h
pqueue.o: /usr/include/pthread.h /usr/include/sched.h /usr/include/signal.h
pqueue.o: /usr/include/bits/initspin.h /usr/include/bits/sigthread.h
stats.o: /home/arl/student/cgw1/wu_arl/wusrc/wulib/kern.h
stats.o: /usr/include/stdio.h /usr/include/features.h
stats.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
stats.o: /usr/lib/gcc/i386-redhat-linux/3.4.5/include/stddef.h
stats.o: /usr/include/bits/types.h /usr/include/bits/wordsize.h
stats.o: /usr/include/bits/typesizes.h /usr/include/libio.h
stats.o: /usr/include/_G_config.h /usr/include/wchar.h
stats.o: /usr/include/bits/wchar.h /usr/include/gconv.h
stats.o: /usr/lib/gcc/i386-redhat-linux/3.4.5/include/stdarg.h
stats.o: /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h
stats.o: /usr/include/stdlib.h /usr/include/bits/waitflags.h
stats.o: /usr/include/bits/waitstatus.h /usr/include/endian.h
stats.o: /usr/include/bits/endian.h /usr/include/xlocale.h
stats.o: /usr/include/sys/types.h /usr/include/time.h
stats.o: /usr/include/sys/select.h /usr/include/bits/select.h
stats.o: /usr/include/bits/sigset.h /usr/include/bits/time.h
stats.o: /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h
stats.o: /usr/include/bits/sched.h /usr/include/alloca.h
stats.o: /usr/include/inttypes.h /usr/include/stdint.h /usr/include/strings.h
stats.o: /usr/include/ctype.h /usr/include/math.h
stats.o: /usr/include/bits/huge_val.h /usr/include/bits/huge_valf.h
stats.o: /usr/include/bits/huge_vall.h /usr/include/bits/inf.h
stats.o: /usr/include/bits/nan.h /usr/include/bits/mathdef.h
stats.o: /usr/include/bits/mathcalls.h /usr/include/sys/time.h
stats.o: /home/arl/student/cgw1/wu_arl/wusrc/wulib/wulog.h
stats.o: /usr/include/string.h
stats.o: /home/arl/student/cgw1/wu_arl/wusrc/wulib/stats.h
stats.o: /home/arl/student/cgw1/wu_arl/wusrc/wulib/timer.h
stats.o: /usr/include/sys/param.h /usr/include/limits.h
stats.o: /usr/lib/gcc/i386-redhat-linux/3.4.5/include/limits.h
stats.o: /usr/include/bits/posix1_lim.h /usr/include/bits/local_lim.h
stats.o: /usr/include/linux/limits.h /usr/include/bits/posix2_lim.h
stats.o: /usr/include/bits/xopen_lim.h /usr/include/linux/param.h
stats.o: /usr/include/asm/param.h /usr/include/unistd.h
stats.o: /usr/include/bits/posix_opt.h /usr/include/bits/environments.h
stats.o: /usr/include/bits/confname.h /usr/include/getopt.h
timer.o: /home/arl/student/cgw1/wu_arl/wusrc/wulib/kern.h
timer.o: /usr/include/stdio.h /usr/include/features.h
timer.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
timer.o: /usr/lib/gcc/i386-redhat-linux/3.4.5/include/stddef.h
timer.o: /usr/include/bits/types.h /usr/include/bits/wordsize.h
timer.o: /usr/include/bits/typesizes.h /usr/include/libio.h
timer.o: /usr/include/_G_config.h /usr/include/wchar.h
timer.o: /usr/include/bits/wchar.h /usr/include/gconv.h
timer.o: /usr/lib/gcc/i386-redhat-linux/3.4.5/include/stdarg.h
timer.o: /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h
timer.o: /usr/include/stdlib.h /usr/include/bits/waitflags.h
timer.o: /usr/include/bits/waitstatus.h /usr/include/endian.h
timer.o: /usr/include/bits/endian.h /usr/include/xlocale.h
timer.o: /usr/include/sys/types.h /usr/include/time.h
timer.o: /usr/include/sys/select.h /usr/include/bits/select.h
timer.o: /usr/include/bits/sigset.h /usr/include/bits/time.h
timer.o: /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h
timer.o: /usr/include/bits/sched.h /usr/include/alloca.h
timer.o: /usr/include/inttypes.h /usr/include/stdint.h /usr/include/strings.h
timer.o: /usr/include/ctype.h /usr/include/string.h /usr/include/math.h
timer.o: /usr/include/bits/huge_val.h /usr/include/bits/huge_valf.h
timer.o: /usr/include/bits/huge_vall.h /usr/include/bits/inf.h
timer.o: /usr/include/bits/nan.h /usr/include/bits/mathdef.h
timer.o: /usr/include/bits/mathcalls.h /usr/include/sys/time.h
timer.o: /home/arl/student/cgw1/wu_arl/wusrc/wulib/wulog.h
timer.o: /home/arl/student/cgw1/wu_arl/wusrc/wulib/timer.h
timer.o: /usr/include/sys/param.h /usr/include/limits.h
timer.o: /usr/lib/gcc/i386-redhat-linux/3.4.5/include/limits.h
timer.o: /usr/include/bits/posix1_lim.h /usr/include/bits/local_lim.h
timer.o: /usr/include/linux/limits.h /usr/include/bits/posix2_lim.h
timer.o: /usr/include/bits/xopen_lim.h /usr/include/linux/param.h
timer.o: /usr/include/asm/param.h /usr/include/unistd.h
timer.o: /usr/include/bits/posix_opt.h /usr/include/bits/environments.h
timer.o: /usr/include/bits/confname.h /usr/include/getopt.h
barrier.o: /usr/include/stdio.h /usr/include/features.h
barrier.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
barrier.o: /usr/lib/gcc/i386-redhat-linux/3.4.5/include/stddef.h
barrier.o: /usr/include/bits/types.h /usr/include/bits/wordsize.h
barrier.o: /usr/include/bits/typesizes.h /usr/include/libio.h
barrier.o: /usr/include/_G_config.h /usr/include/wchar.h
barrier.o: /usr/include/bits/wchar.h /usr/include/gconv.h
barrier.o: /usr/lib/gcc/i386-redhat-linux/3.4.5/include/stdarg.h
barrier.o: /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h
barrier.o: /usr/include/stdlib.h /usr/include/bits/waitflags.h
barrier.o: /usr/include/bits/waitstatus.h /usr/include/endian.h
barrier.o: /usr/include/bits/endian.h /usr/include/xlocale.h
barrier.o: /usr/include/sys/types.h /usr/include/time.h
barrier.o: /usr/include/sys/select.h /usr/include/bits/select.h
barrier.o: /usr/include/bits/sigset.h /usr/include/bits/time.h
barrier.o: /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h
barrier.o: /usr/include/bits/sched.h /usr/include/alloca.h
barrier.o: /usr/include/inttypes.h /usr/include/stdint.h
barrier.o: /usr/include/string.h /usr/include/sys/time.h
barrier.o: /usr/include/sys/wait.h /usr/include/signal.h
barrier.o: /usr/include/sys/resource.h /usr/include/bits/resource.h
barrier.o: /usr/include/bits/siginfo.h /usr/include/unistd.h
barrier.o: /usr/include/bits/posix_opt.h /usr/include/bits/environments.h
barrier.o: /usr/include/bits/confname.h /usr/include/getopt.h
barrier.o: /usr/include/errno.h /usr/include/bits/errno.h
barrier.o: /usr/include/linux/errno.h /usr/include/asm/errno.h
barrier.o: /home/arl/student/cgw1/wu_arl/wusrc/wulib/syn.h
barrier.o: /usr/include/pthread.h /usr/include/sched.h
barrier.o: /usr/include/bits/initspin.h /usr/include/bits/sigthread.h
barrier.o: /home/arl/student/cgw1/wu_arl/wusrc/wulib/barrier.h
barrier.o: /home/arl/student/cgw1/wu_arl/wusrc/wulib/wulog.h
syn.o: /home/arl/student/cgw1/wu_arl/wusrc/wulib/syn.h /usr/include/pthread.h
syn.o: /usr/include/features.h /usr/include/sys/cdefs.h
syn.o: /usr/include/gnu/stubs.h /usr/include/sched.h
syn.o: /usr/include/bits/types.h /usr/include/bits/wordsize.h
syn.o: /usr/lib/gcc/i386-redhat-linux/3.4.5/include/stddef.h
syn.o: /usr/include/bits/typesizes.h /usr/include/time.h
syn.o: /usr/include/bits/sched.h /usr/include/signal.h
syn.o: /usr/include/bits/sigset.h /usr/include/bits/pthreadtypes.h
syn.o: /usr/include/bits/initspin.h /usr/include/bits/sigthread.h
wusock.o: /usr/include/sys/uio.h /usr/include/features.h
wusock.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
wusock.o: /usr/include/sys/types.h /usr/include/bits/types.h
wusock.o: /usr/include/bits/wordsize.h
wusock.o: /usr/lib/gcc/i386-redhat-linux/3.4.5/include/stddef.h
wusock.o: /usr/include/bits/typesizes.h /usr/include/time.h
wusock.o: /usr/include/endian.h /usr/include/bits/endian.h
wusock.o: /usr/include/sys/select.h /usr/include/bits/select.h
wusock.o: /usr/include/bits/sigset.h /usr/include/bits/time.h
wusock.o: /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h
wusock.o: /usr/include/bits/sched.h /usr/include/bits/uio.h
wusock.o: /usr/include/stdio.h /usr/include/libio.h /usr/include/_G_config.h
wusock.o: /usr/include/wchar.h /usr/include/bits/wchar.h /usr/include/gconv.h
wusock.o: /usr/lib/gcc/i386-redhat-linux/3.4.5/include/stdarg.h
wusock.o: /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h
wusock.o: /usr/include/stdlib.h /usr/include/bits/waitflags.h
wusock.o: /usr/include/bits/waitstatus.h /usr/include/xlocale.h
wusock.o: /usr/include/alloca.h /usr/include/errno.h
wusock.o: /usr/include/bits/errno.h /usr/include/linux/errno.h
wusock.o: /usr/include/asm/errno.h /usr/include/sys/socket.h
wusock.o: /usr/include/bits/socket.h /usr/include/limits.h
wusock.o: /usr/lib/gcc/i386-redhat-linux/3.4.5/include/limits.h
wusock.o: /usr/include/bits/posix1_lim.h /usr/include/bits/local_lim.h
wusock.o: /usr/include/linux/limits.h /usr/include/bits/posix2_lim.h
wusock.o: /usr/include/bits/xopen_lim.h /usr/include/bits/sockaddr.h
wusock.o: /usr/include/asm/socket.h /usr/include/asm/sockios.h
wusock.o: /usr/include/sys/un.h /usr/include/string.h
wusock.o: /usr/include/netinet/in.h /usr/include/stdint.h
wusock.o: /usr/include/bits/in.h /usr/include/bits/byteswap.h
wusock.o: /usr/include/netinet/tcp.h /usr/include/netinet/udp.h
wusock.o: /usr/include/arpa/inet.h /usr/include/netdb.h
wusock.o: /usr/include/rpc/netdb.h /usr/include/bits/siginfo.h
wusock.o: /usr/include/bits/netdb.h /usr/include/unistd.h
wusock.o: /usr/include/bits/posix_opt.h /usr/include/bits/environments.h
wusock.o: /usr/include/bits/confname.h /usr/include/getopt.h
wusock.o: /usr/include/fcntl.h /usr/include/bits/fcntl.h
wusock.o: /usr/include/sys/stat.h /usr/include/bits/stat.h
wusock.o: /usr/include/ctype.h /usr/include/netpacket/packet.h
wusock.o: /usr/include/net/ethernet.h /usr/include/linux/if_ether.h
wusock.o: /usr/include/sys/ioctl.h /usr/include/bits/ioctls.h
wusock.o: /usr/include/asm/ioctls.h /usr/include/asm/ioctl.h
wusock.o: /usr/include/bits/ioctl-types.h /usr/include/sys/ttydefaults.h
wusock.o: /usr/include/net/if.h
wusock.o: /home/arl/student/cgw1/wu_arl/wusrc/wulib/wulog.h
wusock.o: /home/arl/student/cgw1/wu_arl/wusrc/wulib/keywords.h
wusock.o: /home/arl/student/cgw1/wu_arl/wusrc/wulib/wusock.h
wusock.o: /home/arl/student/cgw1/wu_arl/wusrc/wulib/net_util.h
wusock.o: /home/arl/student/cgw1/wu_arl/wusrc/wulib/wunet.h
wusock.o: /usr/include/sys/param.h /usr/include/linux/param.h
wusock.o: /usr/include/asm/param.h
valTypes.o: /usr/include/stdio.h /usr/include/features.h
valTypes.o: /usr/include/sys/cdefs.h /usr/include/gnu/stubs.h
valTypes.o: /usr/lib/gcc/i386-redhat-linux/3.4.5/include/stddef.h
valTypes.o: /usr/include/bits/types.h /usr/include/bits/wordsize.h
valTypes.o: /usr/include/bits/typesizes.h /usr/include/libio.h
valTypes.o: /usr/include/_G_config.h /usr/include/wchar.h
valTypes.o: /usr/include/bits/wchar.h /usr/include/gconv.h
valTypes.o: /usr/lib/gcc/i386-redhat-linux/3.4.5/include/stdarg.h
valTypes.o: /usr/include/bits/stdio_lim.h /usr/include/bits/sys_errlist.h
valTypes.o: /usr/include/stdlib.h /usr/include/bits/waitflags.h
valTypes.o: /usr/include/bits/waitstatus.h /usr/include/endian.h
valTypes.o: /usr/include/bits/endian.h /usr/include/xlocale.h
valTypes.o: /usr/include/sys/types.h /usr/include/time.h
valTypes.o: /usr/include/sys/select.h /usr/include/bits/select.h
valTypes.o: /usr/include/bits/sigset.h /usr/include/bits/time.h
valTypes.o: /usr/include/sys/sysmacros.h /usr/include/bits/pthreadtypes.h
valTypes.o: /usr/include/bits/sched.h /usr/include/alloca.h
valTypes.o: /usr/include/inttypes.h /usr/include/stdint.h
valTypes.o: /home/arl/student/cgw1/wu_arl/wusrc/wulib/wulog.h
valTypes.o: /usr/include/string.h
valTypes.o: /home/arl/student/cgw1/wu_arl/wusrc/wulib/keywords.h
valTypes.o: /home/arl/student/cgw1/wu_arl/wusrc/wulib/valTypes.h
