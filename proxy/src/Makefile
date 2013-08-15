#
# $Source: /project/arlcvs/cvsroot/wu_arl/openNetLab/proxy/Makefile,v $
# $Author: fredk $
# $Date: 2008/09/03 13:38:07 $
# $Revision: 1.15 $
#
# Author:       Fred Kuhns
#               fredk@arl.wustl.edu
# Organization: Applied Research Laboratory
#               Washington UNiversity in St. Louis
#
WUSRC = ../wusrc
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

