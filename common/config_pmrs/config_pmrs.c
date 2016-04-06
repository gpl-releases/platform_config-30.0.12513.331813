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

/*-----------------------------------------------------------------------
 Program Description:

 config_pmrs is a program that manages the organization of non-kernel memory 
 in systems with Intel CE processors.  The non-kernel memory consists of 
 contiguous physical memory used by the display driver, the streaming media 
 drivers and various firmwares.  This memory is specified in the "memory 
 layout" file.  However, since it is tedious to manually organize all of the 
 memory regions at specific physical addresses, this program performs the 
 task during each bootup.

 Some Intel CE processors also support protected memory regions (PMRs) which 
 are regions of physical memory that can be read only by specific devices 
 within the processor.  config_pmrs organizes memory layout entries that need 
 to be protected into the correct regions (specified in the memory layout) and 
 according to the various hardware alignment rules.

 In addition to protected memory regions, some Intel CE processors support 
 a Memory Encryption Unit (MEU).  The MEU encrypts specific physical memory 
 regions.  The current usage model is that PMRs may be placed within memory 
 regions that are encrypted by the MEU.  
 
 The use of PMRs and the MEU is part of the Trusted Data Path feature (TDP).
 The "TDP Configuration File" specifies which devices can access each PMR and 
 which PMRs will reside within MEU regions.  config_pmrs reads the 
 configuration file and organizes the entire memory layout according to its
 specifications.


 NOTES:

 1.  There are alignment restrictions in the hardware that make optimal 
 memory space utilization difficult.  config_pmrs organizes the memory layout 
 entries optimally based on the size and alignment specified for each entry.  
 Since the hardware alignment of PMRs and MEU regions does not necessarily 
 match the actual memory size requirements, there can be wasted space near 
 the boundaries of PMRs and MEU regions.  The largest gaps being just before 
 MEU regions which must be 64MB aligned.  If wasted space becomes a problem, 
 a possible workaround is that if there are memory layout entries that are 
 large but later divided into smaller buffers, then those entries can be 
 split up into multiple entries which are easier to pack into the wasted 
 space.

 For example, the following entry specifies a single layout entry that is 
 160MB in size.

 smd_buffers_linear  { type = "linear" size = 0x0A000000 }

 If there is a gap due to alignment that creates 50MB of wasted space, then 
 this layout entry cannot be used to fill the gap.  However, the region is 
 typically divided into 512kB pages by other software after the system boots, 
 so it could be easily re-organized into smaller entries which can then be 
 used by config_pmrs to fill or partially fill the gap.  For example, the 
 following entries successfully split up the single 160MB region into one 
 50MB region and one 110MB region.

 smd_buffers_linear0 { type = "linear" size = 0x03200000 }
 smd_buffers_linear1 { type = "linear" size = 0x06E00000 }

 The total is still 160MB, but now config_pmrs can use the 50MB entry to fill  
 the original 50MB gap.  It likely isn't practical to create entries that 
 always fill gaps, but if some caution is used to avoid too many very large 
 entries, the wasted space will be minimal.  config_pmrs prints out a 
 warning message when there is more than 1MB of wasted space due to 
 alignment issues.

 TODO:  config_pmrs can waste memory if a memory layout entry has a size 
 that is not a multiple of it's alignment.  This is not common practice, 
 so the extra logic to deal with it has not been implemented.  The amount 
 of memory wasted will always be the difference between the specified size 
 and that size rounded up to the next multipe of the alignment.
-----------------------------------------------------------------------*/

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stddef.h>
#include <assert.h>
#include "osal.h"

#ifdef VER
const char *config_pmrs_version_string = "#@# config_pmrs " VER;
#endif

#include "platform_config.h"
#include "platform_config_paths.h"

/* Control for debug messages. */
#define DEFAULT_LOCAL_DEBUG_LEVEL 0
static int verbosity_level = DEFAULT_LOCAL_DEBUG_LEVEL;
#define LOCAL_LOG_MSG(level, format...)    \
   do {                                    \
      if ( (level) <= verbosity_level ) {  \
         printf( format );                 \
      }                                    \
   } while ( false )

/* Miscellaneous macros */
#define ROUND_UP(n,a)           (((n)+((a)-1)) & ~((a)-1)) /* Round n up to the next multiple of a, which must be a power-of-2 */
#define ROUND_DOWN(n,a)         ((n) & ~((a)-1)) /* Round n down to the next multiple of a, which must be a power-of-2 */
#define MIN(x,y)                (((x) < (y)) ? (x) : (y))
#define MAX(x,y)                (((x) > (y)) ? (x) : (y))
#define IS_PWR_OF_2(n)          (((n) != 0) && (((n) & ((n)-1)) == 0)) /* Determine if a number is a power-of-2 */
#define LARGEST_PWR_OF_2(m)     ((m) & -(m)) /* Find the largest power-of-2 factor of a number */

#define CONFIG_PATH_PLATFORM_MEDIA_BASE_ADDR "platform.memory.media_base_address"
#define CONFIG_PATH_PLATFORM_ENABLE_PMR      "platform.memory.enable_pmr"
#define CONFIG_PATH_PLATFORM_TDP_CONFIG_FILE "platform.memory.tdp_config_file"
#define CONFIG_PATH_PLATFORM_PMR_INFO        "platform.memory.pmr_info"
#define CONFIG_PATH_PLATFORM_MEU_INFO        "platform.memory.meu_info"

/* Size constraints */
#define MAX_MEM_LAYOUT_ENTRIES  256 /* Max number of memory layout entries supported. */
#define MAX_LAYOUT_NAME_SIZE     64 /* Max length (in characters) of a memory layout entry's name */
#define NUM_PMRS                 16 /* Number of PMRs supported by current hardware. */
#define DEFAULT_ALIGNMENT        64 /* Memory layout regions will default to an alignment of 64 bytes.  This size was chosen for x86 cacheline alignment */
#define PMR_ALIGNMENT      0x100000 /* Hardware requirement for PMR alignment. */
#define NUM_MEU_REGIONS           2 /* Number of MEU regions supported by current hardware. */
#define MAX_FILE_NAME_LENGTH    256 /* Max length (in characters) of a filename including path. */
#define GAP_WARN_THRESHOLD 0x100000 /* Print a warning if more than this much space is wasted due to alignment restrictions. */


/* Mapping of PMR type to a name based on its intended use. */
typedef enum {
   PMR_INVALID   = -1, /* Invalid = not protected. */
   UNUSED        =  0,
   SEC           =  1,
   RESERVED      =  2,
   TSD_FW_DATA   =  3,
   VIDEO_FW_CODE =  4,
   VIDEO_FW_DATA =  5,
   AV_STREAM     =  6,
   CMP_VID       =  7,
   AUDIO         =  8,
   PIXELS        =  9,
   VDC_WB        = 10,
   AUDIO_FW_CODE = 11,
   AUDIO_FW_DATA = 12,
   RESERVED13    = 13,
   RESERVED14    = 14,
   RESERVED15    = 15,
} pmr_type_t;


/* Default PMR type if no type is specified in a memory layout entry. */
#define DEFAULT_PMR_TYPE  PMR_INVALID


/* Available MEU Regions. This is the platform_config format. */
typedef enum {
   MEU_DISABLED = -1,
   MEU_REGION_0 =  0,
   MEU_REGION_1 =  1,
} pmr_meu_region_t;


/* Completely describe a memory layout entry. */
typedef struct mem_layout_struct {
   char                       name[MAX_LAYOUT_NAME_SIZE+1];
   unsigned int               base_pa;       /* Physical base address of this layout entry */
   unsigned int               ram_base;      /* Actual address in RAM, different from base_pa if mapped into MEU. */
   unsigned int               size;          /* Size of this layout entry in bytes. */
   int                        pmr_type;      /* PMR type assigned to this layout entry. */
   unsigned int               alignment;     /* Byte alignment required for buffers in this entry. */
   unsigned int               adjusted_size; /* Size after accounting for alignment restrictions. */
   struct mem_layout_struct * next;          /* Pointer to next layout entry in the list for a PMR. */
} mem_layout_t;


/* Memory layout list for associating memory layout entries with PMRs. */
typedef struct {
    mem_layout_t *head;
} layout_list_t;


/* Completely describe a Protected Memory Region */
typedef struct pmr_struct {
   unsigned int        base_pa;      /* Physical base address of PMR */
   unsigned int        size;         /* PMR size in bytes */
   pmr_type_t          type;         /* Region type as described in SEC spec. */
   char                name[MAX_LAYOUT_NAME_SIZE+1];  /* Name used in memory layout configuration */
   pmr_meu_region_t    meu_region;   /* MEU region assigned to this PMR (-1 = none, 0 = region 0, 1 = region 1) */
   unsigned int        meu_base;     /* Physical base address of PMR in RAM (same as base_pa when meu_region = -1). */
   unsigned int        sta;          /* Security attributes for this PMR region, deprecated in this app. */
   struct pmr_struct * next;         /* Pointer to next PMR in the PMR list for an MEU region. */
   mem_layout_t      * layouts;      /* List of layout entries residing within this PMR. */
} pmr_config_t;

