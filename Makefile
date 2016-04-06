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



SUBDIRS = \
	core \
	common \
	linux_driver \
	docs

include ./Makefile.inc

#------------------------------------------------------------------------
# Utilities 
#------------------------------------------------------------------------
MAKEFILE_NAME:=Makefile
define RECURSE_INTO_SUBDIRS_ELF_KERNEL
	for subdir in $(SUBDIRS); \
		do \
			exit_logic="1";	\
			if [ -f $$subdir/$(MAKEFILE_NAME) ]; then \
				echo "making in $$subdir"; \
                (($(MAKE) \
                    -f $(MAKEFILE_NAME) \
                    -C $$subdir \
		    all	\
                     TARG_FMT=i686-linux-elf )\
                && ($(MAKE) \
                    -f $(MAKEFILE_NAME) \
                    -C $$subdir \
		    all	\
                    TARG_FMT=i686-linux-kernel )) \
                    || exit $$exit_logic; \
			fi ;\
		done
endef

.PHONY: all
all: bld doc test

#------------------------------------------------------------------------
# Build targets
#------------------------------------------------------------------------
.PHONY: bld 
bld:
	@echo ">>>Building platform_config"
	@echo "PATH: $(PATH)"
	@$(RECURSE_INTO_SUBDIRS_ELF_KERNEL)
#------------------------------------------------------------------------
# Install targets
#------------------------------------------------------------------------
.PHONY: install
install: install_dev install_target
	@echo ">>>Installed platform_config development and target files"

.PHONY: install_dev
install_dev:
	@echo "Copying platform_config development files to $(BUILD_DEST)"
	@[ -d "$(BUILD_DEST)/lib/modules" ] || mkdir -pv "$(BUILD_DEST)/lib/modules"
	@cp -v linux_driver/platform_config.ko $(BUILD_DEST)/lib/modules
	@[ -d "$(BUILD_DEST)/bin" ] || mkdir -pv "$(BUILD_DEST)/bin"
	@cp -v common/apps/platform_config_app $(BUILD_DEST)/bin
	@cp -v common/config_pmrs/config_pmrs $(BUILD_DEST)/bin
	@[ -d "$(BUILD_DEST)/lib" ] || mkdir -pv "$(BUILD_DEST)/lib"
	@cp -v core/i686-linux-elf/libplatform_config_core.so $(BUILD_DEST)/lib
	@cp -v core/i686-linux-elf/libplatform_config_core.a $(BUILD_DEST)/lib
	@cp -v common/lib/libplatform_config.a $(BUILD_DEST)/lib
	@cp -v common/lib/libplatform_config.so $(BUILD_DEST)/lib
	@[ -d "$(BUILD_DEST)/include" ] || mkdir -pv "$(BUILD_DEST)/include"
	@cp -v include/platform_config.h $(BUILD_DEST)/include
	@cp -v include/platform_config_paths.h $(BUILD_DEST)/include

.PHONY: install_target
install_target:
	@echo "Copying platform_config target files to $(FSROOT)"
	@[ -d "$(FSROOT)/lib/modules" ] || mkdir -pv "$(FSROOT)/lib/modules"
	@cp -v linux_driver/platform_config.ko $(FSROOT)/lib/modules
	@[ -d "$(FSROOT)/bin" ] || mkdir -pv "$(FSROOT)/bin"
	@cp -v common/apps/platform_config_app $(FSROOT)/bin
	@cp -v common/config_pmrs/config_pmrs $(FSROOT)/bin
	@[ -d "$(FSROOT)/lib" ] || mkdir -pv "$(FSROOT)/lib"
	@cp -v core/i686-linux-elf/libplatform_config_core.so $(FSROOT)/lib
	@cp -v common/lib/libplatform_config.so $(FSROOT)/lib
	

#------------------------------------------------------------------------
# Build debug version targets
#------------------------------------------------------------------------
.PHONY: debug 
debug: dbld doc test

.PHONY: dbld 
dbld:
	@echo ">>>Building debug version of platform_config"
	@echo ">>>Debug version is same as normal version"
	@echo ">>>Building platform_config"
	@echo "PATH: $(PATH)"
	@$(RECURSE_INTO_SUBDIRS_ELF_KERNEL)

.PHONY: doc
doc:
	@echo ">>>Building doc for platform_config"
	make -C docs

.PHONY: doc-clean
doc-clean:
	@echo ">>>Clean doc for platform_config"
	make clean -C docs

.PHONY: test
test:
	@echo ">>>Do nothing"

#------------------------------------------------------------------------
# Clean targets
#------------------------------------------------------------------------
.PHONY: clean
clean:
	@echo ">>>Cleaning platform_config"
	$(foreach SUBDIR, $(SUBDIRS), $(MAKE) -C $(SUBDIR) clean;)
	make uninstall

.PHONY: uninstall
uninstall: uninstall_dev uninstall_target
	@echo "Uninstalled platform_config development and target files"

uninstall_dev:
	@echo "Removing platform_config development files from $(BUILD_DEST)"
	@rm -f $(BUILD_DEST)/lib/modules/platform_config.ko
	@rm -f $(BUILD_DEST)/bin/platform_config_app
	@rm -f $(BUILD_DEST)/bin/config_pmrs
	@rm -f $(BUILD_DEST)/lib/libplatform_config_core.so
	@rm -f $(BUILD_DEST)/lib/libplatform_config.so
	@rm -f $(BUILD_DEST)/lib/libplatform_config.a
	@rm -f $(BUILD_DEST)/lib/libplatform_config_core.a


uninstall_target:
	@echo "Removing platform_config target files from $(FSROOT)"
	@rm -f $(FSROOT)/lib/modules/platform_config.ko
	@rm -f $(FSROOT)/bin/platform_config_app
	@rm -f $(FSROOT)/bin/config_pmrs
	@rm -f $(FSROOT)/lib/libplatform_config_core.so
	@rm -f $(FSROOT)/lib/libplatform_config.so
