/* 

  This file is provided under a dual BSD/GPLv2 license.  When using or 
  redistributing this file, you may do so under either license.

  GPL LICENSE SUMMARY

  Copyright(c) 2007-2012 Intel Corporation. All rights reserved.

  This program is free software; you can redistribute it and/or modify 
  it under the terms of version 2 of the GNU General Public License as
  published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but 
  WITHOUT ANY WARRANTY; without even the implied warranty of 
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
  General Public License for more details.

  You should have received a copy of the GNU General Public License 
  along with this program; if not, write to the Free Software 
  Foundation, Inc., 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
  The full GNU General Public License is included in this distribution 
  in the file called LICENSE.GPL.

  Contact Information:

  Intel Corporation
  2200 Mission College Blvd.
  Santa Clara, CA  97052

  BSD LICENSE 

  Copyright(c) 2007-2012 Intel Corporation. All rights reserved.
  All rights reserved.

  Redistribution and use in source and binary forms, with or without 
  modification, are permitted provided that the following conditions 
  are met:

    * Redistributions of source code must retain the above copyright 
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright 
      notice, this list of conditions and the following disclaimer in 
      the documentation and/or other materials provided with the 
      distribution.
    * Neither the name of Intel Corporation nor the names of its 
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

*/


/**

\mainpage Platform Config 

*/
/*@(*/

/** \file platform_config.h */
#ifndef _PLATFORM_CONFIG_DRV_H_
#define _PLATFORM_CONFIG_DRV_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "platform_config.h"

/* DRIVER PRIVATE: initialize the memory layout internal hash table */
config_result_t config_initialize( void );
/* DRIVER PRIVATE: deinitialize memory layout internal hash table */
config_result_t config_deinitialize( void );


/** \def PLATFORM_CONFIG_DEVICE_NAME
    \brief The driver's name
*/
#define PLATFORM_CONFIG_DEVICE_NAME	"platform_config"

#define PLATFORM_CONFIG_DEVICE  "/proc/platform_config"
/* IOCTLs */

/*
*/
#include <asm/ioctl.h>

/** \def PLATFORM_MEMORY_LAYOUT_IOC_MAGIC
    \brief The driver's Magic Number for IOCTLs

    I found no other device using a \b % for their Magic Number.
*/
#define PLATFORM_CONFIG_IOC_MAGIC '%'

/** \def PLATFORM_CONFIG_IOC_GET_INT
    \brief IOCTL number to Get The Integer Value
*/
#define PLATFORM_CONFIG_IOC_GET_INT		_IOR(PLATFORM_CONFIG_IOC_MAGIC, 1, char *)

/** \def PLATFORM_CONFIG_IOC_GET_STR
    \brief IOCTL number to Get The String Value
*/
#define PLATFORM_CONFIG_IOC_GET_STR		_IOR(PLATFORM_CONFIG_IOC_MAGIC, 2, char *)

/** \def PLATFORM_CONFIG_IOC_SET_INT
    \brief IOCTL number to Set The Integer Value
*/
#define PLATFORM_CONFIG_IOC_SET_INT		_IOR(PLATFORM_CONFIG_IOC_MAGIC, 3, char *)

/** \def PLATFORM_CONFIG_IOC_SET_STR
    \brief IOCTL number to Set The String Value
*/
#define PLATFORM_CONFIG_IOC_SET_STR		_IOR(PLATFORM_CONFIG_IOC_MAGIC, 4, char *)

/** \def PLATFORM_CONFIG_IOC_NODE_FIND
    \brief IOCTL number to Find The Specified Node
*/
#define PLATFORM_CONFIG_IOC_NODE_FIND		_IOR(PLATFORM_CONFIG_IOC_MAGIC, 5, char *)

/** \def PLATFORM_CONFIG_IOC_NODE_FIRST_CHILD
    \brief IOCTL number to Find The First Child Node
*/
#define PLATFORM_CONFIG_IOC_NODE_FIRST_CHILD	_IOR(PLATFORM_CONFIG_IOC_MAGIC, 6, char *)

/** \def PLATFORM_CONFIG_IOC_NODE_NEXT_SIBLING
    \brief IOCTL number to Find The Next Sibling Node
*/
#define PLATFORM_CONFIG_IOC_NODE_NEXT_SIBLING 	_IOR(PLATFORM_CONFIG_IOC_MAGIC, 7, char *)

/** \def PLATFORM_CONFIG_IOC_NODE_GET_NAME
    \brief IOCTL number to Get The Name of Specified Node 
*/
#define PLATFORM_CONFIG_IOC_NODE_GET_NAME	_IOR(PLATFORM_CONFIG_IOC_MAGIC, 8, char *)

/** \def PLATFORM_CONFIG_IOC_NODE_GET_STR
    \brief IOCTL number to Get The String Value of Specified Node
*/
#define PLATFORM_CONFIG_IOC_NODE_GET_STR	_IOR(PLATFORM_CONFIG_IOC_MAGIC, 9, char *)

/** \def PLATFORM_CONFIG_IOC_NODE_GET_INT
    \brief IOCTL number to Get the Int Value of Specified Node
*/
#define PLATFORM_CONFIG_IOC_NODE_GET_INT	_IOR(PLATFORM_CONFIG_IOC_MAGIC, 10, char *) 

/** \def PLATFORM_CONFIG_IOC_LOAD
    \brief IOCTL number to Load The Configuration Data
*/
#define PLATFORM_CONFIG_IOC_LOAD		_IOR(PLATFORM_CONFIG_IOC_MAGIC, 11, char *)

/** \def PLATFORM_CONFIG_IOC_INITIALIZE
    \brief IOCTL number to return VERSION STRING of the driver
*/
#define PLATFORM_CONFIG_IOC_INITIALIZE 	_IOR(PLATFORM_CONFIG_IOC_MAGIC, 12, char *)

/** \def PLATFORM_CONFIG_IOC_INITIALIZE
    \brief IOCTL number to return VERSION STRING of the driver
*/
#define PLATFORM_CONFIG_IOC_UNINITIALIZE 	_IOR(PLATFORM_CONFIG_IOC_MAGIC, 13, char *)

/** \def PLATFORM_CONFIG_IOC_LOAD
    \brief IOCTL number to Load The Configuration Data
*/
#define PLATFORM_CONFIG_IOC_REMOVE		_IOR(PLATFORM_CONFIG_IOC_MAGIC, 14, char *)

struct plat_cfg_ioctl {
	config_ref_t	base_ref;
	const char *	const_name;
	char *   		name;
	const char * 	const_string;
	char *   		string;
	int  			val;	
	int * 			val_ptr;	
	const char *	config_data;
	size_t 			bufsize;
	config_ref_t *	node_ptr;
};

/*@)*/

#ifdef __cplusplus
}
#endif

#endif /* _PLATFORM_CONFIG_DRV_H_ */