/* Define an MEU region. */
typedef struct {
   int           region_id;   /* Region ID (currently only 0 and 1 are valid in HW). */
   unsigned int  region_base; /* Base address defined by hardware */
   unsigned int  phys_base;   /* Base address in RAM */
   unsigned int  size;        /* Size of the region, in bytes. */
   unsigned int  alignment;   /* Alignment required for this region (HW requires 64MB). */
   unsigned int  max_size;    /* The maximum size of this region (HW defined) in bytes. */
   pmr_config_t *pmrs;        /* List of PMRs residing within this region. */
} meu_config_t;


typedef enum {
   DISABLE_PMR        = 0,
   ENABLE_PMR_FULL    = 1,
   ENABLE_PMR_PARTIAL = 2  /* No longer supported. */
} pmr_mode_t;

#define DEFAULT_PMR_MODE   DISABLE_PMR
#define DEFAULT_ENABLE_PMR false


/* Default security attributes for a PMR. */
#define   STA_DEFAULT          0x00

/* 
 * The following security attributes are the default values for a PMR if 
 * there is no tdp_config_file.  This was used by the SEC driver for the 
 * former "Partial TDP" mode, but should no longer be used.
 */
#define   STA_UNUSED           0x00
#define   STA_SEC              0x80
#define   STA_RESERVED         0x00
#define   STA_TSD_FW_DATA      0xA0
#define   STA_VIDEO_FW_CODE    0x88
#define   STA_VIDEO_FW_DATA    0x88
#define   STA_AV_STREAM        0xA0
#define   STA_CMP_VID          0xA8
#define   STA_AUDIO            0x00
#define   STA_PIXELS           0x00
#define   STA_VDC_WB           0x00
#define   STA_AUDIO_FW_CODE    0x00
#define   STA_AUDIO_FW_DATA    0x00

/*
 * PMR Configuration Table.  This supports "full" trusted data path as opposed 
 * to the previous "partial" TDP solution.  There is one entry for each 
 * protected memory region and one additional entry for non-PMR buffers.  Each 
 * entry contains a full description of the region plus a list of memory layout 
 * entries that fall within the region.  The non-PMR (PMR_INVALID) entry is 
 * used only for performing alignment and packing of the non-PMR memory.  It's 
 * contents are not written back to platform_config.
 */
static pmr_config_t pmr_config[NUM_PMRS+1] = {
// { base, size, type         , name           , meu_region,   meu_base, sta              , next, layouts },
   {    0,    0, UNUSED       , "UNUSED"       , MEU_DISABLED,        0, STA_UNUSED       , NULL, NULL    },
   {    0,    0, SEC          , "SEC"          , MEU_DISABLED,        0, STA_SEC          , NULL, NULL    },
   {    0,    0, RESERVED     , "RESERVED"     , MEU_DISABLED,        0, STA_RESERVED     , NULL, NULL    },
   {    0,    0, TSD_FW_DATA  , "TSD_FW_DATA"  , MEU_DISABLED,        0, STA_TSD_FW_DATA  , NULL, NULL    },
   {    0,    0, VIDEO_FW_CODE, "VIDEO_FW_CODE", MEU_DISABLED,        0, STA_VIDEO_FW_CODE, NULL, NULL    },
   {    0,    0, VIDEO_FW_DATA, "VIDEO_FW_DATA", MEU_DISABLED,        0, STA_VIDEO_FW_DATA, NULL, NULL    },
   {    0,    0, AV_STREAM    , "AV_STREAM"    , MEU_DISABLED,        0, STA_AV_STREAM    , NULL, NULL    },
   {    0,    0, CMP_VID      , "CMP_VID"      , MEU_DISABLED,        0, STA_CMP_VID      , NULL, NULL    },
   {    0,    0, AUDIO        , "AUDIO"        , MEU_DISABLED,        0, STA_AUDIO        , NULL, NULL    },
   {    0,    0, PIXELS       , "PIXELS"       , MEU_DISABLED,        0, STA_PIXELS       , NULL, NULL    },
   {    0,    0, VDC_WB       , "VDC_WB"       , MEU_DISABLED,        0, STA_VDC_WB       , NULL, NULL    },
   {    0,    0, AUDIO_FW_CODE, "AUDIO_FW_CODE", MEU_DISABLED,        0, STA_AUDIO_FW_CODE, NULL, NULL    },
   {    0,    0, AUDIO_FW_DATA, "AUDIO_FW_DATA", MEU_DISABLED,        0, STA_AUDIO_FW_DATA, NULL, NULL    },
   {    0,    0, RESERVED13   , "RESERVED13"   , MEU_DISABLED,        0, STA_DEFAULT      , NULL, NULL    },
   {    0,    0, RESERVED14   , "RESERVED14"   , MEU_DISABLED,        0, STA_DEFAULT      , NULL, NULL    },
   {    0,    0, RESERVED15   , "RESERVED15"   , MEU_DISABLED,        0, STA_DEFAULT      , NULL, NULL    },
   {    0,    0, PMR_INVALID  , "NONE"         , MEU_DISABLED,        0, STA_DEFAULT      , NULL, NULL    }
};


/*
 *  MEU Configuration table.  This table defines the MEU configuration for each
 *  MEU region.  There is one entry to describe each MEU region and one 
 *  additional entry to describe the non-MEU region(s).  The non-MEU entry is 
 *  used as a placeholder for all PMRs that do not reside within an MEU region. 
 *  However, unlike the other entries, the non-MEU entry is not necessarily a 
 *  single contiguous memory region.
 */
static meu_config_t meu_config[NUM_MEU_REGIONS+1] = {
// { region_id,    region_base, phys_base , size      , alignment , max_size  , pmrs }, 
   { MEU_REGION_0, 0xD0000000 , 0x00000000, 0x00000000, 0x04000000, 0x04000000, NULL }, 
   { MEU_REGION_1, 0xD4000000 , 0x00000000, 0x00000000, 0x04000000, 0x04000000, NULL }, 
   { MEU_DISABLED, 0x00000000 , 0x00000000, 0x00000000, 0x00100000, 0xC0000000, NULL }
};


/* Global that specifies whether PMRs are enabled. */
static bool g_enable_pmr = DEFAULT_ENABLE_PMR;

/* Global that specifies whether the MEU is enabled. */
static bool g_enable_meu = false;

/* Global that holds platform.memory.media_base_address */
static unsigned int g_media_base_address;

/* Global array to hold data read from platform_config platform.memory.layout */
static mem_layout_t mem_layout_table[MAX_MEM_LAYOUT_ENTRIES];

/* Number of memory layout entries read from platform_config. */
static int mem_layout_count = 0;

/*
 * Determine where the MEU region base is actually located by examining the 
 * PCI config space registers of the SOC's AV Bridge device.  Assign the 
 * correct region base to each MEU region.  Note that this function may be  
 * overkill as we believe the custom PCI enumeration in the Linux kernel 
 * just assigns a hard-coded base address of 0xC0000000 to the AV Bridge.
 */
#define AV_BRIDGE_BUS         0
#define AV_BRIDGE_DEVICE      1
#define AV_BRIDGE_FUNCTION    0
#define AV_BRIDGE_BAR_OFFSET  0x20

bool get_meu_region_base( void )
{
   bool         result       = false;
   osal_result  osal_res     = OSAL_SUCCESS;
   uint16_t     base_address = 0;
   uint32_t     meu_base     = 0;
   int          i            = 0;
   os_pci_dev_t pci_dev;

   osal_res = os_pci_device_from_address( &pci_dev, 
                                          AV_BRIDGE_BUS, 
                                          AV_BRIDGE_DEVICE, 
                                          AV_BRIDGE_FUNCTION );
   if ( osal_res != OSAL_SUCCESS ) {
      LOCAL_LOG_MSG( 0, 
                     "Unable to find AV Bridge PCI device, error %d.\n", 
                     osal_res );
   }
   else {
      osal_res = os_pci_read_config_16( pci_dev, 
                                        AV_BRIDGE_BAR_OFFSET, 
                                        &base_address );
      if ( osal_res != OSAL_SUCCESS ) {
         LOCAL_LOG_MSG( 0, 
                        "Unable to read AV Bridge device BAR, error %d.\n", 
                        osal_res );
      }
      else {
         LOCAL_LOG_MSG( 4, "AV Bridge device BAR = 0x%04X.\n", base_address );
         
         /* The MEU regions start 256MB from the AV bridge base. */
         meu_base = (base_address << 16) + 0x10000000;

         /* Set the region base address of each MEU region. */
         /* Note: assumes the MEU_DISABLED region is last in the list. */
         for ( i = 0; i < NUM_MEU_REGIONS; i++ ) {
            meu_config[i].region_base = meu_base ;
            LOCAL_LOG_MSG( 4, 
                           "MEU Region %d base = 0x%08X.\n", 
                           i, 
                           meu_config[i].region_base );
            meu_base += meu_config[i].max_size;
         }
         result = true;
      }

      os_pci_free_device( pci_dev );
   }

   return result;
}


/*
 *  Return a pointer to the meu config table entry for a given region ID.  
 *  If the region ID is not found, return NULL.
 */
