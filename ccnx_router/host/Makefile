include ../../Makefile.inc

DEBUG = -g

CXXFLAGS = ${DEBUG} -DPTHREADS -D_GNU_SOURCE -D_REENTRANT -D_THREAD_SAFE -W -Wall -Wundef -Wimplicit -Wno-deprecated -Woverloaded-virtual 
INCLUDES = -I. -I../../shared

LIBS = -lrt -pthread -L../../shared/${OBJDIR} -lONLshared -lrt

EXEC = $(OBJDIR)/host
OBJS = $(OBJDIR)/host_configuration.o \
       $(OBJDIR)/host_requests.o \
       $(OBJDIR)/host_main.o

all: ${OBJDIR} ${OBJS} ${EXEC}

${OBJDIR} :
	mkdir ${OBJDIR}

$(OBJS) : ${OBJDIR}/%.o : %.cc
	${CXX} ${CXXFLAGS} ${INCLUDES} -o $@ -c $<

$(EXEC) : ${OBJDIR}/% : ${OBJS}
	${CXX} -o $@ ${OBJS} ${LIBS}
	chmod 0755 $@

install:
	scp ${EXEC} root@onlsrv:/users/onl/pc1core

clean:
	rm -f ${OBJDIR}/*
