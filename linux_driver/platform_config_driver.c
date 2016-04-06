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

#include <linux/init.h>
#include <linux/module.h>
#include <linux/cdev.h>
#include <linux/fs.h>
#include <linux/proc_fs.h>
#include <linux/capability.h>

#include <asm/uaccess.h>

#include "platform_config_drv.h"
//#include "platform_config_compile.h"

//MODULE_VERSION("platform_config  (" LINUX_COMPILE_BY "@"  LINUX_COMPILE_HOST ") (" LINUX_COMPILER ") " UTS_VERSION "\n");

/* These are predefined macros that specify different parameters
 * for our driver */ 
MODULE_AUTHOR("Intel Corporation, (C) 2005-2008 - All Rights Reserved");
MODULE_DESCRIPTION("Platform Configuration Device Driver");
MODULE_SUPPORTED_DEVICE("Intel Media Processors");

/* Notifies the kernel that our driver is not GPL. */
MODULE_LICENSE("Dual BSD/GPL");
//MODULE_LICENSE("GPL");
EXPORT_SYMBOL(config_node_find);
EXPORT_SYMBOL(config_node_first_child);
EXPORT_SYMBOL(config_node_next_sibling);
EXPORT_SYMBOL(config_node_get_name);
EXPORT_SYMBOL(config_node_get_int);
EXPORT_SYMBOL(config_node_get_str);
EXPORT_SYMBOL(config_get_int);
EXPORT_SYMBOL(config_get_str);
EXPORT_SYMBOL(config_set_int);
EXPORT_SYMBOL(config_set_str);
EXPORT_SYMBOL(config_load);
//#define VERBOSE_DEBUG
#ifdef VER
const char *Version_string = "#@# platform_config.ko " VER;
#endif

#define IS_ROOT (capable(CAP_SYS_RAWIO) ? 1:0)

#define PLAT_GET_CONST_NAME( dst, src ) ({              \
            int __err = 0;                              \
            if ((str_len = strlen_user( src )) == 0)                  \
                __err = -EFAULT;			\
            else{					\
                dst = kmalloc(str_len, GFP_KERNEL);         \
            	if (NULL == dst)                            \
                	__err = -ENOMEM;                        \
            	else if (copy_from_user(dst, src, str_len)) {    \
                        kfree(dst);                             \
                        __err = -EFAULT;                        \
                     }                                           \
                }					\
            	__err;})

#define PLAT_GET_CONST_DATA( dst, src,size ) ({         \
            int __err = 0;                              \
            dst = kmalloc(size, GFP_KERNEL);            \
            if (NULL == dst)                            \
                __err = -ENOMEM;                         \
            else if (copy_from_user(dst, src, size)) {       \
                kfree(dst);                             \
                __err = -EFAULT;                        \
            }                                           \
            __err;})
/*-------------------------------------------------------------------------------------------------------------------*/
/*				 Platform configuration device operation interfaces				     */
/*-------------------------------------------------------------------------------------------------------------------*/
static int plat_cfg_open(struct inode *inode, struct file *filp);
static int plat_cfg_release(struct inode *inode, struct file *filp);
static int plat_cfg_ioctl(struct file *filp, unsigned int cmd, unsigned long arg);

static int plat_cfg_init(void);
static void plat_cfg_exit(void);

static int plat_cfg_open(struct inode *inode, struct file *filp)
{
#ifdef VERBOSE_DEBUG
	printk(KERN_INFO "%s:%4i: %s (pid %d) opened platform_config_drv.\n",
		       	__FILE__, __LINE__, current->comm, current->pid);
#endif

	return(0);
}

static int plat_cfg_release(struct inode *inode, struct file *filp)
{
#ifdef VERBOSE_DEBUG
	printk(KERN_INFO "%s:%4i: %s (pid %d) released paltform_config_drv.\n",
		       	__FILE__, __LINE__, current->comm, current->pid);
#endif
	return(0);
}

