include ../Makefile.inc

DEBUG = -g

CXXFLAGS = ${DEBUG} -DPTHREADS -D_GNU_SOURCE -D_REENTRANT -D_THREAD_SAFE -W -Wall -Wundef -Wimplicit -Wno-deprecated -Woverloaded-virtual 
INCLUDES = -I. -I../shared

LIBS = -lrt -pthread -L../shared/${OBJDIR} -lONLshared -lrt

EXEC = $(OBJDIR)/keeboot
OBJS = $(OBJDIR)/keeboot_requests.o \
       $(OBJDIR)/keeboot_main.o

all: ${OBJDIR} ${OBJS} ${EXEC}

${OBJDIR} :
	mkdir ${OBJDIR}

$(OBJS) : ${OBJDIR}/%.o : %.cc
	${CXX} ${CXXFLAGS} ${INCLUDES} -o $@ -c $<

$(EXEC) : ${OBJDIR}/% : ${OBJS}
	${CXX} -o $@ ${OBJS} ${LIBS}
	chmod 0755 $@

ifeq ($(OBJDIR), Linux26.32)
install:
	scp ${EXEC} root@onlsrv:/root/keeboot/32bit
else
install:
	scp ${EXEC} root@onlsrv:/root/keeboot/64bit
endif

clean:
	rm -f ${OBJDIR}/*
