/***********************************************************************
  This file is provided under a dual BSD/LGPLv2.1 license.  When using
  or redistributing this file, you may do so under either license.

  LGPL LICENSE SUMMARY

  Copyright(c) <2007-2011>. Intel Corporation. All rights reserved.

  This program is free software; you can redistribute it and/or modify
  it under the terms of version 2.1 of the GNU Lesser General Public
  License as published by the Free Software Foundation.

  This library is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
  USA. The full GNU Lesser General Public License is included in this
  distribution in the file called LICENSE.LGPL.

  Contact Information:
      Intel Corporation
      2200 Mission College Blvd.
      Santa Clara, CA  97052

  BSD LICENSE

  Copyright (c) <2007-2011>. Intel Corporation. All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions
  are met:

    - Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    - Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in
      the documentation and/or other materials provided with the
      distribution.
    - Neither the name of Intel Corporation nor the names of its
      contributors may be used to endorse or promote products derived
      from this software without specific prior written permission.

  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

************************************************************************/
/*
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
*/

#include <fcntl.h>
#include <sys/ioctl.h>

#include "platform_config_drv.h"
#ifdef VER
const char *platform_config_lib_version_string = "#@# libplatform_config.so " VER;
#endif

static int pc_handle=-1;
static struct plat_cfg_ioctl ioctl_args;

/* Start at the specified reference node, locate the sub-node with the specified name, and return the integer value associated with that name. */
config_result_t config_get_int( config_ref_t base_ref, const char *name, int *val )
{

	if ( pc_handle < 0)
    {
//  	OS_DEBUG("platform_config: Not initialized\n");
        return CONFIG_ERR_NOT_INITIALIZED;
    }

    // configure the platform_memory_layout based on user request
    ioctl_args.base_ref 	= base_ref;
	ioctl_args.const_name	= name;
	ioctl_args.val_ptr		= val;	

    // configure the platform_memory_layout based on user request
    if (ioctl(pc_handle, PLATFORM_CONFIG_IOC_GET_INT, &ioctl_args) < 0)
    {
    	return CONFIG_ERR_INVALID_REFERENCE;
    }

    return CONFIG_SUCCESS;
}

/* Start at the specified reference node, locate the sub-node with the specified name, and return the string value associated with that name. */
config_result_t config_get_str( config_ref_t base_ref, const char *name, char *string, size_t bufsize ) 
{

	if ( pc_handle < 0)
    {
//  	OS_DEBUG("platform_config: Not initialized\n");
        return CONFIG_ERR_NOT_INITIALIZED;
    }

    // configure the platform_memory_layout based on user request
    ioctl_args.base_ref 	= base_ref;
	ioctl_args.const_name	= name;
	ioctl_args.string		= string;
	ioctl_args.bufsize		= bufsize;

    // configure the platform_memory_layout based on user request
    if (ioctl(pc_handle, PLATFORM_CONFIG_IOC_GET_STR, &ioctl_args) < 0)
    {
		return CONFIG_ERR_INVALID_REFERENCE;
    }

    return CONFIG_SUCCESS;
}

/* Start at the specified reference node, locate or create the sub-node with the specified name, and assign the integer value to that name. */
config_result_t config_set_int( config_ref_t base_ref, const char *name, int val )
{

	if ( pc_handle < 0)
    {
//  	OS_DEBUG("platform_config: Not initialized\n");
        return CONFIG_ERR_NOT_INITIALIZED;
    }

    // configure the platform_memory_layout based on user request
    ioctl_args.base_ref 	= base_ref;
	ioctl_args.const_name	= name;
	ioctl_args.val			= val;	

    // configure the platform_memory_layout based on user request
    if (ioctl(pc_handle, PLATFORM_CONFIG_IOC_SET_INT, &ioctl_args) < 0)
    {
    	return CONFIG_ERR_INVALID_REFERENCE;
    }

        return CONFIG_SUCCESS;
}