static int plat_cfg_ioctl(struct file *filp, unsigned int cmd, unsigned long arg)
{
	struct plat_cfg_ioctl pc_args; 
	int status = 0;
	config_result_t pc_status = CONFIG_SUCCESS;
	int val_data = 0, str_len = 0;
	config_ref_t node_data = 0;     
	char *p_const_name = NULL;
	char *p_string = NULL;
	char *p_const_string = NULL;
	char *p_name = NULL;
	char *p_config_data = NULL;
 
	/* Make sure that we haven't erroneously received the ioctl call */
	if (_IOC_TYPE(cmd) != PLATFORM_CONFIG_IOC_MAGIC)
	{
		printk(KERN_ERR "%s:%4i: ioctl command %x does not belong to this driver\n", 
				__FILE__, __LINE__, cmd);
		return -ENOTTY;
	}
#ifdef VERBOSE_DEBUG
	printk(KERN_INFO "%s:%4i: Driver received ioctl command: %x\n", __FILE__, __LINE__, cmd);
#endif
	/* make sure we have a valid pointer to our parameters */
	if (!arg)
		return -EINVAL;
	/* read the parameters from user */
	status = copy_from_user(&pc_args, (void *)arg, sizeof(struct plat_cfg_ioctl));
	
	if (status)
	{
		printk(KERN_ERR "%s:%4i: could not read ioctl arguments \n", 
				__FILE__, __LINE__);
		return status;
	}
#ifdef VERBOSE_DEBUG
	else
	{   
		printk(KERN_INFO "%s:%4i: ioctl arguments (pc_args) are:\n", __FILE__, __LINE__);
		printk(KERN_INFO "%s:%4i: -----base_ref      = %lu\n", __FILE__, __LINE__, pc_args.base_ref);
		printk(KERN_INFO "%s:%4i: -----const_name    = %lux\n", __FILE__, __LINE__, (unsigned long)pc_args.const_name);
		printk(KERN_INFO "%s:%4i: -----name          = %lux\n", __FILE__, __LINE__, (unsigned long)pc_args.name);
		printk(KERN_INFO "%s:%4i: -----const_string  = %lux\n", __FILE__, __LINE__, (unsigned long)pc_args.const_string);
		printk(KERN_INFO "%s:%4i: -----string        = %lux\n", __FILE__, __LINE__, (unsigned long)pc_args.string);
		printk(KERN_INFO "%s:%4i: -----val           = %d\n", __FILE__, __LINE__, pc_args.val);
		printk(KERN_INFO "%s:%4i: -----val_ptr       = %lux\n", __FILE__, __LINE__, (unsigned long)pc_args.val_ptr);
		printk(KERN_INFO "%s:%4i: -----config_data   = %lux\n", __FILE__, __LINE__, (unsigned long)pc_args.config_data);
		printk(KERN_INFO "%s:%4i: -----bufsize       = %d\n", __FILE__, __LINE__, pc_args.bufsize);
		printk(KERN_INFO "%s:%4i: -----node_ptr      = %lux\n", __FILE__, __LINE__, (unsigned long)pc_args.node_ptr);
	}
#endif
	
	switch(cmd) {

		case PLATFORM_CONFIG_IOC_INITIALIZE:
			pc_status = config_initialize();
			if (CONFIG_SUCCESS != pc_status )
			{
				pc_status = -EINVAL;
			}
			break;

		case PLATFORM_CONFIG_IOC_GET_INT:       
            if((pc_status = PLAT_GET_CONST_NAME(p_const_name,pc_args.const_name)) != CONFIG_SUCCESS)
                break;
            pc_status = config_get_int(pc_args.base_ref, p_const_name, &val_data);
            if (CONFIG_SUCCESS != pc_status )
            {
                pc_status = -EINVAL;
            }
            if (put_user(val_data, (int *)pc_args.val_ptr))
                pc_status = -EINVAL;
            kfree(p_const_name);
            break;

		case PLATFORM_CONFIG_IOC_GET_STR:
            if((pc_status = PLAT_GET_CONST_NAME(p_const_name,pc_args.const_name)) != CONFIG_SUCCESS)
                break;
            p_string = kmalloc(pc_args.bufsize, GFP_KERNEL);
            if (NULL == p_string) {
                kfree(p_const_name);
                return -ENOMEM;
            }
            pc_status = config_get_str(pc_args.base_ref, p_const_name, p_string, pc_args.bufsize );
            if (CONFIG_SUCCESS != pc_status )
            {
                pc_status = -EINVAL;
            }
            if (copy_to_user(pc_args.string, p_string, pc_args.bufsize))
                pc_status = -EINVAL;
            kfree(p_const_name);			
            kfree(p_string);
            break;

        case PLATFORM_CONFIG_IOC_SET_INT:
            if (!IS_ROOT)
                return -EACCES;
            if((pc_status = PLAT_GET_CONST_NAME(p_const_name,pc_args.const_name)) != CONFIG_SUCCESS)
                break;
            pc_status = config_set_int(pc_args.base_ref, p_const_name, pc_args.val );
            if (CONFIG_SUCCESS != pc_status )
            {
                pc_status = -EINVAL;
            }
            kfree(p_const_name);
            break;

        case PLATFORM_CONFIG_IOC_SET_STR:
            if (!IS_ROOT)
                return -EACCES;
            if((pc_status = PLAT_GET_CONST_NAME(p_const_name,pc_args.const_name)) != CONFIG_SUCCESS)
                break;
            if((pc_status = PLAT_GET_CONST_DATA(p_const_string, pc_args.const_string, pc_args.bufsize))
                != CONFIG_SUCCESS) {
                kfree(p_const_name);
                break;
            }
            pc_status = config_set_str(pc_args.base_ref, p_const_name, p_const_string, pc_args.bufsize );
            if (CONFIG_SUCCESS != pc_status )
            {
                pc_status = -EINVAL;
            }
            kfree(p_const_name);
            kfree(p_const_string);
            break;

        case PLATFORM_CONFIG_IOC_NODE_FIND:
            if((pc_status = PLAT_GET_CONST_NAME(p_const_name,pc_args.const_name)) != CONFIG_SUCCESS)
                break;
            pc_status = config_node_find(pc_args.base_ref, p_const_name, &node_data );
            if (CONFIG_SUCCESS != pc_status )
            {
                pc_status = -EINVAL;
            }
            if (put_user(node_data, (int *)pc_args.node_ptr))
                pc_status = -EINVAL;
            kfree(p_const_name);
            break;

        case PLATFORM_CONFIG_IOC_NODE_FIRST_CHILD:
            pc_status = config_node_first_child(pc_args.base_ref, &node_data );
            if (CONFIG_SUCCESS != pc_status )
            {
                pc_status = -EINVAL;
            }
            if (put_user(node_data, (int *)pc_args.node_ptr))
                pc_status = -EINVAL;
            break;

        case PLATFORM_CONFIG_IOC_NODE_NEXT_SIBLING:
            pc_status = config_node_next_sibling(pc_args.base_ref, &node_data );
            if (CONFIG_SUCCESS != pc_status )
            {
                pc_status = -EINVAL;
            }
            if (put_user(node_data, (int *)pc_args.node_ptr))
                pc_status = -EINVAL;
            break;

        case PLATFORM_CONFIG_IOC_NODE_GET_NAME:
            p_name = kmalloc(pc_args.bufsize, GFP_KERNEL);
            if (NULL == p_name)
                return -ENOMEM;
            pc_status = config_node_get_name(pc_args.base_ref, p_name, pc_args.bufsize );
            if (CONFIG_SUCCESS != pc_status )
            {
                pc_status = -EINVAL;
            }
            if (copy_to_user(pc_args.name, p_name, pc_args.bufsize))
                pc_status = -EINVAL;
            kfree(p_name);
            break;

        case PLATFORM_CONFIG_IOC_NODE_GET_STR:
            p_string = kmalloc(pc_args.bufsize, GFP_KERNEL);
            if (NULL == p_string)
                return -ENOMEM;
            pc_status = config_node_get_str(pc_args.base_ref, p_string, pc_args.bufsize );
            if (CONFIG_SUCCESS != pc_status )
            {
                pc_status = -EINVAL;
            }
            if (copy_to_user(pc_args.string, p_string, pc_args.bufsize))
                pc_status = -EINVAL;
            kfree(p_string);
            break;

        case PLATFORM_CONFIG_IOC_NODE_GET_INT:        
            pc_status = config_node_get_int(pc_args.base_ref, &val_data );
            if (CONFIG_SUCCESS != pc_status )
            {
                pc_status = -EINVAL;
            }
            if (put_user(val_data, pc_args.val_ptr))
                pc_status = -EINVAL;
            break;

        case PLATFORM_CONFIG_IOC_REMOVE:
        	if (!IS_ROOT)
 	            return -EACCES;
			pc_status = config_private_tree_remove( pc_args.base_ref );
			if (CONFIG_SUCCESS != pc_status )
			{
				pc_status = -EINVAL;
			}
			break;

        case PLATFORM_CONFIG_IOC_LOAD:
            if (!IS_ROOT)
                return -EACCES;
            if((pc_status = PLAT_GET_CONST_DATA(p_config_data, pc_args.config_data, pc_args.bufsize))
                 != CONFIG_SUCCESS) {
                break;
            }
            pc_status = config_load(pc_args.base_ref, p_config_data, pc_args.bufsize );
            if (CONFIG_SUCCESS != pc_status )
            {
                pc_status = -EINVAL;
            }
            kfree(p_config_data);
            break;

		default:
			pc_status = -ENOTTY;
	}
	
	return pc_status;
}