static meu_config_t *
lookup_meu_by_region_id( int region_id )
{
   int           index = 0;
   meu_config_t *meu   = NULL;

   for ( index = 0; index < NUM_MEU_REGIONS+1; index++ ) {
      if ( meu_config[index].region_id == region_id ) {
         meu = &meu_config[index];
         break;
      }
   }

   return meu;
}


/*
 *  Return a pointer to the pmr table entry for a given PMR type.  If the type 
 *  is not found, return NULL.
 */
static pmr_config_t *
lookup_pmr_by_type( pmr_type_t type )
{
   int           index = 0;
   pmr_config_t *pmr   = NULL;

   for ( index = 0; index < NUM_PMRS+1; index++ ) {
      if ( pmr_config[index].type == type ) {
         pmr = &pmr_config[index];
         break;
      }
   }

   return pmr;
}


/* Read the media base address from platform_config. */
static config_result_t 
get_media_base_address( void )
{
   config_result_t ret = CONFIG_SUCCESS;

   ret = config_get_int( ROOT_NODE,
                         CONFIG_PATH_PLATFORM_MEDIA_BASE_ADDR,
                         (int *)&g_media_base_address );
   if ( CONFIG_SUCCESS != ret ) {
      LOCAL_LOG_MSG( 0, 
                     "Failed to retreive media base address at %s.\n", 
                     CONFIG_PATH_PLATFORM_MEDIA_BASE_ADDR );
   }
   else {
      LOCAL_LOG_MSG( 1, 
                     "%s = 0x%08X\n", 
                     CONFIG_PATH_PLATFORM_MEDIA_BASE_ADDR, 
                     g_media_base_address );
   }

   return ret;
}


/*
 *  Get the value of the "enable_pmr" setting in platform_config, or the
 *  default if it doesn't exist and determine whether PMRs should be 
 *  enabled or not.
 */
static int
determine_pmr_mode( void )
{
   config_result_t config_ret;
   int enable_pmr = DEFAULT_PMR_MODE;

   /* Read the enable_pmr setting. */
   config_ret = config_get_int( ROOT_NODE, 
                                CONFIG_PATH_PLATFORM_ENABLE_PMR, 
                                &enable_pmr );
   if ( CONFIG_SUCCESS != config_ret ) {
      LOCAL_LOG_MSG( 1, 
                     "Failed to find the enable_pmr setting at %s. "
                     "Using default of %d\n", 
                     CONFIG_PATH_PLATFORM_ENABLE_PMR, 
                     DEFAULT_PMR_MODE );
      enable_pmr = DEFAULT_PMR_MODE;
   }

   LOCAL_LOG_MSG( 2, 
                  "enable_pmr setting %s = %d.\n", 
                  CONFIG_PATH_PLATFORM_ENABLE_PMR, 
                  enable_pmr );

   switch ( enable_pmr ) {
      case DISABLE_PMR:
         g_enable_pmr = false;
         break;

      case ENABLE_PMR_FULL:
         g_enable_pmr = true;
         break;

      /* If config file specifies deprecated setting, use the newer one. */
      case ENABLE_PMR_PARTIAL:
         LOCAL_LOG_MSG( 0, 
                        "WARNING: enable_pmr setting %d no longer supported."
                        " Using %d instead.\n", 
                        enable_pmr, 
                        ENABLE_PMR_FULL );
         g_enable_pmr = true;
         break;

      /* If config file specifies invalid setting, use the default setting. */
      default:
         LOCAL_LOG_MSG( 0, 
                        "WARNING: Unknown enable_pmr setting %d. "
                        "Using default of %d.\n", 
                        enable_pmr, 
                        DEFAULT_PMR_MODE );
         g_enable_pmr = DEFAULT_ENABLE_PMR;
         break;
   }

   return enable_pmr;
}


/*
 * Add a PMR node to the list of PMRs associted with an MEU region.  
 * Keep the list sorted by PMR ID from highest to lowest.
 */
static void
add_pmr_to_meu_region( meu_config_t *meu, 
                       pmr_config_t *pmr )
{
   pmr_config_t **curr_ptr = &(meu->pmrs);

   while ( *curr_ptr != NULL &&
           pmr->type <= (*curr_ptr)->type ) {
      curr_ptr = &((*curr_ptr)->next);
   }

   pmr->next = *curr_ptr;
   *curr_ptr = pmr;
}


/*
 * Remove a PMR node from the list of PMRs associated with an MEU region.
 */
static void
remove_pmr_from_meu_region( meu_config_t *meu, 
                            pmr_config_t *pmr )
{
   pmr_config_t **curr_ptr = &(meu->pmrs);

   /* Find the node to remove in the list. */
   while ( (*curr_ptr != NULL) &&
           (*curr_ptr != pmr) ) {
      curr_ptr = &((*curr_ptr)->next);
   }

   /* Unlink the node from the list. */
   if ( *curr_ptr != NULL ) {
      *curr_ptr = pmr->next;
   }
   pmr->next = NULL;
}


/*****************************************************************************
 * Code to convert TDP configuration file to what is needed for 
 * platform_config. 
 * TODO: Split this out into another file?
 *****************************************************************************/

/* TDP config file has a fixed size portion and a variable size portion.
 * TDP_CONFIG_FILE_SIZE corresponds to the fixed size portion, which consists
 * of a key (of size TDP_CONFIG_FILE_KEY_SIZE), the TDP config struct, and the
 * signature.
 * The variable size portion consists of "key_count" hashes of keys, 
 * each of size TDP_CONFIG_FILE_KEY_HASH_SIZE
 */
#define TDP_CONFIG_FILE_SIZE                1028
#define TDP_CONFIG_FILE_KEY_SIZE            644
#define TDP_CONFIG_FILE_SIGNATURE_SIZE      256
#define TDP_CONFIG_STRUCT_OFFSET            TDP_CONFIG_FILE_KEY_SIZE
#define TDP_CONFIG_FILE_MODULE_TYPE_VALUE   0x06000000
#define TDP_CONFIG_FILE_MODULE_TYPE_OFFSET  (TDP_CONFIG_STRUCT_OFFSET + 0)
#define TDP_CONFIG_FILE_MODULE_ID_LO_VALUE_ONE  0x0140
#define TDP_CONFIG_FILE_MODULE_ID_LO_VALUE_TWO  0x0240
#define TDP_CONFIG_FILE_MODULE_ID_LO_OFFSET (TDP_CONFIG_STRUCT_OFFSET + 14)
#define TDP_CONFIG_FILE_KEY_HASH_SIZE       0x24
#define TDP_CONFIG_FILE_KEY_COUNT_OFFSET    (TDP_CONFIG_STRUCT_OFFSET + offsetof(tdp_configuration_t, key_count))

#define ENDIAN_SWITCH_DWORD(_n)                                \
   ((((_n) & 0x000000ff) << 24) | (((_n) & 0x0000ff00) << 8) | \
     (((_n) & 0x00ff0000) >> 8) | (((_n) & 0xff000000) >> 24))

/* 
 *  MEU Regions defined in the TDP config file. 
 *  Requires translation to the platform_config format.
 */
typedef enum {
   PMR_MEU_DISABLED = 0,
   PMR_MEU_REGION_0 = 1,
   PMR_MEU_REGION_1 = 2,

   /* Make sure compiler does not choose unsigned type for this enum. */
   MAKE_PMR_MEU_CONFIG_T_SIGNED = -1 
} pmr_meu_config_t;


/* Determine if an MEU_CONFIG value read from the TDP config file is valid. */
#define VALID_MEU_CONFIG(mc) ( ((pmr_meu_config_t)(mc) >= PMR_MEU_DISABLED) && \
                               ((pmr_meu_config_t)(mc) <= PMR_MEU_REGION_1) )

/* 
 * Translate from the MEU_CONFIG values in the TDP config file to the values 
 * we will be writing to platform_config.
 */
#define TRANSLATE_MEU_CONFIG(mc) ( (pmr_meu_region_t)(VALID_MEU_CONFIG(mc) ? \
                                   ((mc) - 1) :                              \
                                   MEU_DISABLED) )

/* 
 * This structure contains the entire TDP configuration.  The structure matches 
 * the TDP Configuration File format defined in the Security Controller FW 
 * Design Document.
 * 
 * This application uses only the pmr_attributes and pmr_meu_config fields.  
 * The pmr_attributes field contains the security attributes (STA) for each 
 * PMR.  The pmr_meu_config field specifies a mapping between the PMR regions 
 * and the MEU regions (0 = none, 1 = MEU Region0, 2 = MEU Region1).
 */
