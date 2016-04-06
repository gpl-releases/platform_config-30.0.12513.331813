#/*
#
#  This file is provided under a dual BSD/GPLv2 license.  When using or
#  redistributing this file, you may do so under either license.
#
#  GPL LICENSE SUMMARY
#
#  Copyright(c) 2007-2012 Intel Corporation. All rights reserved.
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
#  Copyright(c) 2007-2012 Intel Corporation. All rights reserved.
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

########################################################################
#
# Intel Corporation Proprietary Information
# Copyright (c) 2007-2012 Intel Corporation. All rights reserved.
#
# This listing is supplied under the terms of a license agreement
# with Intel Corporation and may not be used, copied, nor disclosed
# except in accordance with the terms of that agreement.
#
########################################################################
#export COMPONENTNAME ?= libplatform_config_core.so
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
COMP_VER  ?= [Unofficial_Build]
CFLAGS += -DLINUX -DVER1=$(COMP_VER1) -DVER2=$(COMP_VER2) -DVER3=$(COMP_VER3) -DVER4=$(COMP_VER4) -DVER="\"$(COMP_VER)\"" -DCOMPONENTNAME="\"$(COMPONENTNAME)\""


#-----------------------------------------------------
# Compiling flags are different for user space and 
# kernel space library 
#-----------------------------------------------------
ifeq ($(TARG_FMT),i686-linux-elf)
#CFLAGS += -I$(BUILD_DEST)/include
CFLAGS += -Wimplicit -Wreturn-type -m32  -O2 
CFLAGS += -I../include/  -I$(BUILD_DEST)/include 

else
CFLAGS += -MMD -MF i686-linux-kernel//.platform_config_core.d 
CFLAGS += -I${KERNEL_BUILD_DIR} -I${KERNEL_BUILD_DIR}/include 
CFLAGS += -maccumulate-outgoing-args 
CFLAGS += -I${KERNEL_BUILD_DIR}/include/asm-i386/mach-default 
CFLAGS += -m32 -march=i686 -mpreferred-stack-boundary=2 -mregparm=3 -msoft-float -pipe -D__KERNEL__ -ffreestanding -fno-common -fomit-frame-pointer  -fno-stack-protector -Wundef -Wno-trigraphs -Wno-pointer-sign -DMODULE -DCONFIG_AS_CFI=1 -DCONFIG_AS_CFI_SIGNAL_FRAME=1 -fno-strict-aliasing -Wdeclaration-after-statement -Wall -Wstrict-prototypes -DKBUILD_MODNAME="__stringify()" -DKBUILD_BASENAME="__stringify()" 
CFLAGS += -include ${KERNEL_BUILD_DIR}/include/linux/autoconf.h 
CFLAGS += -I${KERNEL_BUILD_DIR}/arch/x86/include/asm/mach-default -I/${KERNEL_BUILD_DIR}/arch/x86/include  
CFLAGS += -O2  -Wall -fno-builtin-memcpy -fno-strict-aliasing  -D_INTERNAL_PCI_ -DUSER=root -DCOMP_VER=[Unofficial_Build] 
CFLAGS += -I. -I$(BUILD_DEST)/include/linux 

endif
CFLAGS += -I../include/  -I$(BUILD_DEST)/include 
CFLAGS += -I$(BUILD_ROOT)/htuple


COMPONENT = platform_config_core
 
OUT_DIR = $(TARG_FMT)
STATIC_LIB_OBJ_PVT = $(COMPONENT).o
SHARE_LIB_OBJ_PVT = $(patsubst %.o,%.pic.o,$(STATIC_LIB_OBJ_PVT))

COMP_LIB_NAME = lib$(COMPONENT)
SHARE_LIB_NAME=$(COMP_LIB_NAME).so
STATIC_LIB_NAME=$(COMP_LIB_NAME).a

#-----------------------------------------------------
# Utilities 
#-----------------------------------------------------
define CREATE_DEST_DIR
	@if [ ! -d $(dir $@) ] ; then \
		echo Creating: $(dir $@) ;\
		mkdir -p $(dir $@) ;\
	fi
endef
#-----------------------------------------------------
# Targets 
#-----------------------------------------------------
.PHONY: all clean
$(OUT_DIR)/$(STATIC_LIB_NAME): $(OUT_DIR)/$(STATIC_LIB_OBJ_PVT)
	@echo 'shared $(OUT_DIR)/$(SHARE_LIB_OBJ_PVT)'
	cd $(OUT_DIR);	\
	$(AR) $(ARFLAGS) $(STATIC_LIB_NAME) $(notdir $^)
ifeq ($(TARG_FMT),i686-linux-kernel)
	@install -D -m 755 $@ $(BUILD_DEST)/lib/modules	
endif
$(OUT_DIR)/$(SHARE_LIB_NAME): $(OUT_DIR)/$(SHARE_LIB_OBJ_PVT)
	cd $(OUT_DIR);	\
	$(CC) -shared  $(notdir $^) -o $(notdir $@)

all: $(OUT_DIR)/$(STATIC_LIB_NAME) $(OUT_DIR)/$(SHARE_LIB_NAME)
#-----------------------------------------------------
# Rules 
#-----------------------------------------------------
$(OUT_DIR)/%.o: %.c
	@$(CREATE_DEST_DIR)
	$(CC) -c $(CFLAGS) -o $@ $<

$(OUT_DIR)/%.pic.o: %.c
	@$(CREATE_DEST_DIR)
	$(CC) -c $(CFLAGS) -fPIC -fpic -o $@ $<

clean: 
	rm -rf *.o *.so i686-linux-elf i686-linux-kernel $(BUILD_DEST)/lib/modules/$(STATIC_LIB_NAME)

