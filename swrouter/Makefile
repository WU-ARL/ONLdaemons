include ../Makefile.inc

DEBUG = -g

CXXFLAGS = ${DEBUG} -DPTHREADS -D_GNU_SOURCE -D_REENTRANT -D_THREAD_SAFE -W -Wall -Wundef -Wimplicit -Wno-deprecated -Woverloaded-virtual 

INCLUDES = -I. -I../shared 

LIBS = -lrt -pthread -L../shared/${OBJDIR} -lONLshared 

EXEC = $(OBJDIR)/swrd
OBJS = $(OBJDIR)/swrd_configuration.o \
       $(OBJDIR)/swrd_requests.o \
       $(OBJDIR)/swrd_main.o \
       $(OBJDIR)/swrd_monitor.o 

all: ${OBJDIR} ${OBJS} ${EXEC}


${OBJDIR} :
	mkdir ${OBJDIR}

$(OBJS) : ${OBJDIR}/%.o : %.cc
	${CXX} ${CXXFLAGS} ${INCLUDES} -o $@ -c $<

$(EXEC) : ${OBJDIR}/% : ${OBJS}
	${CXX} -o $@ ${OBJS} ${LIBS}
	chmod 0755 $@

clean:
	rm -f ${OBJDIR}/*
