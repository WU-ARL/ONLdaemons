# Copyright (c) 2009-2013 Charlie Wiseman, Jyoti Parwatikar, John DeHart 
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

include ../Makefile.inc
include ../../Makefile.inc

DEBUG = -g
CXXFLAGS = ${DEBUG} -DPTHREADS -D_GNU_SOURCE -D_REENTRANT -D_THREAD_SAFE -DWU_RING_OP_BASE -W -Wall -Wundef -Wimplicit -Wno-deprecated -Woverloaded-virtual 
INCLUDES = -I${MVL_TARGET_FS}/usr/include -I${IXA_ME_TOOLS}/include -I${IXA_ME_TOOLS}/XSC_CoreLibs ${WUIXP_USRINC} ${WUIXP_CXXFLAGS} -I${ATCA7010_DIR}/include -I${IXA_SDK_DEV}/src/include

LIBS = -L${ATCA7010_DIR}/api/arm -lppm ${WULDFLAGS} -lpthread ${IXA_ME_TOOLS}/lib_linux_be/halMev2.a ${IXA_ME_TOOLS}/lib_linux_be/libossl.a ${IXA_ME_TOOLS}/lib_linux_be/uclo.a ${IXA_ME_TOOLS}/lib_linux_be/utils.a

EXEC = $(OBJDIR)/load_uc_test
OBJS = $(OBJDIR)/load_uc_test.o

EXEC2 = $(OBJDIR)/init_mem_test
OBJS2 = $(OBJDIR)/init_mem_test.o

EXEC3 = $(OBJDIR)/init_router_test
OBJS3 = $(OBJDIR)/init_router_test.o

EXEC4 = $(OBJDIR)/load_uc
OBJS4 = $(OBJDIR)/load_uc.o

EXEC5 = $(OBJDIR)/read_rings
OBJS5 = $(OBJDIR)/read_rings.o

EXEC6 = $(OBJDIR)/read_sram
OBJS6 = $(OBJDIR)/read_sram.o

EXEC7 = $(OBJDIR)/init_mem_route
OBJS7 = $(OBJDIR)/init_mem_route.o

EXEC8 = $(OBJDIR)/chip_type
OBJS8 = $(OBJDIR)/chip_type.o

EXEC9 = $(OBJDIR)/glob_test
OBJS9 = $(OBJDIR)/glob_test.o

EXEC10 = $(OBJDIR)/add_plugin
OBJS10 = $(OBJDIR)/add_plugin.o

EXEC11 = $(OBJDIR)/misc_test
OBJS11 = $(OBJDIR)/misc_test.o

EXEC12 = $(OBJDIR)/read_dram
OBJS12 = $(OBJDIR)/read_dram.o

EXEC13 = $(OBJDIR)/euid_test
OBJS13 = $(OBJDIR)/euid_test.o

EXEC14 = $(OBJDIR)/write_sram
OBJS14 = $(OBJDIR)/write_sram.o

EXEC15 = $(OBJDIR)/write_scratch
OBJS15 = $(OBJDIR)/write_scratch.o

EXEC16 = $(OBJDIR)/write_dram
OBJS16 = $(OBJDIR)/write_dram.o

CXX=${PREFIX}g++

all: ${OBJDIR} ${OBJS} ${EXEC} ${OBJS2} ${EXEC2} ${OBJS3} ${EXEC3} ${OBJS4} ${EXEC4} ${OBJS5} ${EXEC5} ${OBJS6} ${EXEC6} ${OBJ7} ${EXEC7} ${OBJS8} ${EXEC8} ${OBJS9} ${EXEC9} ${OBJS10} ${EXEC10} ${OBJS11} ${EXEC11} ${OBJS12} ${EXEC12} ${OBJS13} ${EXEC13} ${OBJS14} ${EXEC14} ${OBJS15} ${EXEC15} ${OBJS16} ${EXEC16}

install: ${EXEC} ${EXEC2} ${EXEC3} ${EXEC4} ${EXEC5} ${EXEC6} ${EXEC7} ${EXEC8} ${EXEC9} ${EXEC10} ${EXEC11} ${EXEC12} ${EXEC13} ${EXEC14} ${EXEC15} ${EXEC16}
	scp ${EXEC} ${EXEC2} ${EXEC3} ${EXEC4} ${EXEC5} ${EXEC6} ${EXEC7} ${EXEC8} ${EXEC9} ${EXEC10} ${EXEC11} ${EXEC12} ${EXEC13} ${EXEC14} ${EXEC15} ${EXEC16} root@onlsrv:/export/npr/test_utils

${OBJDIR} :
	mkdir ${OBJDIR}

$(OBJS) : ${OBJDIR}/%.o : %.C
	${CXX} ${CXXFLAGS} ${INCLUDES} -o $@ -c $<

${EXEC} : ${OBJDIR}/% : ${OBJS}
	${CXX} -o $@ ${OBJS} ${LIBS}
	chmod 0755 $@

