#/*
#
#  This file is provided under a dual BSD/GPLv2 license.  When using or
#  redistributing this file, you may do so under either license.
#
#  GPL LICENSE SUMMARY
#
#  Copyright(c) 2008-2012 Intel Corporation. All rights reserved.
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of version 2 of the GNU General Public License as
#  published by the Free Software Foundation.
#
#  This program is distributed in the hope that it will be useful, but
#  WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
#  General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
#  The full GNU General Public License is included in this distribution
#  in the file called LICENSE.GPL.
#
#  Contact Information:
#  intel.com
#  Intel Corporation
#  2200 Mission College Blvd.
#  Santa Clara, CA  95052
#  USA
#  (408) 765-8080
#
#
#  BSD LICENSE
#
#  Copyright(c) 2008-2012 Intel Corporation. All rights reserved.
#  All rights reserved.
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#
#    * Redistributions of source code must retain the above copyright
#      notice, this list of conditions and the following disclaimer.
#    * Redistributions in binary form must reproduce the above copyright
#      notice, this list of conditions and the following disclaimer in
#      the documentation and/or other materials provided with the
#      distribution.
#    * Neither the name of Intel Corporation nor the names of its
#      contributors may be used to endorse or promote products derived
#      from this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
#  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
#  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
#  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
#  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
#  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
#  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
#  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
#
#*/


#-----
# versioning
#-----
# Official build generates the follow version environment variables:
#       COMP_VER1=1
#       COMP_VER2=2
#       COMP_VER3=3
#       COMP_VER4=4
#       COMP_VER=1-2-3-0004
#
# If official build is not being done, we use bogus "development" build number
COMP_VER4 ?= 9999999
COMP_VER  ?= <Unofficial Build>

#EXTRA_CFLAGS += -maccumulate-outgoing-args -O2 -Wall -fno-builtin-memcpy -fno-strict-aliasing 
EXTRA_CFLAGS+= -DLINUX -DVER1=$(COMP_VER1) -DVER2=$(COMP_VER2) -DVER3=$(COMP_VER3) -DVER4=$(COMP_VER4) -DVER="\"$(COMP_VER)\"" -DCOMPONENTNAME="\"$(COMPONENTNAME)\""
#-----------------------------------------------------
# Targets and libraries definition
#-----------------------------------------------------
MOD_NAME  = platform_config
COMPONENT = platform_config_driver
LIB_LIBS := platform_config_core htuple 
#-----------------------------------------------------
# Kbuild compiler flags 
#-----------------------------------------------------
EXTRA_CFLAGS += -I${KERNEL_BUILD_DIR}
EXTRA_CFLAGS += -I${KERNEL_BUILD_DIR}/include
EXTRA_CFLAGS += -maccumulate-outgoing-args
EXTRA_CFLAGS += -I${KERNEL_BUILD_DIR}/include/asm-i386/mach-default
EXTRA_CFLAGS += -m32 -O2 -Wall  -fno-builtin-memcpy -fno-strict-aliasing
EXTRA_CFLAGS += -I$(BUILD_DEST)/include 
EXTRA_CFLAGS += -I$(PWD)/../include/
#-----------------------------------------------------
# Variable and Utilities
#-----------------------------------------------------
TARGET:= $(MOD_NAME).ko
STATIC_LIB_LIST=$(foreach lib, $(LIB_LIBS), extract$(lib).list)

define EXTRACT_ARCHIVE
	@echo 'Making $@ from $+ using $(AR)'
	$(AR) -t $+ > $@
	$(AR) -x $+
endef
#-----------------------------------------------------
# Create kernel modules
#-----------------------------------------------------
ifeq ($(TARG_FMT),i686-linux-elf)
.PHONY: all clean
all:
clean:
else
ifneq ($(KERNELRELEASE),)

obj-m := $(MOD_NAME).o 
$(MOD_NAME)-y += $(COMPONENT).o $(foreach lib, $(LIB_LIBS), $(lib).o)
else

PWD := $(shell pwd)

.PHONY: all clean
ifeq ($(TARG_FMT),i686-linux-kernel)
all:$(TARGET)

$(TARGET):$(STATIC_LIB_LIST)
	make -C $(KERNEL_BUILD_DIR)  M=`pwd` modules

extract%.list : $(BUILD_DEST)/lib/modules/lib%.a
	$(EXTRACT_ARCHIVE)
else
all:
endif
clean:
	rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions *.symvers *.order *.list
endif
endif