/* Start at the specified reference node, locate or create the sub-node with the specified name, and assign the string value to that name. */
config_result_t config_set_str( config_ref_t base_ref, const char *name, const char *string, size_t bufsize ) 
{
	if ( pc_handle < 0)
    {
//  	OS_DEBUG("platform_config: Not initialized\n");
        return CONFIG_ERR_NOT_INITIALIZED;
    }

    // configure the platform_memory_layout based on user request
    ioctl_args.base_ref 	= base_ref;
	ioctl_args.const_name	= name;
	ioctl_args.const_string	= string;
	ioctl_args.bufsize		= bufsize;	

    // configure the platform_memory_layout based on user request
    if (ioctl(pc_handle, PLATFORM_CONFIG_IOC_SET_STR, &ioctl_args) < 0)
    {
    	return CONFIG_ERR_INVALID_REFERENCE;
    }

    return CONFIG_SUCCESS;
}

/* Start at the specified reference node, locate the sub-node with the specified name, and return a reference to that node. */
config_result_t config_node_find( config_ref_t base_ref, const char *name, config_ref_t *node_ref )
{
	if ( pc_handle < 0)
    {
//  	OS_DEBUG("platform_config: Not initialized\n");
        return CONFIG_ERR_NOT_INITIALIZED;
    }

    // configure the platform_memory_layout based on user request
    ioctl_args.base_ref 	= base_ref;
	ioctl_args.const_name	= name;
	ioctl_args.node_ptr		= node_ref;	

    // configure the platform_memory_layout based on user request
    if (ioctl(pc_handle, PLATFORM_CONFIG_IOC_NODE_FIND, &ioctl_args) < 0)
    {
    	return CONFIG_ERR_INVALID_REFERENCE;
    }

    return CONFIG_SUCCESS;
}

/* Find the first child of the specified reference node, and return a reference to that child. */
config_result_t config_node_first_child( config_ref_t node_ref, config_ref_t *child_ref )
{

	if ( pc_handle < 0)
    {
//  	OS_DEBUG("platform_config: Not initialized\n");
        return CONFIG_ERR_NOT_INITIALIZED;
    }

    // configure the platform_memory_layout based on user request
    ioctl_args.base_ref 	= node_ref;
	ioctl_args.node_ptr		= child_ref;	

    // configure the platform_memory_layout based on user request
    if (ioctl(pc_handle, PLATFORM_CONFIG_IOC_NODE_FIRST_CHILD, &ioctl_args) < 0)
    {
    	return CONFIG_ERR_INVALID_REFERENCE;
    }

    return CONFIG_SUCCESS;
}

/* Find the next sibling of the specified reference node, and return a reference to that sibling. */
config_result_t config_node_next_sibling( config_ref_t node_ref, config_ref_t *child_ref )
{

	if ( pc_handle < 0)
    {
//  	OS_DEBUG("platform_config: Not initialized\n");
        return CONFIG_ERR_NOT_INITIALIZED;
    }

    // configure the platform_memory_layout based on user request
    ioctl_args.base_ref 	= node_ref;
	ioctl_args.node_ptr		= child_ref;	

    // configure the platform_memory_layout based on user request
    if (ioctl(pc_handle, PLATFORM_CONFIG_IOC_NODE_NEXT_SIBLING, &ioctl_args) < 0)
    {
    	return CONFIG_ERR_INVALID_REFERENCE;
    }

   	return CONFIG_SUCCESS;
}

/* Return the name of the specified reference node. */
config_result_t config_node_get_name( config_ref_t node_ref, char *name, size_t bufsize )
{

	if ( pc_handle < 0)
    {
//  	OS_DEBUG("platform_config: Not initialized\n");
        return CONFIG_ERR_NOT_INITIALIZED;
    }

    // configure the platform_memory_layout based on user request
    ioctl_args.base_ref 	= node_ref;
	ioctl_args.name			= name;
	ioctl_args.bufsize		= bufsize;

    // configure the platform_memory_layout based on user request
    if (ioctl(pc_handle, PLATFORM_CONFIG_IOC_NODE_GET_NAME, &ioctl_args) < 0)
    {
    	return CONFIG_ERR_INVALID_REFERENCE;
    }

    return CONFIG_SUCCESS;
}