$(OBJS2) : ${OBJDIR}/%.o : %.C
	${CXX} ${CXXFLAGS} ${INCLUDES} -o $@ -c $<

${EXEC2} : ${OBJDIR}/% : ${OBJS2}
	${CXX} -o $@ ${OBJS2} ${LIBS}
	chmod 0755 $@

$(OBJS3) : ${OBJDIR}/%.o : %.C
	${CXX} ${CXXFLAGS} ${INCLUDES} -o $@ -c $<

${EXEC3} : ${OBJDIR}/% : ${OBJS3}
	${CXX} -o $@ ${OBJS3} ${LIBS}
	chmod 0755 $@

$(OBJS4) : ${OBJDIR}/%.o : %.C
	${CXX} ${CXXFLAGS} ${INCLUDES} -o $@ -c $<

${EXEC4} : ${OBJDIR}/% : ${OBJS4}
	${CXX} -o $@ ${OBJS4} ${LIBS}
	chmod 0755 $@

$(OBJS5) : ${OBJDIR}/%.o : %.C
	${CXX} ${CXXFLAGS} ${INCLUDES} -o $@ -c $<

${EXEC5} : ${OBJDIR}/% : ${OBJS5}
	${CXX} -o $@ ${OBJS5} ${LIBS}
	chmod 0755 $@

$(OBJS6) : ${OBJDIR}/%.o : %.C
	${CXX} ${CXXFLAGS} ${INCLUDES} -o $@ -c $<

${EXEC6} : ${OBJDIR}/% : ${OBJS6}
	${CXX} -o $@ ${OBJS6} ${LIBS}
	chmod 0755 $@

$(OBJS7) : ${OBJDIR}/%.o : %.C
	${CXX} ${CXXFLAGS} ${INCLUDES} -o $@ -c $<

${EXEC7} : ${OBJDIR}/% : ${OBJS7}
	${CXX} -o $@ ${OBJS7} ${LIBS}
	chmod 0755 $@

$(OBJS8) : ${OBJDIR}/%.o : %.C
	${CXX} ${CXXFLAGS} ${INCLUDES} -o $@ -c $<

${EXEC8} : ${OBJDIR}/% : ${OBJS8}
	${CXX} -o $@ ${OBJS8} ${LIBS}
	chmod 0755 $@

$(OBJS9) : ${OBJDIR}/%.o : %.C
	${CXX} ${CXXFLAGS} ${INCLUDES} -o $@ -c $<

${EXEC9} : ${OBJDIR}/% : ${OBJS9}
	${CXX} -o $@ ${OBJS9} ${LIBS}
	chmod 0755 $@

$(OBJS10) : ${OBJDIR}/%.o : %.C
	${CXX} ${CXXFLAGS} ${INCLUDES} -o $@ -c $<

${EXEC10} : ${OBJDIR}/% : ${OBJS10}
	${CXX} -o $@ ${OBJS10} ${LIBS}
	chmod 0755 $@

$(OBJS11) : ${OBJDIR}/%.o : %.C
	${CXX} ${CXXFLAGS} ${INCLUDES} -o $@ -c $<

${EXEC11} : ${OBJDIR}/% : ${OBJS11}
	${CXX} -o $@ ${OBJS11} ${LIBS}
	chmod 0755 $@

$(OBJS12) : ${OBJDIR}/%.o : %.C
	${CXX} ${CXXFLAGS} ${INCLUDES} -o $@ -c $<

${EXEC12} : ${OBJDIR}/% : ${OBJS12}
	${CXX} -o $@ ${OBJS12} ${LIBS}
	chmod 0755 $@

$(OBJS13) : ${OBJDIR}/%.o : %.C
	${CXX} ${CXXFLAGS} ${INCLUDES} -o $@ -c $<

${EXEC13} : ${OBJDIR}/% : ${OBJS13}
	${CXX} -o $@ ${OBJS13} ${LIBS}
	chmod 0755 $@

$(OBJS14) : ${OBJDIR}/%.o : %.C
	${CXX} ${CXXFLAGS} ${INCLUDES} -o $@ -c $<

${EXEC14} : ${OBJDIR}/% : ${OBJS14}
	${CXX} -o $@ ${OBJS14} ${LIBS}
	chmod 0755 $@

$(OBJS15) : ${OBJDIR}/%.o : %.C
	${CXX} ${CXXFLAGS} ${INCLUDES} -o $@ -c $<

${EXEC15} : ${OBJDIR}/% : ${OBJS15}
	${CXX} -o $@ ${OBJS15} ${LIBS}
	chmod 0755 $@

$(OBJS16) : ${OBJDIR}/%.o : %.C
	${CXX} ${CXXFLAGS} ${INCLUDES} -o $@ -c $<

${EXEC16} : ${OBJDIR}/% : ${OBJS16}
	${CXX} -o $@ ${OBJS16} ${LIBS}
	chmod 0755 $@

clean:
	rm -f ${OBJDIR}/*