typedef struct {
   uint32_t module_type;
   uint32_t header_length;
   uint32_t header_version;

   uint32_t module_id;
   uint32_t module_vendor;
   uint32_t date;
   uint32_t module_size;
   uint32_t key_size;
   uint32_t modulus_size;
   uint32_t exponent_size;

   uint32_t vendor_id;
   uint32_t config_file_version;
   uint8_t  pmr_attributes[NUM_PMRS]; /* Security attributes (STA) */
   uint8_t  pmr_meu_config[NUM_PMRS]; /* PMR to MEU mapping */
   uint8_t  tsd_config;
   uint8_t  tsd_attribute;
   uint8_t  mfd_gvd_config; 
   uint8_t  mfd_gvd_attribute; 
   uint8_t  gvs_gvt_config;
   uint8_t  gvs_gvt_attribute;
   uint8_t  h264ve_config;
   uint8_t  h264ve_attribute;
   uint8_t  dpe_config;
   uint8_t  dpe_attribute;
   uint8_t  vdc_attribute;
   uint8_t  hdvcap_attribute;
   uint8_t  audio_config;
   uint8_t  audio_attribute;
   uint8_t  reserved0[2]; /* "Reserved (2 bytes)" in the spec. */
   uint32_t tdp_fw_version;
   uint32_t tsd_fw_version;
   uint32_t mfd_gvd_fw_version;
   uint32_t gvs_gvt_fw_version;
   uint32_t dpe_fw_version;
   uint32_t audio_dsp0_fw_version;
   uint32_t audio_dsp1_fw_version;

   uint32_t key_count;
} tdp_configuration_t;


static tdp_configuration_t tdp_config;


/*
 * Validate the TDP configuration file.  These are only cursory checks to make 
 * sure the file format looks correct.  Real validation is done by the SEC.
 */
bool 
valid_tdp_config_file( FILE *tdp_config_file, 
                       char *filename )
{
   bool           result      = false;
   unsigned int   module_type = 0;
   unsigned short module_id   = 0;
   int            position    = 0;
   unsigned int   key_count   = 0;

   if ( fseek(tdp_config_file, 
                   TDP_CONFIG_FILE_KEY_COUNT_OFFSET, 
                   SEEK_SET) != 0 ) {
      LOCAL_LOG_MSG( 0, 
                     "ERROR: Cannot find key count in TDP configuration "
                     "file '%s'.\n", 
                     filename );
   }
   else if ( fread( &key_count, 
                    1, 
                    sizeof(unsigned int), 
                    tdp_config_file ) != sizeof(unsigned int) ) {
      LOCAL_LOG_MSG( 0, 
                     "ERROR: Unable to read key_count in TDP configuration "
                     "file '%s'.\n", 
                     filename );
   }
   else if ( fseek(tdp_config_file, 0, SEEK_END) != 0 ) {
      LOCAL_LOG_MSG( 0, 
                     "ERROR: Cannot find EOF in TDP configuration file "
                     "'%s'.\n", 
                     filename );
   }
   else if ( (position = ftell(tdp_config_file)) != 
         (TDP_CONFIG_FILE_SIZE + ENDIAN_SWITCH_DWORD(key_count) * TDP_CONFIG_FILE_KEY_HASH_SIZE)) {
      LOCAL_LOG_MSG( 0, 
                     "ERROR: TDP configuration file '%s' has invalid size "
                     "%d.\n", 
                     filename, 
                     position );
   }
   else if ( fseek(tdp_config_file, 
                   TDP_CONFIG_FILE_MODULE_TYPE_OFFSET, 
                   SEEK_SET) != 0 ) {
      LOCAL_LOG_MSG( 0, 
                     "ERROR: Cannot find module type in TDP configuration "
                     "file '%s'.\n", 
                     filename );
   }
   else if ( fread( &module_type, 
                    1, 
                    sizeof(unsigned int), 
                    tdp_config_file ) != sizeof(unsigned int) ) {
      LOCAL_LOG_MSG( 0, 
                     "ERROR: Unable to read module type in TDP configuration "
                     "file '%s'.\n", 
                     filename );
   }
   else if ( module_type != TDP_CONFIG_FILE_MODULE_TYPE_VALUE ) {
      LOCAL_LOG_MSG( 0, 
                     "ERROR: Invalid module type 0x%08X in TDP configuration "
                     "file '%s'.\n", 
                     module_type, 
                     filename );
   }
   else if ( fseek(tdp_config_file, 
                   TDP_CONFIG_FILE_MODULE_ID_LO_OFFSET, 
                   SEEK_SET) != 0 ) {
      LOCAL_LOG_MSG( 0, 
                     "ERROR: Cannot find module ID in TDP configuration file "
                     "'%s'.\n", 
                     filename );
   }
   else if ( fread( &module_id, 
                    1, 
                    sizeof(unsigned short), 
                    tdp_config_file ) != sizeof(unsigned short) ) {
      LOCAL_LOG_MSG( 0, 
                     "ERROR: Unable to read module ID in TDP configuration "
                     "file '%s'.\n", 
                     filename );
   }
   else if ( (module_id != TDP_CONFIG_FILE_MODULE_ID_LO_VALUE_ONE) && 
         (module_id != TDP_CONFIG_FILE_MODULE_ID_LO_VALUE_TWO)) {
      LOCAL_LOG_MSG( 0, 
                     "ERROR: Invalid module ID 0x%04X in TDP configuration "
                     "file '%s'.\n", 
                     module_id, 
                     filename );
   }
   else {
      result = true;
   }

   return result;
}


/* 
 * Read the TDP config from the file into a data structure in memory and 
 * determine if the MEUs should be enabled.  The MEUs will be enabled if the 
 * following conditions are met:
 *   1.  The tdp_config_file entry exists in the platform_config database
 *   2.  The TDP configuration file exists and is valid.
 * However, the MEU will be enabled on the platform only if the TDP 
 * configuration file specifies that at least one PMR is mapped within an 
 * MEU region.
 *
 * Also note that if the MEUs are enabled, then PMRs will automatically be 
 * enabled, even if the enable_pmr setting is set to disable PMRs.
 */
bool 
read_tdp_configuration( void )
{
   FILE *tdp_config_file      = NULL;
   bool            result     = false;
   int             fret       = 0;
   config_result_t config_ret = CONFIG_SUCCESS;
   char filename[MAX_FILE_NAME_LENGTH+1];
   
   /* Get the name of the meu config file. */
   filename[MAX_FILE_NAME_LENGTH] = '\0';
   config_ret = config_get_str( ROOT_NODE, 
                                CONFIG_PATH_PLATFORM_TDP_CONFIG_FILE, 
                                filename, 
                                MAX_FILE_NAME_LENGTH );
   if ( CONFIG_SUCCESS != config_ret ) {
      LOCAL_LOG_MSG( 1, 
                     "WARNING: TDP configuration file not specified at '%s'. "
                     "MEU will not be enabled.\n", 
                     CONFIG_PATH_PLATFORM_TDP_CONFIG_FILE );
   }
   else {
      LOCAL_LOG_MSG( 2, 
                     "Reading TDP configuration from file '%s'.\n", 
                     filename );

      tdp_config_file = fopen( filename, "rb" );
      if ( tdp_config_file == NULL ) {
         LOCAL_LOG_MSG( 0, 
                        "ERROR: Cannot open TDP configuration file: %s. "
                        "MEU will not be enabled.\n", 
                        filename );
      }
      else if ( !valid_tdp_config_file(tdp_config_file, filename) ) {
         LOCAL_LOG_MSG( 0, 
                        "ERROR: Invalid TDP configuration file '%s'. "
                        "MEU will not be enabled.\n", 
                        filename );
         fclose( tdp_config_file );
      }
      else {
         assert( (TDP_CONFIG_FILE_KEY_SIZE + 
                  sizeof(tdp_configuration_t) + 
                  TDP_CONFIG_FILE_SIGNATURE_SIZE) == TDP_CONFIG_FILE_SIZE );

         fret = fseek( tdp_config_file, TDP_CONFIG_STRUCT_OFFSET, SEEK_SET );
         assert( fret == 0 );

         fret = ftell( tdp_config_file );
         assert( fret == TDP_CONFIG_STRUCT_OFFSET );

         fret = fread( &tdp_config, 
                       1, 
                       sizeof(tdp_configuration_t), 
                       tdp_config_file );
         assert( fret == sizeof(tdp_configuration_t) );

         fret = fclose( tdp_config_file );
         assert( fret == 0 );

         g_enable_pmr = true;
         g_enable_meu = true;

         result = true;
      }
   }

   return result;
}


/*
 * Walk through the TDP configuration file structure and assign an MEU 
 * configuration to each PMR entry in the global pmr configuration table.
 */
