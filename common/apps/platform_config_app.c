/***********************************************************************
  This file is provided under a dual BSD/LGPLv2.1 license.  When using
  or redistributing this file, you may do so under either license.

  LGPL LICENSE SUMMARY

  Copyright(c) <2007-2012>. Intel Corporation. All rights reserved.

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

  Copyright (c) <2007-2012>. Intel Corporation. All rights reserved.

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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "platform_config.h"
#include "platform_config_lib.h"
#ifdef VER
const char *platform_config_version_string = "#@# platform_config_app " VER;
#endif
static void config_verbose_printf( config_ref_t id, int level )
{
    char          name[ BUFSIZE + 1 ], val[ BUFSIZE + 1 ];
    int                  i;
    int                  ival;
    config_ref_t         child;
    config_ref_t         next_sibling;

	name[ BUFSIZE ] = '\0';
	val[ BUFSIZE ] = '\0';

    for ( i = 0; i < level; i++ )
        printf("  ");
    config_node_get_name( id, name, BUFSIZE );

    if ( CONFIG_SUCCESS == config_node_get_int( id, &ival ) )
    {
        printf("\"%s\" = 0x%08x /* id %d */\n", name, ival, id);
    }
    else if ( CONFIG_SUCCESS == config_node_get_str( id, val, BUFSIZE ) )
    {
        printf("\"%s\" = \"%s\" /* id %d */\n", name, val, id );
    }

    config_node_first_child( id , &child);

    if ( 0 != child  )
    {
        for ( i = 0; i < level; i++ )
            printf("  ");
        printf("{\n");

        level++;

        do
        {
            config_verbose_printf( child, level );

            config_node_next_sibling( child , &next_sibling );

            child = next_sibling ;

        } while ( 0 != child );

        level--;

        for ( i = 0; i < level; i++ )
            printf("  ");
        printf("}\n");
    }
}

static config_result_t load_config_file( config_ref_t id, const char *filename )
{
    int          retval = CONFIG_ERR_NO_RESOURCES;
    FILE        *fp;

    if ( NULL != (fp = fopen(filename, "r")) )
    {
        char            *txt;
        long             txtlen;

        fseek( fp, 0, SEEK_END );
        txtlen = ftell(fp);
        rewind(fp);
        if ( NULL != (txt = malloc(txtlen+1)) )
        {
            if ( txtlen == (long) fread( txt, 1, txtlen, fp ) )
            {
                /* add an (unnecessary) null term */
                txt[txtlen] = '\0';

                retval = config_load( id, txt, txtlen );
            }

            free(txt);
        }

        fclose(fp);
    }
    else
    {
        printf("could not open file \"%s\"\n", filename );
    }

    return(retval);
}

static config_result_t execute_config_commands( config_ref_t id )
{
    config_result_t     retval = CONFIG_SUCCESS;
    config_ref_t        node;

    if ( CONFIG_SUCCESS == config_node_first_child( id, &node ) )
    {
        do
        {
            static char            action[64];

            action[63] = '\0';

            if ( CONFIG_SUCCESS == config_get_str( node, "action", action, sizeof(action) - 1 ) )
            {
                if ( ! strcmp( action, "load" ) )
                {
                    config_ref_t            loc_id;
                    static char             bigstr[1024];

                    loc_id = ROOT_NODE;
                    bigstr[1023] = '\0';

                    #if 0
                    /* this test loads a dictionary to a location */
                    {   action      "load"
                        location    "platform.memory.layout"
                        filename    "/etc/platform_config/memory_layout_128M.hcfg"
                        // filename    "/etc/platform_config/memory_layout_256M.hcfg"
                        // filename    "/etc/platform_config/memory_layout_512M.hcfg"
                    }
                    #endif

                    /* Location specified? */
                    if ( CONFIG_SUCCESS == config_get_str( node, "location", bigstr, sizeof(bigstr) - 1) )
                    {
                        /* Does the target location exist already? */
                        if ( CONFIG_SUCCESS != config_node_find( ROOT_NODE, bigstr, &loc_id ) )
                        {
                            /* NO?: Create the location */
                            if ( (CONFIG_SUCCESS == config_set_int( ROOT_NODE, bigstr, 0 )) &&
                                 (CONFIG_SUCCESS == config_node_find( ROOT_NODE, bigstr, &loc_id )) )
                            {
                                ;
                            }
                            else
                            {
                                printf("ERR: could not find/create config database location \"%s\"\n", bigstr );
                                retval = CONFIG_ERR_NOT_FOUND;
                            }
                        }
                    }

                    if ( CONFIG_SUCCESS == config_get_str( node, "filename", bigstr, sizeof(bigstr) - 1) )
                    {
                        retval = load_config_file( loc_id, bigstr );

                        if ( CONFIG_SUCCESS != retval )
                            break;
                    }
                }
                else
                {
                    printf("err: node %d unknown action \"%s\"\n", node, action );
                }
            }
            else
            {
                printf("err: node %d keyword \"action\" not present\n", node );
            }

            /* go to next node */
            if ( CONFIG_SUCCESS != config_node_next_sibling (node, &node) )
            {
                break;
            }
        } while ( 0 != node );
    }

    return( retval );
}

