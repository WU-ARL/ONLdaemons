#
# $Source: /project/arl/arlcvs/cvsroot/wu_arl/wusrc/wuPP/Makefile,v $
# $Author: fredk $
# $Date: 2008/02/11 16:41:01 $
# $Revision: 1.43 $
#
# Author:       Fred Kuhns
#               fredk@arl.wustl.edu
# Organization: Applied Research Laboratory
#               Washington UNiversity in St. Louis
#
WUSRC = ..
include ../Makefile.inc

# Thread and Synchronization specific flags, for us it
# # will always be PTHREADS
THRFLAGS := -DPTHREADS

#INCLUDES = ${WUINC}
# CXXFLAGS = ${WUDEBUG} -DWUBUF_DEBUG ${WUCXXFLAGS} ${CXXTEMPLATE}
# CXXFLAGS = ${WUDEBUGXX} ${WUCXXFLAGS} ${CXXTEMPLATE}
#CXXFLAGS =  ${WARNSTRICT} ${GNUFLAGS} ${USRFLG} ${WUDEBUG}
# CXXFLAGS += ${WUDEBUGXX}
CXXFLAGS := ${WUCXXFLAGS_DB} ${CXXTEMPLATE} -fpermissive
INCLUDES = ${WUINCLUDES}
# CXXFLAGS = -O2 ${WUCXXFLAGS} ${CXXTEMPLATE}
#CXX      = ${WUCXX}
LIBS     = $(WUCXXLIBS) $(WUCLIBS) -lpthread

HFILES := timer.h addr.h net.h buf.h msg.h handle.h stringUtils.h token.h error.h valBuf.h wusock.h sockInet.h sockUnix.h sockLL.h scanner.h conf.h RangeMap.h
SRCS   := buf.cc addr.cc stringUtils.cc token.cc valBuf.cc error.cc wusock.cc sockInet.cc sockUnix.cc sockLL.cc scanner.cc conf.cc RangeMap.cc sock.cc
OBJS=$(addprefix ${OBJDIR}/, $(patsubst %.cc,%.o,$(SRCS)))

CSRC= bufTest.cc handleTest2.cc
COBJ=$(addprefix ${OBJDIR}/, $(patsubst %.cc,%.o,$(CSRC)))
CMDS=$(addprefix ${OBJDIR}/, $(basename $(CSRC)))

all: $(OBJDIR) $(OBJDIR)/$(WUPLUSNAME) $(CMDS)

ixp :
	@echo "@@@ Building for the IXP target @@@"
	@${MAKE} --no-print-directory WUTARGET="ixp" -e all

ixp-clean :
	@echo "@@@ Cleaning for the IXP target @@@"
	@${MAKE} --no-print-directory WUTARGET="ixp" -e clean

install : all

$(OBJDIR) :
	mkdir $(OBJDIR)

$(OBJS) : ${OBJDIR}/%.o : %.cc Makefile
	${CXX} ${CXXFLAGS} ${INCLUDES} -o $@ -c $<

$(OBJDIR)/$(WUPLUSNAME) : $(OBJS)
	$(AR) rc $@ $(OBJS)
	$(RANLIB) $@

$(COBJ) : ${OBJDIR}/%.o : %.cc ${HFILES} Makefile
	${CXX} ${CXXFLAGS} ${INCLUDES} -o $@ -c $<

$(CMDS) : ${OBJDIR}/% : $(OBJDIR)/%.o $(WULIBOBJ)
	${CXX} ${CXXFLAGS} -o $@ $@.o ${LIBS}
	chmod 0755 $@


clean:
	/bin/rm -f $(OBJDIR)/*

realclean:
	/bin/rm -f *~
	/bin/rm -R $(OBJDIR)

depend:
	makedepend -- $(CXXFLAGS) $(INCLUDES) -- $(SRCS) $(CSRC)

# DO NOT DELETE
