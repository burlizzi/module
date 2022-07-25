#include <linux/uaccess.h>  
#include <linux/fs.h>    
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/kernel.h>
#include <linux/version.h>


#include "net.h"
#include "mmap.h"
#include "config.h"
#include "rfm2g/include/rfm2g_defs.h"
#include "rfm2g/include/rfm2g_struct.h"


#define MAX_BUFFER 1024

char procfs_buffer[MAX_BUFFER];







struct class *cl;
struct cdev cdev;
dev_t dev=0;



int majorNum;
dev_t devNo;  // Major and Minor device numbers combined into 32 bits
struct class *pClass;  // class_create will set this


static char *vrfm_devnode(struct device *dev, umode_t *mode)
{
        if (!mode)
                return NULL;
        *mode = 0666;
        return NULL;
}



//static const char g_s_Hello_World_string[] = "Hello world from kernel mode!\n\0";
//static const ssize_t g_s_Hello_World_size = sizeof(g_s_Hello_World_string);
extern int size;
/*===============================================================================================*/
ssize_t device_file_read(
    struct file *file_ptr
    , char __user *user_buffer
    , size_t count
    , loff_t *position)
{
    struct mmap_info *info = file_ptr->private_data;
    
    printk( KERN_NOTICE "vrfm: Device file is read at offset = %i, read bytes count = %u\n"
        , (int)*position
        , (unsigned int)count );

    if( *position >= size )
        return 0;

    if( *position + count > size )
        count = size - *position;

    //if( copy_to_user(user_buffer, g_s_Hello_World_string + *position, count) != 0 )
    if( copy_to_user(user_buffer, info->data + *position, count) != 0 )
        return -EFAULT;

    *position += count;
    
    return count;
}

ssize_t complete_write(struct file *filp,const char __user *buf,size_t count,loff_t *pos)
{
    
    struct mmap_info *info = filp->private_data;
    size_t i;
    size_t reminder=count&(PAGE_SIZE-1);

    printk( KERN_NOTICE "vrfm: Device file is written at offset = %i, write bytes count = %u\n", (int)*pos, (unsigned int)count );

    if (count > MAX_BUFFER ) {
		count = MAX_BUFFER;
	}    
    if ( copy_from_user(procfs_buffer, buf, count) ) {
		return -EFAULT;
	}
    
    for (i = 0; i < count>>PAGE_SHIFT; i++)
    {
        info->data[i]=(char *)__get_free_pages(GFP_KERNEL, PAGES_ORDER);
        memset(info->data[i],0,PAGE_SIZE<<PAGES_ORDER);
        memcpy(info->data[i],procfs_buffer+(i<<PAGE_SHIFT),PAGE_SIZE<<PAGES_ORDER);
    }

    memcpy(info->data[i],procfs_buffer+reminder,reminder);
    

    
    
    //memcpy(info->data, "Hello from kernel this is file: ", 32);

    //printk( KERN_NOTICE "vrfm: received %s\n" , procfs_buffer);

    sendpacket(0);
    return count;
}

#define RFM2G_MAGIC 0xeb
#define IOCTL_RFM2G_GET_CONFIG               _IOWR(RFM2G_MAGIC, 30, struct RFM2GCONFIG_)
#define RFM2G_PRODUCT_STRING    "SW-RFM2G-DRV-LNX"
#define RFM2G_PRODUCT_OS        "LINUX"
#define RFM2G_PRODUCT_VERSION   "R09.00"

RFM2GCONFIGLINUX mycfg;
long rfm2g_ioctl(struct file *filp, unsigned int cmd, unsigned long arg )
{
    int ret_status = 0;
    printk(KERN_ALERT "IOCTRL: %x\n", cmd);
    RFM2GCONFIGLINUX *cfg=&mycfg;

    switch( cmd )
    {
        case IOCTL_RFM2G_GET_CONFIG:
        {
			RFM2GCONFIG Config;
            char *myself = "GET_CONFIG";


			/* Copy the common stuff over */
			Config.NodeId              = cfg->NodeId;
			Config.BoardId             = cfg->BoardId;
			Config.Unit                = cfg->Unit;
			Config.PlxRevision         = cfg->PlxRevision;
			Config.MemorySize          = cfg->MemorySize;
			strcpy(Config.Device, "ciao");
			strcpy(Config.Name, RFM2G_PRODUCT_STRING);
			strcpy(Config.DriverVersion, RFM2G_PRODUCT_VERSION);
			Config.BoardRevision       = cfg->RevisionId;
			Config.RevisionId          = cfg->PCI.revision;
            Config.BuildId             = cfg->BuildId;
            memcpy(&Config.PciConfig, &cfg->PCI, sizeof(RFM2GPCICONFIG));

			/* Read LCSR1 */
			Config.Lcsr1 = 123;//(RFM2G_UINT32) readl( (char *)(( (RFM2G_ADDR) cfg->pCsRegisters + rfm2g_lcsr )));

            /* Copy the data back to the user */
            if( copy_to_user( (void *)arg, (void *)&Config,
                sizeof(RFM2GCONFIG) ) > 0 )
            {
                return( -EFAULT );
            }
        }
        break;    
    }
    return( ret_status );
}


static struct file_operations vrfm_driver_fops = 
{
    .owner = THIS_MODULE,
	.open = mmap_open,
	.release = mmapfop_close,
    .read = device_file_read,
    .write = complete_write,    
    .mmap = memory_map,
    unlocked_ioctl: rfm2g_ioctl,
};

int chdev_init(void)
{
    
   struct device *pDev;
    
  // Register character device
  majorNum = register_chrdev(0, xstr(MODULE_NAME), &vrfm_driver_fops);
  if (majorNum < 0) {
    printk(KERN_ALERT "Could not register device: %d\n", majorNum);
    return majorNum;
  }
  devNo = MKDEV(majorNum, 0);  // Create a dev_t, 32 bit version of numbers

  // Create /sys/class/DEVICE_NAME in preparation of creating /dev/DEVICE_NAME
  pClass = class_create(THIS_MODULE, xstr(DEVICE_NAME));

  pClass->devnode = vrfm_devnode;


  if (IS_ERR(pClass)) {
    printk(KERN_WARNING "\ncan't create class");
    unregister_chrdev_region(devNo, 1);
    return -1;
  }

  // Create /dev/DEVICE_NAME for this char dev
  if (IS_ERR(pDev = device_create(pClass, NULL, devNo, NULL, xstr(DEVICE_NAME)))) {
    printk(KERN_WARNING xstr(MODULE_NAME)".ko can't create device /dev/"xstr(DEVICE_NAME)"\n");
    class_destroy(pClass);
    unregister_chrdev_region(devNo, 1);
    return -1;
  }
  return 0;
}

void chdev_shutdown(void)
{
    device_destroy(pClass, devNo);  // Remove the /dev/DEVICE_NAME
    class_destroy(pClass);  // Remove class /sys/class/DEVICE_NAME
    unregister_chrdev(majorNum, xstr(DEVICE_NAME));  // Unregister the device    
}