static void 
process_meu_config( void )
{
   pmr_meu_config_t pmr_meu_config = PMR_MEU_DISABLED;
   pmr_config_t    *pmr            = NULL;
   meu_config_t    *meu            = NULL;
   int              i              = 0;

   LOCAL_LOG_MSG( 2, "Processing MEU configuration in TDP Config file.\n" );

   /* For each PMR id in the TDP configuration file. */
   for ( i = 0; i < NUM_PMRS; i++ ) {

      /* Get the meu configuration for the PMR type. */
      pmr_meu_config = PMR_MEU_DISABLED;
      if ( g_enable_meu ) {
         pmr_meu_config = (pmr_meu_config_t)tdp_config.pmr_meu_config[i];
      }

      if ( VALID_MEU_CONFIG(pmr_meu_config) ) {
         pmr = lookup_pmr_by_type( i );
         if ( pmr != NULL ) {
            pmr->sta = (unsigned int)(tdp_config.pmr_attributes[i]);
            pmr->meu_region = TRANSLATE_MEU_CONFIG( pmr_meu_config );
            meu = lookup_meu_by_region_id( pmr->meu_region );
            assert( NULL != meu );
            LOCAL_LOG_MSG( 2, 
                           "Adding PMR %3d to MEU region %2d.\n", 
                           i, 
                           pmr->meu_region );
            add_pmr_to_meu_region( meu, pmr );
         }
         else {
            LOCAL_LOG_MSG( 2, 
                           "WARNING: Ignoring undefined PMR type %d.\n", 
                           i );
         }
      }
      else {
         /* The MEU Config value read from the TDP Config file was not valid. */
         LOCAL_LOG_MSG( 0, 
                        "ERROR: Invalid MEU config %d for PMR ID %d. "
                        "Defaulting to MEU disabled.\n", 
                        pmr_meu_config, 
                        i );
      }

   }

   /*
    * Put the non-PMR memory into the pseudo MEU region with region_id of 
    * MEU_DISABLED.
    */
   pmr = lookup_pmr_by_type( PMR_INVALID );
   assert( pmr != NULL );
   meu = lookup_meu_by_region_id( MEU_DISABLED );
   assert( meu != NULL );
   LOCAL_LOG_MSG( 2, 
                  "Adding PMR %3d to MEU region %2d.\n", 
                  PMR_INVALID, 
                  pmr->meu_region );
   add_pmr_to_meu_region( meu, pmr );

   return;
}

/****************************************************************************
 * End of TDP Configuration File code.
 ****************************************************************************/


/*
 * Update the base physical address of a memory layout entry in the config 
 * database.  Also update the PMR for the entry in case a PMR was specified 
 * but PMRs are disabled.
 */
static config_result_t
update_layout_entry( mem_layout_t *region )
{
   config_ref_t    layout_node;
   config_result_t config_ret;

   config_ret = config_node_find( ROOT_NODE, 
                                  CONFIG_PATH_PLATFORM_MEMORY_LAYOUT, 
                                  &layout_node );
   if ( config_ret == CONFIG_SUCCESS ) {
      config_ret = config_node_find( layout_node, 
                                     region->name, 
                                     &layout_node );
      if ( config_ret == CONFIG_SUCCESS ) {
         config_ret = config_set_int( layout_node, 
                                      "base", 
                                      (int)region->base_pa );
         if ( config_ret == CONFIG_SUCCESS ) {
            config_ret = config_set_int( layout_node, 
                                         "pmr", 
                                         (int)region->pmr_type );
         }
      }
   }

   return config_ret;
}


/*
 * Ensure that the alignment boundry for a given memory layout
 * entry is valid.  Adjust the layout boundary if the size is
 * not a multiple of the alignment.
 */
static void
check_alignment_boundry( mem_layout_t *layout )
{
   /* If the alignment was specified... */
   if ( layout->alignment != 0 ) {
      /* If the specified alignment is not a power of 2, then it's invalid. */
      if ( !IS_PWR_OF_2(layout->alignment) ) {
         LOCAL_LOG_MSG( 0, 
                        "WARNING: Buffer alignment of 0x%X is not supported. "
                        "Alignment must be a power of 2.", 
                        layout->alignment );
         layout->alignment = 0;
      }
   }

   /* If the alignment was not specified or was invalid... */
   if ( layout->alignment == 0 ) {
      /*
       * Make the alignment the largest power of 2 that satisfies:
       * DEFAULT_ALIGNMENT <= alignment <= PMR_ALIGNMENT 
       */
      layout->alignment = MIN( PMR_ALIGNMENT, LARGEST_PWR_OF_2(layout->size) );
      layout->alignment = MAX( DEFAULT_ALIGNMENT, layout->alignment );
   }

   /* Calculate the next boundary that is a multiple of the alignment. */
   layout->adjusted_size = ROUND_UP( layout->size, layout->alignment );
}


/*
 *  Add a layout node to a PMR's layout list.  Keep the list sorted by 
 *  alignment from highest to lowest.
 */
static void
add_layout_to_pmr( pmr_config_t *pmr, 
                   mem_layout_t *layout )
{
   mem_layout_t **curr_ptr = &(pmr->layouts);

   while ( *curr_ptr != NULL &&
           layout->alignment <= (*curr_ptr)->alignment ) {
      curr_ptr = &((*curr_ptr)->next);
   }

   layout->next = *curr_ptr;
   *curr_ptr = layout;
}


/* 
 * Remove a layout entry from a PMR's layout list.
 */
static void
remove_layout_from_pmr( pmr_config_t *pmr, 
                        mem_layout_t *layout )
{
   mem_layout_t **curr_ptr = &(pmr->layouts);

   /* Find the node to remove in the list. */
   while ( *curr_ptr != NULL &&
           *curr_ptr != layout ) {
      curr_ptr = &((*curr_ptr)->next);
   }

   /* Unlink the node from the list. */
   if ( *curr_ptr != NULL ) {
      *curr_ptr = layout->next;
   }
   layout->next = NULL;
}


/*
 *  Transfer a memory layout read from the platform_config tree into the 
 *  appropriate PMR structure.
 */
static void
add_layout_entry_to_pmr( mem_layout_t *layout_entry )
{
   pmr_config_t *pmr = NULL;

   pmr = lookup_pmr_by_type( layout_entry->pmr_type );

   assert( NULL != pmr );

   LOCAL_LOG_MSG( 4,
                  "Adding layout entry '%36s' to pmr %3d (%14s)\n",
                  layout_entry->name,
                  layout_entry->pmr_type,
                  pmr->name );

   add_layout_to_pmr( pmr, layout_entry );
}


/*
 * Walk through the config tree analyzing the memory layout entries and 
 * placing the results in the mem_layout_table.  Then add the memory layout 
 * entries to their respective PMRs.
 */
static void
process_memory_layout( void )
{
   config_result_t config_ret     = CONFIG_SUCCESS;
   config_ref_t    layout_node    = ROOT_NODE;
   config_ref_t    child_node     = ROOT_NODE;
   int             i              = 0;

   memset( &mem_layout_table, 0, sizeof( mem_layout_table ) );

   /* Find the first memory layout node. */
   config_ret = config_node_find( ROOT_NODE, 
                                  CONFIG_PATH_PLATFORM_MEMORY_LAYOUT, 
                                  &layout_node );

   if ( config_ret == CONFIG_SUCCESS )  {
      config_ret = config_node_first_child( layout_node, &child_node );

      /* Iterate through all of the layout entries. */
      while ( (config_ret == CONFIG_SUCCESS) && 
              (i < MAX_MEM_LAYOUT_ENTRIES) ) {
         /* Get the values of the name, size, pmr, and align fields. */
         config_ret = config_node_get_name( child_node, 
                                            mem_layout_table[i].name, 
                                            MAX_LAYOUT_NAME_SIZE );
         /* Guarantee the name string is null-terminated. */
         mem_layout_table[i].name[MAX_LAYOUT_NAME_SIZE] = '\0';

         config_ret = config_get_int( child_node, 
                                      "size",  
                                      (int *)&mem_layout_table[i].size      );
         if ( (config_ret != CONFIG_SUCCESS) || 
              (mem_layout_table[i].size == 0) ) {
            LOCAL_LOG_MSG( 1, 
                           "WARNING:  Memory Layout entry '%s' has size=0.\n", 
                           mem_layout_table[i].name );
            mem_layout_table[i].size = 0;
         }

         /* align is optional, so ne need to check return value. */
         config_get_int( child_node, 
                         "align", 
                         (int *)&mem_layout_table[i].alignment );

         config_ret = config_get_int( child_node, 
                                      "pmr", 
                                      (int *)&mem_layout_table[i].pmr_type );
         if ( config_ret != CONFIG_SUCCESS ) {
            LOCAL_LOG_MSG( 2, 
                           "WARNING:  Memory Layout entry '%s' has no PMR. "
                           "Using default of %d.\n", 
                           mem_layout_table[i].name, 
                           DEFAULT_PMR_TYPE );
            mem_layout_table[i].pmr_type = DEFAULT_PMR_TYPE;
         }

         if ( !g_enable_pmr && 
              (mem_layout_table[i].pmr_type != DEFAULT_PMR_TYPE) ) {
            LOCAL_LOG_MSG( 2, 
                           "WARNING:  Memory Layout entry '%s' specifies "
                           "PMR=%d, but PMRs are disabled.  Using default "
                           "of %d.\n", 
                           mem_layout_table[i].name, 
                           mem_layout_table[i].pmr_type, 
                           DEFAULT_PMR_TYPE );
            mem_layout_table[i].pmr_type = DEFAULT_PMR_TYPE;
         }

         /* 
          * Calculate the best alignment and possibly adjust the size if the 
          * specified alignment is larger than the size.
          */
         check_alignment_boundry( &mem_layout_table[i] );

         LOCAL_LOG_MSG( 4, 
                        "Layout entry '%36s': size=%08X, align=%08X, "
                        "adj_size=%08X, pmr=%3d\n", 
                        mem_layout_table[i].name, 
                        mem_layout_table[i].size, 
                        mem_layout_table[i].alignment, 
                        mem_layout_table[i].adjusted_size, 
                        mem_layout_table[i].pmr_type);
         i++;
         config_ret = config_node_next_sibling(child_node, &child_node);
      }

      if ( i >= MAX_MEM_LAYOUT_ENTRIES ) {
         LOCAL_LOG_MSG( 0, 
                        "ERROR: Number of memory layout entries exceeds "
                        "maximum of %d.  Additional entries ignored.\n", 
                        MAX_MEM_LAYOUT_ENTRIES );
      }
      mem_layout_count = i;
 
      LOCAL_LOG_MSG( 2, 
                     "Read %d memory layout entries from platform_config.\n", 
                     mem_layout_count );
   }

   /*
    * Add the layout entries to lists in their respective PMRs.  The lists
    * will be sorted by alignment (highest first).
    */
   for ( i = 0; i < mem_layout_count; i++ ) {
      add_layout_entry_to_pmr( &mem_layout_table[i] );
   }

   LOCAL_LOG_MSG( 2, "Processed %d memory layout records\n", mem_layout_count );
}