static config_result_t memory_config( void )
{
    config_ref_t        node;
    config_result_t     result;

    result = config_node_find( ROOT_NODE, "platform.memory.layout", &node );
    if ( CONFIG_SUCCESS == result )
    {
        result = config_node_first_child( node, &node );

        if ( (CONFIG_SUCCESS == result) && node )
        {
            do
            {
                unsigned int    base, size;
                static char     name[64];

                name[63] = '\0';

                if ( (CONFIG_SUCCESS == config_node_get_name( node, name, sizeof(name) - 1 )) &&
                     (CONFIG_SUCCESS == config_get_int( node, "base", (int *) &base )) &&
                     (CONFIG_SUCCESS == config_get_int( node, "size", (int *) &size )) )
                {
                    printf( "\t%20s: base %08lx size %08lx\n", name, (long) base, (long) size );
                }
                else
                {
                    printf("ERR: entry %d requires integer entries \"base\" and \"size\"\n", node );
                    result = CONFIG_ERR_NOT_FOUND;
                    break;
                }

                /* go to next node */
                if ( CONFIG_SUCCESS != config_node_next_sibling (node, &node) )
                {
                    break;
                }
            } while ( 0 != node );
        }
        else
        {
            printf("ERR: \"platform.memory.layout\" has no entries\n" );
            result = CONFIG_ERR_NOT_FOUND;
        }
    }
    else
    {
        printf("ERR: could not find \"platform.memory.layout\"\n" );
    }

    return( result );
}

