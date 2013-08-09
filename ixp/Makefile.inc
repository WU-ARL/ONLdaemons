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

ONL               = /project/arl/freleng/jp/d/wu_arl/openNetLab
IXP_BASE          = /export
IXP_CROSSBUILD    = ${IXP_BASE}/npr
IXP_LINUXSRC      = linux-2.4.20_mvl31

MVL_TARGET_PREFIX = xscale_be
MVL_ROOT          = ${IXP_BASE}/montavista
MVL_CROSS_ROOT    = ${MVL_ROOT}/pro/devkit/arm/${MVL_TARGET_PREFIX}
MVL_CROSS_BIN     = ${MVL_CROSS_ROOT}/bin

MVL_TARGET_FS     = ${MVL_CROSS_ROOT}/target

MVL_KERNEL_DIR    = ${MVL_ROOT}/pro/devkit/lsp/intel-ixdp2800-arm_xscale_be/${IXP_LINUXSRC}
KERNELDIR         = ${MVL_KERNEL_DIR}

MODINC := -I${KERNELDIR}/include

WUTARGETDIR  = ${IXP_CROSSBUILD}/fs

BUILDTOOL_CC   ?= gcc
BUILDTOOL_CCLD ?= ld

PREFIX := ${MVL_CROSS_BIN}/${MVL_TARGET_PREFIX}-
OBJDIR := ${MVL_TARGET_PREFIX}

WUIXP_CFLAGS   += -DIXP
WUIXP_CXXFLAGS += -DIXP
WUIXP_USRINC    = -I${MVL_TARGET_FS}/usr/include -I${MVL_TARGET_FS}/usr/local/include

WUIXP_LDFLAGS = -L${MVL_TARGET_FS}/lib -L${MVL_TARGET_FS}/usr/local/lib 

IXA_SDK_DEV                = /export/ixa_sdk_4.3

IXP2XXX_TOOLCHAIN_ROOT     = ${KERNELDIR}
IXP2800_KERNEL_SOURCE_ROOT = ${KERNELDIR}
IXP2800_TOOLCHAIN_ROOT     = ${MVL_CROSS_ROOT}
IXA_ME_TOOLS               = ${IXA_SDK_DEV}/me_tools
ATCA7010_DIR               = ${IXA_SDK_DEV}/atca-7010
CONFIG                     = XSCALE_BE
HARDWARE_TYPE              = IXP2800
PLATFORM                   = IXP2800
BUILD_TYPE                 = DEBUG
BUILD_MODE                 = HARDWARE
SYS                        = LINUX
BE                         = 1
IOSTYLE                    = HARDWARE

LINUX_ARCH_DIR ?= arm

WUIXP_CFLAGS   += -DIOSTYLE=${IOSTYLE}
WUIXP_CXXFLAGS += -DIOSTYLE=${IOSTYLE}

#CC     = ${PREFIX}gcc
#CXX    = ${PREFIX}g++
#LD     = ${PREFIX}ld
#AR     = ${PREFIX}ar
#RANLIB = ${PREFIX}ranlib