/*
 * Find the PMR within the given MEU region that fits best within the given 
 * size. Return a pointer to the config structure for the PMR.  If no PMR 
 * region is found, return NULL.
 */
static pmr_config_t *
find_best_fit_pmr( meu_config_t *meu,
                   unsigned int size )
{
   pmr_config_t *pmr           = NULL;
   pmr_config_t *best_fit_pmr  = NULL;
   unsigned int  best_fit_size = 0;

   if ( meu != NULL ) {
      pmr = meu->pmrs;

      while ( pmr != NULL ) {
         /* If this PMR is the best fit so far... */
         if ( (pmr->size > best_fit_size) && (pmr->size <= size) ) {
            best_fit_pmr  = pmr;
            best_fit_size = pmr->size;
         }
         pmr = pmr->next;
      }
   }

   return best_fit_pmr;
}


/*
 * Find the layout within the given PMR region that fits best within the given 
 * size. Return a pointer to the layout structure.  If no layout is found, 
 * return NULL.
 */
static mem_layout_t *
find_best_fit_layout( pmr_config_t *pmr,
                      unsigned int  size,
                      unsigned int  alignment )
{
   mem_layout_t *layout          = NULL;
   mem_layout_t *best_fit_layout = NULL;
   unsigned int  best_fit_size   = 0;

   if ( pmr != NULL ) {
      layout = pmr->layouts;
      
      while ( layout != NULL ) {
         if ( (layout->adjusted_size > best_fit_size) && 
              (layout->adjusted_size <= size)         && 
              (layout->alignment <= alignment) ) {
            best_fit_layout = layout;
            best_fit_size   = layout->adjusted_size;
         }
         layout = layout->next;
      }
   }

   return best_fit_layout;
}


/*
 * Insert a layout entry into the global memory map.  The base address of the 
 * PMR where the layout entry resides must be initialized before calling this 
 * function.
 */
void 
insert_layout_into_memory_map( mem_layout_t *layout, 
                               pmr_config_t *pmr, 
                               unsigned int  offset )
{
   assert( pmr != NULL && layout != NULL ); 

   layout->ram_base = pmr->meu_base + offset;
   layout->base_pa  = pmr->base_pa + offset;

   update_layout_entry( layout );
   LOCAL_LOG_MSG( 4, 
                  "Layout: '%36s' base_pa: %08x size: %08x align: "
                  "0x%08X pmr: %3d\n", 
                  layout->name, 
                  layout->base_pa, 
                  layout->size, 
                  layout->alignment, 
                  layout->pmr_type );
}


/*
 * Insert a PMR into the global memory map.  The base_pa of the PMR must be 
 * initilized to the correct physical address before calling this function.
 * This function walks through each memory layout entry in the PMR, assigns 
 * it a physical address, and updates the address in the config_database.
 */
void 
insert_pmr_into_memory_map( pmr_config_t *pmr,
                            uint32_t      base_pa )
{
   mem_layout_t *layout     = NULL;
   uint32_t      pmr_offset = 0;

   pmr->base_pa = base_pa;
   layout = pmr->layouts;
   while ( layout != NULL ) {
      insert_layout_into_memory_map( layout, pmr, pmr_offset );
      pmr_offset += layout->adjusted_size;
      layout = layout->next;
   }

   assert( pmr->size == ROUND_UP(pmr_offset, PMR_ALIGNMENT) || 
           pmr->type == PMR_INVALID );
}


/*
 * Fill a gap in memory with full PMRs and/or individual layout entries.
 * Any PMR inserted into the gap cannot reside within an MEU region.
 * Any individual layout entry inserted cannot reside within a PMR.
 */
static void 
fill_memory_gap( unsigned int gap_base, 
                 unsigned int gap_size )
{
   pmr_config_t *pmr    = NULL;
   meu_config_t *meu    = NULL;
   mem_layout_t *layout = NULL;

   if ( gap_size > 0 ) {

      LOCAL_LOG_MSG( 4, 
                     "Attempting to fill gap at 0x%08X of size 0x%08X\n", 
                     gap_base, 
                     gap_size );

      /* 
       * Make sure that gap_base is on a PMR alignment boundary.  This should 
       * always be true since PMRs are 1MB aligned, media_base_address is 
       * 1MB-aligned, and MEU windows are 64MB aligned.
       */
      assert( (gap_base & (PMR_ALIGNMENT-1)) == 0 );
      
      meu = lookup_meu_by_region_id( MEU_DISABLED );
      while ( (gap_size > 0) && (meu != NULL) ) { 
         /* Find the largest PMR (non-MEU) that can fit into the gap. */
         pmr = find_best_fit_pmr( meu, gap_size );
         if ( pmr != NULL ) {
            /* Remove the PMR from the non-MEU list. */
            remove_pmr_from_meu_region( meu, pmr );

            LOCAL_LOG_MSG( 4, 
                           "Inserting PMR %s with size 0x%08X into memory map "
                           "at 0x%08X\n", 
                           pmr->name, 
                           pmr->size, 
                           gap_base );

            /* Insert the PMR and all of it's layout entries at gap_base. */
            insert_pmr_into_memory_map( pmr, gap_base );

            gap_size -= pmr->size;
            gap_base += pmr->size;
         }
         else {
            break;
         }
      }

      pmr = lookup_pmr_by_type( PMR_INVALID );
      while ( (gap_size > 0) && (pmr != NULL) ) {
         /* 
          * Find the largest layout entry (non-PMR, non-MEU) that can fit 
          * into the gap. 
          */
         layout = find_best_fit_layout( pmr, 
                                        gap_size, 
                                        LARGEST_PWR_OF_2(gap_base) );
         if ( layout != NULL ) {
            /* Remove the layout from the non-PMR list. */
            remove_layout_from_pmr( pmr, layout );
            pmr->size -= layout->adjusted_size;

            LOCAL_LOG_MSG( 4, 
                           "Inserting Layout %s with size 0x%08X into memory "
                           "map at 0x%08X\n", 
                           layout->name, 
                           layout->adjusted_size, 
                           gap_base );

            /* Insert layout entry at gap_base. */
            insert_layout_into_memory_map( layout, 
                                           pmr, 
                                           gap_base - pmr->meu_base );

            gap_size -= layout->adjusted_size;
            gap_base += layout->adjusted_size;
         }
         else {
            break;
         }

      }

      LOCAL_LOG_MSG( 4, "Gap size remaining 0x%08X\n", gap_size );
      if ( gap_size >= GAP_WARN_THRESHOLD ) {
         LOCAL_LOG_MSG( 0, 
                        "WARNING: Memory Layout alignment restrictions waste "
                        "0x%08X bytes of physical memory.\n", gap_size );
      }
   }
}


/* 
 * Go through the list of MEU configurations and PMR configurations and 
 * add up all of the sizes of the layout entries within a PMR to determine 
 * the size of the PMR and add up all of the sizes of the PMRs to determine
 * the size of the MEU region.
 *
 * This operation is now required due to MEU region alignment restrictions.  
 * We may need to be able to move entire PMRs in order to fill in gaps before 
 * or after MEU regions.
 */
void
calculate_meu_pmr_sizes( void )
{
   int           i      = 0;
   meu_config_t *meu    = NULL;
   pmr_config_t *pmr    = NULL;
   mem_layout_t *layout = NULL;

   for ( i = 0; i < (NUM_MEU_REGIONS+1); i++ ) {
      meu = &meu_config[i];
      meu->size = 0;
      pmr = meu->pmrs;
      while ( pmr != NULL ) {
         pmr->size = 0;
         layout = pmr->layouts;
         while ( layout != NULL ) {
            pmr->size += layout->adjusted_size;
            layout = layout->next;
         }
         if ( pmr->type != PMR_INVALID ) {
            pmr->size = ROUND_UP( pmr->size, PMR_ALIGNMENT );
         }
         meu->size += pmr->size;
         pmr = pmr->next;
      }
      if ( meu->size > meu->max_size ) {
         LOCAL_LOG_MSG( 0, 
                        "WARNING: MEU region %d Actual size of 0x%08X exceeds "
                        "maximum size of 0x%08X.\n", 
                        meu->size, 
                        meu->max_size, 
                        meu->region_id );
      }
   }
}