int main( int argc, const char *argv[] )
{
    int                 err = 0,print_help = 0;

    if ( argc > 1 )
    {
        config_ref_t        base_id;

        base_id = ROOT_NODE;

        if ( ! strcmp( argv[1], "dump" ) )        /* dump <location> */
        {
            if ( argc > 2 )
            {
                if ( CONFIG_SUCCESS == config_node_find( base_id, argv[2], &base_id ) )
                {
                    printf("/* DUMP of config database location \"%s\"*/\n", argv[2] );

                    config_verbose_printf( base_id, 0 );
                }
                else
                {
                    printf("ERR: could not find config database location \"%s\"\n", argv[2] );
                    err = 1;
                }
            }
            else
            {
                config_verbose_printf( base_id, 0 );
            }
        }
        else if ( ! strcmp( argv[1], "load" ) )    /* load [filename] <location> */
        {
            if ( argc > 2 )
            {
                if ( argc > 3 )
                {
                    if ( CONFIG_SUCCESS == config_node_find( base_id, argv[3], &base_id ) )
                    {
                        printf("/* LOAD \"%s\" to database location \"%s\"*/\n", argv[2], argv[3] );

                        if ( CONFIG_SUCCESS != load_config_file( base_id, argv[2] ) )
                        {
                            err = 1;
                        }
                    }
                    else
                    {
                        /* Create the location */
                        if ( (CONFIG_SUCCESS == config_set_int( ROOT_NODE, argv[3], 0 )) &&
                             (CONFIG_SUCCESS == config_node_find( ROOT_NODE, argv[3], &base_id )) )
                        {
                            printf("/* LOAD \"%s\" to (new) database location \"%s\"*/\n", argv[2], argv[3] );

                            if ( CONFIG_SUCCESS != load_config_file( base_id, argv[2] ) )
                            {
                                err = 1;
                            }
                        }
                        else
                        {
                            printf("ERR: could not find config database location \"%s\"\n", argv[3] );
                            err = 1;
                        }
                    }
                }
                else
                {
                    if ( CONFIG_SUCCESS != load_config_file( base_id, argv[2] ) )
                    {
                        err = 1;
                    }
                }
            }
            else
            {
                print_help = 1;
            }
        }
        else if ( ! strcmp( argv[1], "set_int" ) )    /* set_int <location> <value>*/
        {
           int   new_value = 0;
           err = 1;    /* default err */

           if ( argc > 3 )
           {
               if ( CONFIG_SUCCESS == config_get_int( base_id, argv[2], (int *)&new_value) )
               {
                  printf("/* SET_INT \"%s\" to database location \"%s\"*/\n", argv[3], argv[2] );
                  new_value = strtol( argv[3], NULL, 0 );
                  if ( CONFIG_SUCCESS != config_set_int( base_id, argv[2], (int) new_value) )
                  {
                     printf( "ERR: could not set \"%d\": at \"%s\" \n", new_value, argv[2]);
                  }
               }
               else
               {
                  printf( "ERR: Either could not find \"%s\" or it is not stored as an integer\n", argv[2]);
               }
            }
            else
            {
               print_help = 1;
            }
        }
        else if ( ! strcmp( argv[1], "execute" ) )    /* execute [location] */
        {
            err = 1;    /* default err */

            if ( argc > 2 )
            {
                if ( CONFIG_SUCCESS == config_node_find( base_id, argv[2], &base_id ) )
                {
                    printf("/* EXECUTE config database location \"%s\"*/\n", argv[2] );

                    if ( CONFIG_SUCCESS == execute_config_commands( base_id ) )
                    {
                        err = 0;    /* success */
                    }
                    else
                    {
                        printf("ERR: executing commands in \"%s\"\n", argv[2] );
                    }
                }
                else
                {
                    printf("ERR: could not find config database location \"%s\"\n", argv[2] );
                }
            }
            else
            {
                print_help = 1;
            }
        }
        else if ( ! strcmp( argv[1], "remove" ) )    /* remove [location] */
        {
            err = 1;    /* default err */

            if ( argc > 2 )
            {
                if ( CONFIG_SUCCESS == config_node_find( base_id, argv[2], &base_id ) )
                {
                    printf("config_node_tree_remove\"%s\"\n", argv[2] );
                    if ( CONFIG_SUCCESS == config_node_tree_remove( base_id ) )
                       err = 0;    /* success */
                }
                else
                {
                    printf("ERR: could not find config database location \"%s\"\n", argv[2] );
                }
            }
            else
            {
                print_help = 1;
            }
        }
        else if ( ! strcmp( argv[1], "memory" ) )    /* memory */
        {
            err = 1;    /* default err */

            if ( CONFIG_SUCCESS == memory_config() )
            {
               err = 0;
            }
        }
        else
        {
            print_help = 1;
        }
    }
    else
    {
        print_help = 1;
    }

    if ( print_help )
    {
        printf(
            "usage for %s:   <location> optional parameter (default root_node)\n"
            "  %s load [filename] <location>\n"
            "  %s dump <location>\n"
            "  %s set_int <location> <int value>\n"
            "  %s execute [location]\n"
            "  %s remove [location]\n"
            "  %s memshift [offset_in_MB]\n"
            "  %s memory \n",
            argv[0], argv[0], argv[0], argv[0], argv[0], argv[0], argv[0], argv[0]);
    }

    return( err );
}