static struct file_operations plat_cfg_fops = {
	.owner		= THIS_MODULE,
	.open	 	= plat_cfg_open,
	.unlocked_ioctl	 	= plat_cfg_ioctl,
	.release 	= plat_cfg_release,
};

static const char configstr[] = 
"platform_config_driver_info\n"
"{\n"
"   source_file = \"" __FILE__ "\"\n"
"   compiled_date = \"" __DATE__ "\"\n"
"}\n";

static int plat_cfg_init(void)
{
    config_result_t          config_result;
    struct proc_dir_entry   *proc;

    //remove_proc_entry(PLATFORM_CONFIG_DEVICE_NAME, NULL);

	proc = create_proc_entry( PLATFORM_CONFIG_DEVICE_NAME, S_IRUSR|S_IWUSR | S_IRGRP |S_IWGRP |S_IROTH |S_IWOTH, NULL);
    if ( NULL != proc )
    {
		memcpy(&plat_cfg_fops, proc->proc_fops, sizeof(plat_cfg_fops));
		plat_cfg_fops.open = plat_cfg_open;
		plat_cfg_fops.unlocked_ioctl = plat_cfg_ioctl;
		plat_cfg_fops.release = plat_cfg_release;
		proc->proc_fops = &plat_cfg_fops;
    }
    else
    {
        printk("create_proc_entry(\"%s\") failed\n", PLATFORM_CONFIG_DEVICE_NAME );
        return -EIO;
    }

    /* create the empty database */
    config_result = config_initialize();
    if ( CONFIG_SUCCESS != config_result )
        printk( "config_initialize() err %d\n", config_result );

    /* create the empty database */
    config_result = config_load( 0, configstr, sizeof(configstr) );
    if ( CONFIG_SUCCESS != config_result )
        printk( "config_load() err %d\n", config_result );

	printk( PLATFORM_CONFIG_DEVICE_NAME " init complete! (build " __DATE__ ")\n");
	return(0);
}

static void plat_cfg_exit(void)
{
	config_deinitialize();

    remove_proc_entry(PLATFORM_CONFIG_DEVICE_NAME, NULL);

    printk( KERN_NOTICE PLATFORM_CONFIG_DEVICE_NAME " unloaded\n");
}

module_init(plat_cfg_init);
module_exit(plat_cfg_exit);