/*
 *  Walk through the memory layout table and coalesce layout entries into their
 *  appropriate PMRs and also put the PMRs into their appropriate MEU regions.
 */
static void
configure_pmrs( void )
{
   meu_config_t * meu        = NULL;
   pmr_config_t * pmr        = NULL;
   unsigned int   meu_base   = 0;
   unsigned int   meu_offset = 0;
   int            i          = 0;

   /* Calculate sizes of all PMRs and MEU regions. */
   calculate_meu_pmr_sizes();

   /*
    * Establish the base address of the first MEU/PMR at the media base 
    * address. PMRs and MEUs have alignment restrictions, so we adjust 
    * the base address if neccessary
    */
   meu_base = ROUND_UP( g_media_base_address, PMR_ALIGNMENT );
   if ( meu_base != g_media_base_address ) {
      LOCAL_LOG_MSG( 0, 
                     "WARNING: media_base_address 0x%08X is not aligned on a "
                     "0x%X boundary. Adjusted to 0x%08X\n", 
                     g_media_base_address, 
                     PMR_ALIGNMENT, 
                     meu_base );
   }

   /* For each MEU region, determine it's physical base address. */
   for ( i = 0; i < (NUM_MEU_REGIONS + 1); i++ ) {
      meu = &meu_config[i];

      /* If the MEU is empty, skip to the next one. */
      if ( meu->size == 0 ) {
         LOCAL_LOG_MSG( 4, 
                        "Skipping MEU configuration for MEU region %d with "
                        "size = 0.\n", 
                        i );
         continue;
      }

      /* Fill in any gap between current address and the base of meu region. */
      meu->phys_base = ROUND_UP( meu_base, meu->alignment );
      fill_memory_gap( meu_base, meu->phys_base - meu_base );
      meu_base = meu->phys_base;
      meu_offset = 0;

      /* For each PMR within the MEU, determine the PMR's base address. */
      pmr = meu->pmrs;
      while ( pmr != NULL ) {
         pmr->meu_base = meu->phys_base + meu_offset;
         /* 
          * If the PMR resides in an MEU region, we adjust the PMR's base  
          * address to be in that region.  Otherwise, the PMR's base 
          * address is the same as the physical address.
          */
         if ( meu->region_id != MEU_DISABLED ) {
            pmr->base_pa = meu->region_base + meu_offset;
         }
         else {
            pmr->base_pa = meu->phys_base + meu_offset;
         }

         insert_pmr_into_memory_map( pmr, pmr->base_pa );
 
         LOCAL_LOG_MSG( 4, 
                        "PMR: %2d (%14s) base_pa: %08x size: %08x meu_base: "
                        "%08x \n", 
                        pmr->type, 
                        pmr->name, 
                        pmr->base_pa, 
                        pmr->size, 
                        pmr->meu_base );

         meu_offset += pmr->size;
         pmr = pmr->next;
      }

      LOCAL_LOG_MSG( 4, 
                     "MEU: %2d region_base: %08x phys_base:%08x size: %08x\n", 
                     meu->region_id, 
                     meu->region_base, 
                     meu->phys_base, 
                     meu->size );

      meu_base += meu->size;
   }

}


/*
 *  Convert the pmr_config_t to a valid memory region layout entry
 *  as would appear in the memory layout file of platform config
 */
static void
convert_pmr_to_config_string( char         *cfg_buf, 
                              pmr_config_t *pmr )
{
   /* Format string for sprintf of pmr entry */
   char *cfg_format = "%s { base_pa = 0x%08X size = 0x%08X type = %d "
                      "name = \"%s\" sta = 0x%08X meu_region = %d "
                      "meu_base = 0x%08X }";

   assert( NULL != cfg_buf );
   assert( NULL != pmr );

   sprintf( cfg_buf, 
            cfg_format, 
            pmr->name, 
            pmr->base_pa, 
            pmr->size, 
            pmr->type, 
            pmr->name, 
            pmr->sta, 
            pmr->meu_region, 
            pmr->meu_base );

   LOCAL_LOG_MSG( 4, "%s\n", cfg_buf );
}


/* Write the PMR configuration table into the platform config database. */
static config_result_t
output_pmrs( void )
{
   config_result_t conf_res      = CONFIG_SUCCESS;
   config_ref_t    pmr_info_node = ROOT_NODE;
   pmr_config_t *  current       = NULL;
   int             i             = 0;
   char            pmr_config_str[512]; /* holds PMR entry in string form. */

   /* Create a new node in the database to for the PMR entries */
   conf_res = config_set_int( ROOT_NODE, CONFIG_PATH_PLATFORM_PMR_INFO, 0 );
   if ( CONFIG_SUCCESS != conf_res ) {
      LOCAL_LOG_MSG( 0, 
                     "ERROR: Could not create config database entry \"%s\"\n", 
                     CONFIG_PATH_PLATFORM_PMR_INFO );
   }
   else {
      conf_res = config_node_find( ROOT_NODE, 
                                   CONFIG_PATH_PLATFORM_PMR_INFO, 
                                   &pmr_info_node );
      if ( CONFIG_SUCCESS != conf_res ) {
         LOCAL_LOG_MSG( 0, 
                        "ERROR: Could not find config database location "
                        "\"%s\"\n", 
                        CONFIG_PATH_PLATFORM_PMR_INFO );
      }
      else {
         /* Write out the PMR entries. */
         for ( i = 0; i < (NUM_PMRS + 1); i++ ) {
            current = &pmr_config[i];

            /* Don't write out the "PMR_INVALID" entry or empty entries.  */
            if ( (current->type != PMR_INVALID) && (current->size != 0) ) {
               LOCAL_LOG_MSG( 4, "Outputting PMR %s: \n",current->name );
               memset( pmr_config_str, 0, sizeof( pmr_config_str ) );
               convert_pmr_to_config_string( pmr_config_str, current );
               conf_res = config_load( pmr_info_node, 
                                       (const char *)pmr_config_str, 
                                       strlen(pmr_config_str) );
               if( CONFIG_SUCCESS != conf_res ) {
                  LOCAL_LOG_MSG( 0, 
                                 "ERROR: Failed to write PMR %d config to "
                                 "platform_config. Error %d\n", 
                                 current->type, 
                                 conf_res );
               }
            }

            current++;
         }
      }
   }

   return conf_res;
}


/*
 *  Convert an meu_config_t structure to a valid entry that can be written 
 *  to the platform_config meu_info.
 *
 *  Also creates a memory layout placeholder to make it clear what physical 
 *  RAM is assigned to an MEU region.
 */
static void
convert_meu_to_config_string( char         *cfg_buf, 
                              char         *layout_buf, 
                              meu_config_t *meu )
{
   /* Format string for sprintf of meu entry */
   char *cfg_format ;

   assert( NULL != cfg_buf );
   assert( NULL != layout_buf );
   assert( NULL != meu );

   cfg_format = "region%d { region_id = %d region_base = 0x%08X "
                "phys_base = 0x%08X size = 0x%08X }";
   sprintf( cfg_buf, 
            cfg_format, 
            meu->region_id, 
            meu->region_id, 
            meu->region_base, 
            meu->phys_base, 
            meu->size );
   LOCAL_LOG_MSG( 4, "MEU Config Entry: %s\n", cfg_buf );

   cfg_format = "meu_region%d { base = 0x%08X size = 0x%08X align = 0x%08X }";
   sprintf( layout_buf, 
            cfg_format, 
            meu->region_id, 
            meu->phys_base, 
            meu->size, 
            PMR_ALIGNMENT );
   LOCAL_LOG_MSG( 4, "MEU Layout Entry: %s\n", layout_buf );
}