/* Return the string value associated with the specified reference node. */
config_result_t config_node_get_str( config_ref_t node_ref, char *string, size_t bufsize )
{

	if ( pc_handle < 0)
    {
//  	OS_DEBUG("platform_config: Not initialized\n");
        return CONFIG_ERR_NOT_INITIALIZED;
    }

    // configure the platform_memory_layout based on user request
    ioctl_args.base_ref 	= node_ref;
	ioctl_args.string		= string;
	ioctl_args.bufsize		= bufsize;
        
	// configure the platform_memory_layout based on user request
    if (ioctl(pc_handle, PLATFORM_CONFIG_IOC_NODE_GET_STR, &ioctl_args) < 0)
    {
    	return CONFIG_ERR_INVALID_REFERENCE;
    }

    return CONFIG_SUCCESS;
}

/* Return the integer value associated with the specified reference node. */
config_result_t config_node_get_int( config_ref_t node_ref, int *val )
{

	if ( pc_handle < 0)
    {
//  	OS_DEBUG("platform_config: Not initialized\n");
        return CONFIG_ERR_NOT_INITIALIZED;
    }

    // configure the platform_memory_layout based on user request
    ioctl_args.base_ref 	= node_ref;
	ioctl_args.val_ptr		= val;	

    // configure the platform_memory_layout based on user request
    if (ioctl(pc_handle, PLATFORM_CONFIG_IOC_NODE_GET_INT, &ioctl_args) < 0)
    {
    	return CONFIG_ERR_INVALID_REFERENCE;
    }

    return CONFIG_SUCCESS;
}

/* Return the integer value associated with the specified reference node. */
config_result_t config_node_tree_remove( config_ref_t node_ref )
{

	if ( pc_handle < 0)
    {
//  	OS_DEBUG("platform_config: Not initialized\n");
        return CONFIG_ERR_NOT_INITIALIZED;
    }

    // configure the platform_memory_layout based on user request
    ioctl_args.base_ref 	= node_ref;

    // configure the platform_memory_layout based on user request
    if (ioctl(pc_handle, PLATFORM_CONFIG_IOC_REMOVE, &ioctl_args) < 0)
    {
    	return CONFIG_ERR_INVALID_REFERENCE;
    }

    return CONFIG_SUCCESS;
}

/* Parse the specified string of configuration data and insert it into the dictionary at the specified reference node. */
config_result_t config_load( config_ref_t base_ref, const char *config_data, size_t datalength )
{

	if ( pc_handle < 0)
    {
//  	OS_DEBUG("platform_config: Not initialized\n");
       	return CONFIG_ERR_NOT_INITIALIZED;
    }

    // configure the platform_memory_layout based on user request
    ioctl_args.base_ref 	= base_ref;
	ioctl_args.config_data	= config_data;	
	ioctl_args.bufsize		= datalength;	

    // configure the platform_memory_layout based on user request
    if (ioctl(pc_handle, PLATFORM_CONFIG_IOC_LOAD, &ioctl_args) < 0)
    {
    	return CONFIG_ERR_INVALID_REFERENCE;
    }

    return CONFIG_SUCCESS;
}

/* initialize memory layout internal hash table */
config_result_t config_initialize( void )
{
	if (pc_handle > 0)
	{
		return CONFIG_SUCCESS;
	}
//	pc_handle = open("/proc/platform_config", O_RDWR);
	pc_handle = open(PLATFORM_CONFIG_DEVICE, O_RDWR);

	if ( pc_handle < 0)
    {
                printf("Error opening /proc/platform_config\n");
                printf("Please ensure that:\n");
                printf("        -The platform_config driver is properly loaded\n");
		return CONFIG_ERR_NOT_INITIALIZED;
	}

    // configure the platform_memory_layout based on user request
    if (ioctl(pc_handle, PLATFORM_CONFIG_IOC_INITIALIZE, &ioctl_args) < 0)
    {
    	return CONFIG_ERR_INVALID_REFERENCE;
    }

    return CONFIG_SUCCESS;
}

/* Shared code library initialization hacks */
__attribute__((constructor)) static void platform_config_usermode_lib_init(void)
{
    config_initialize();
}

 __attribute__((destructor)) static void platform_config_usermode_lib_deinit(void)
{
}
