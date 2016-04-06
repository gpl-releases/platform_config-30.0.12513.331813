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

#ifdef __KERNEL__
#include <linux/types.h>
#include <linux/slab.h>
#else
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#endif

#include "platform_config.h"
#include "htuple.h"
#ifdef VER
const char *platform_config_version_string = "#@# libplatform_config_core.so " VER;
#endif
/* -------------------------------------------------------------------------------- */
/* CONFIG PUBLIC API */
/* -------------------------------------------------------------------------------- */

/* Start at the specified reference node, locate the sub-node with the specified name, and return a reference to that node. */
config_result_t config_node_find( config_ref_t base_ref, const char *name, config_ref_t *node_ref )
{
	if( (*node_ref = htuple_find_child( base_ref, name, strlen(name) )) )
		return CONFIG_SUCCESS ;
	else
		return CONFIG_ERR_INVALID_REFERENCE ;
}

/* Find the first child of the specified reference node, and return a reference to that child. */
config_result_t config_node_first_child( config_ref_t node_ref, config_ref_t *child_ref )
{
	if( (*child_ref = htuple_first_child( node_ref )) )
		return CONFIG_SUCCESS;
	else 
		return CONFIG_ERR_INVALID_REFERENCE;	
}

/* Find the first child of the specified reference node, and return a reference to that child. */
config_result_t config_node_next_sibling( config_ref_t node_ref, config_ref_t *child_ref )
{
	if( (*child_ref = htuple_next_sibling( node_ref )) )
		return CONFIG_SUCCESS;
	else 
		return CONFIG_ERR_INVALID_REFERENCE;	
}

/* Return the name of the specified reference node. */
config_result_t config_node_get_name( config_ref_t node_ref, char *name, size_t bufsize )
{
	config_result_t err = CONFIG_SUCCESS;
	const char *val;

	err = htuple_node_name( node_ref,&val );
	if( CONFIG_SUCCESS == err ) strncpy ( name, val, bufsize );	

    return (err);
}

/* Return the integer value associated with the specified reference node. */
config_result_t config_node_get_int( config_ref_t node_ref, int *val )
{
	config_result_t err = CONFIG_SUCCESS;
	
	err = htuple_node_int_value( node_ref, val );
    
	return (err);
}

/* Return the string value associated with the specified reference node. */
config_result_t config_node_get_str( config_ref_t node_ref, char *string, size_t bufsize )
{
	config_result_t err = CONFIG_SUCCESS;
	const char *val;

	err = htuple_node_str_value( node_ref, &val );
	if( CONFIG_SUCCESS == err ) strncpy( string, val, bufsize );	

	return (err);
}

/* Start at the specified reference node, locate the sub-node with the specified name, and return the integer value associated with that name. */
config_result_t config_get_int( config_ref_t base_ref, const char *name, int *val )
{   
	int         err = CONFIG_SUCCESS;

    err = htuple_get_int_value( base_ref, name, strlen(name), val );

    return(err);
}

/* Start at the specified reference node, locate the sub-node with the specified name, and return the string value associated with that name. */
config_result_t config_get_str( config_ref_t base_ref, const char *name, char *string, size_t bufsize ) 
{
    int         err = CONFIG_SUCCESS;
    const char  *val;

    err = htuple_get_str_value( base_ref, name, strlen(name), &val );
    if ( CONFIG_SUCCESS == err ) strncpy( string, val, bufsize );

    return(err);
}

/* Start at the specified reference node, locate or create the sub-node with the specified name, and assign the integer value to that name. */
config_result_t config_set_int( config_ref_t base_ref, const char *name, int val )
{
	config_result_t err = CONFIG_SUCCESS;

	err = htuple_set_int_value( base_ref, name, strlen(name), val );

	return err;
}

/* Start at the specified reference node, locate or create the sub-node with the specified name, and assign the string value to that name. */
config_result_t config_set_str( config_ref_t base_ref, const char *name, const char *string, size_t bufsize ) 
{
	config_result_t err = CONFIG_SUCCESS;

	err = htuple_set_str_value( base_ref, name, strlen(name), string, strlen(string) );

	return (err);
}

/* Parse the specified string of configuration data and insert it into the dictionary at the specified reference node. */
config_result_t config_load( config_ref_t base_ref, const char *config_data, size_t datalength )
{
	config_result_t err = CONFIG_SUCCESS;

	err =  htuple_parse_config_string( base_ref, config_data, datalength );

	return (err);
}

/* Parse the specified string of configuration data and insert it into the dictionary at the specified reference node. */
config_result_t config_private_tree_remove( config_ref_t base_ref )
{
	config_result_t err = CONFIG_SUCCESS;

	err = htuple_delete_private_tree( base_ref );


	return (err);
}


/* initialize the memory layout internal hash table */
config_result_t config_initialize( void )
{
	if (htuple_initialize())
		return CONFIG_SUCCESS;
	else
		return CONFIG_ERR_INITIALIZE_FAILED;
}

/* deinitialize memory layout internal hash table */
config_result_t config_deinitialize(void)
{
	if (htuple_deinitialize())
		return CONFIG_SUCCESS;
	else
		return CONFIG_ERR_DEINITIALIZE_FAILED;
}