/* Write the MEU configuration table into the platform config database. */
static config_result_t
output_meus( void )
{
   config_result_t conf_res      = CONFIG_SUCCESS;
   config_ref_t    meu_info_node = ROOT_NODE;
   config_ref_t    layout_node   = ROOT_NODE;
   meu_config_t *  current       = NULL;
   int             i             = 0;
   char            meu_config_str[256]; /* holds MEU entry in string form. */
   char            meu_layout_str[256]; /* holds layout entry in string form */

   /* Create a new node in the database to for the MEU entries */
   conf_res = config_set_int( ROOT_NODE, 
                              CONFIG_PATH_PLATFORM_MEU_INFO, 
                              0 );
   if ( CONFIG_SUCCESS != conf_res ) {
      LOCAL_LOG_MSG( 0, 
                     "ERROR: could not create config database entry \"%s\"\n", 
                     CONFIG_PATH_PLATFORM_MEU_INFO );
   }
   /* Get the node of the MEU entries. */
   else if ( (conf_res = config_node_find(ROOT_NODE, 
                                          CONFIG_PATH_PLATFORM_MEU_INFO, 
                                          &meu_info_node)) != CONFIG_SUCCESS ) {
      LOCAL_LOG_MSG( 0, 
                     "ERROR: could not find config database location \"%s\"\n", 
                     CONFIG_PATH_PLATFORM_MEU_INFO );
   }
   /* Get the node of the memory layout entries. */
   else if ( (conf_res = config_node_find(ROOT_NODE, 
                                          CONFIG_PATH_PLATFORM_MEMORY_LAYOUT, 
                                          &layout_node)) != CONFIG_SUCCESS ) {
      LOCAL_LOG_MSG( 0, 
                     "ERROR: could not find config database location \"%s\"\n", 
                     CONFIG_PATH_PLATFORM_MEMORY_LAYOUT );
   }
   else {
      /* Write out the MEU table entries. */
      for ( i = 0; i < (NUM_MEU_REGIONS + 1); i++ ) {
         current = &meu_config[i];

         /* Don't write out the MEU_DISABLED entry or any empty entries.  */
         if ( (current->region_id != MEU_DISABLED) && (current->size != 0) ) {
            LOCAL_LOG_MSG( 4, 
                           "Outputting MEU Region %d: \n", 
                           current->region_id );
            memset( meu_config_str, 0, sizeof( meu_config_str ) );
            
            convert_meu_to_config_string( meu_config_str, 
                                          meu_layout_str, 
                                          current );

            conf_res = config_load( meu_info_node, 
                                    (const char *)meu_config_str, 
                                    strlen(meu_config_str) );
            if( CONFIG_SUCCESS != conf_res ) {
               LOCAL_LOG_MSG( 0, 
                              "ERROR: Failed to write MEU region %d config to "
                              "platform_config. Error %d\n", 
                              current->region_id, 
                              conf_res );
            }

            conf_res = config_load( layout_node, 
                                    (const char *)meu_layout_str, 
                                    strlen(meu_layout_str) );            
            if( CONFIG_SUCCESS != conf_res ) {
               LOCAL_LOG_MSG( 0, 
                              "ERROR: Failed to write MEU region %d layout "
                              "entry to platform_config. Error %d\n", 
                              current->region_id, 
                              conf_res );
            }

         }

         current++;
      }
   }

   return conf_res;
}


/*
 *  Retrieve the pmr settings from the platform_config tree and print them out.
 *  This is intended for debugging and also as sample code for other modules 
 *  that need to read the PMR settings from platform_config.
 */
void 
get_pmr_info( void )
{
   config_result_t config_ret = CONFIG_SUCCESS;
   config_ref_t    pmr_node   = ROOT_NODE;
   config_ref_t    child_node = ROOT_NODE;
   int             i          = 0;
   pmr_config_t    pmr;

   LOCAL_LOG_MSG( 3, 
                  "\n\nRetrieving PMRs from config path %s.\n", 
                  CONFIG_PATH_PLATFORM_PMR_INFO );

   config_ret = config_node_find( ROOT_NODE, 
                                  CONFIG_PATH_PLATFORM_PMR_INFO, 
                                  &pmr_node );
   if ( CONFIG_SUCCESS != config_ret ) {
      LOCAL_LOG_MSG( 3, 
                     "No PMRs found at %s.\n", 
                     CONFIG_PATH_PLATFORM_PMR_INFO );
   }
   else {
      config_ret = config_node_first_child( pmr_node, &child_node );

      for ( i = 0; (config_ret == CONFIG_SUCCESS) && (i < NUM_PMRS); i++ ) {

         config_ret = config_node_get_name( child_node, 
                                            pmr.name, 
                                            MAX_LAYOUT_NAME_SIZE );
         pmr.name[MAX_LAYOUT_NAME_SIZE] = '\0';
         if ( CONFIG_SUCCESS != config_ret ) {
            LOCAL_LOG_MSG( 0, "Failed to find the name of PMR type %d.\n", i );
            break;
         }

         config_ret = config_get_int( child_node, 
                                      "base_pa", 
                                      (int *)&pmr.base_pa );
         if ( CONFIG_SUCCESS != config_ret ) {
            LOCAL_LOG_MSG( 0, 
                           "Failed to find the base physical address of "
                           "PMR type %d.\n", i );
            break;
         }

         config_ret = config_get_int( child_node, 
                                      "size", 
                                      (int *)&pmr.size );
         if ( CONFIG_SUCCESS != config_ret ) {
            LOCAL_LOG_MSG( 0, "Failed to find the size of PMR type %d.\n", i );
            break;
         }

         config_ret = config_get_int( child_node, 
                                      "type", 
                                      (int *)&pmr.type );
         if ( CONFIG_SUCCESS != config_ret ) {
            LOCAL_LOG_MSG( 0, "Failed to find the type of PMR type %d.\n", i );
            break;
         }

         config_ret = config_get_int( child_node, "sta", (int *)&pmr.sta );
         if ( CONFIG_SUCCESS != config_ret ) {
            LOCAL_LOG_MSG( 0, 
                           "Failed to find the security attributes of PMR "
                           "type %d.\n", 
                           i );
            break;
         }

         config_ret = config_node_next_sibling( child_node, &child_node );

         LOCAL_LOG_MSG( 3, 
                        "%-14s { type = %3d base_pa = 0x%08X "
                        "size = 0x%08X sta = 0x%08X }\n", 
                        pmr.name, 
                        pmr.type, 
                        pmr.base_pa, 
                        pmr.size, 
                        pmr.sta );
      }

      LOCAL_LOG_MSG( 3, "Retrieved %d PMR entries.\n", i );
   }

}


/* Compare function for qsort() library function. */
static int 
compare_mem_layout_addresses( const mem_layout_t *layout1, 
                              const mem_layout_t *layout2 ) 
{
   int result = 0;

   if ( layout1->ram_base < layout2->ram_base ) {
      result = -1;
   }
   else if ( layout1->ram_base > layout2->ram_base ) {
      result = 1;
   }

   return result;
}

/*
 * Print the memory layout entries sorted by their actual physical base 
 * address in RAM.  This is intended for debugging.  The sorting makes it 
 * easier to map the memory from RAM to the MEU windows and also to easier 
 * to determine where any gaps are located.
 */
static void 
print_sorted_mem_layout( void )
{
   int i;

   qsort( mem_layout_table, 
          mem_layout_count, 
          sizeof(mem_layout_t), 
          compare_mem_layout_addresses );

   for ( i = 0; i < mem_layout_count; i++ ) {
      LOCAL_LOG_MSG( 4, 
                     "%36s { base = 0x%08X (0x%08X), size = 0x%08X, "
                     "align = 0x%08X, pmr = %3d }\n",
                     mem_layout_table[i].name, 
                     mem_layout_table[i].base_pa, 
                     mem_layout_table[i].ram_base, 
                     mem_layout_table[i].size, 
                     mem_layout_table[i].alignment, 
                     mem_layout_table[i].pmr_type );
   }
}


/* Print the command-line help instructions. */
void
usage( char *app_name )
{
   LOCAL_LOG_MSG( 0, "\nUsage:  %s [options]\n", app_name );
   LOCAL_LOG_MSG( 0, "\tOptions:\n"  );
   LOCAL_LOG_MSG( 0, "\t  -v=N : set verbosity level to N\n" );
   LOCAL_LOG_MSG( 0, "\t         0 <= N <= 4, default is %d\n\n", 
                  DEFAULT_LOCAL_DEBUG_LEVEL );
   exit( -1 );
}


/* 
 * Simplified command-line parsing, supports help and verbosity.
 */
void
parse_command_line_args( int    argc,
                         char **argv )
{
   /* Too few options. */
   if ( argc < 2 ) {
      return;
   }

   /* Too many options. */
   else if ( argc > 2 ) {
      usage( argv[0] );
   }

   /* option "-v=N" to set the verbosity level */
   else if ( (argv[1][0] == '-') &&
             (argv[1][1] == 'v') &&
             (argv[1][2] == '=') ) {
      verbosity_level = atoi( &(argv[1][3]) );
      if ( (verbosity_level < 0) ||
           (verbosity_level > 4) ) {
         usage( argv[0] );
      }
   }
   else {
      usage( argv[0] );
   }

   return;
}


int
main( int    argc,
      char **argv )
{                     
   parse_command_line_args( argc, argv );

   /* Determine if PMRs are explicitly enabled. */
   determine_pmr_mode();
   
   /* Read the TDP configuration and determine if the MEU is enabled. */
   read_tdp_configuration();
   
   /* Get the base address of the MEU regions. */
   get_meu_region_base();

   /* Associated PMRs with their appropriate MEU regions. */
   process_meu_config();

   /* Get the media_base_address from platform_config */
   get_media_base_address();

   /* Get the entire memory layout from platform_config. */
   process_memory_layout();

   /* Organize memory layout entries into PMRs and MEU regions. */
   configure_pmrs();

   /* Write the PMR info back to platform_config. */
   if ( g_enable_pmr ) {
      output_pmrs();
   }

   /* Write the MEU region info back to platform_config. */
   if ( g_enable_meu ) {
      output_meus();
   }

   if ( verbosity_level >= 3 ) {
      print_sorted_mem_layout();
      get_pmr_info();
   }

   LOCAL_LOG_MSG( 0, "MEU is %s.\n", 
                  (g_enable_meu && g_enable_pmr) ? "enabled" : "disabled" );

   LOCAL_LOG_MSG( 0, "PMRs are %s.\n", 
                  g_enable_pmr ? "enabled" : "disabled" );

   return 0;
}

