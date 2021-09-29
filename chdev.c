#include <linux/uaccess.h>  
#include <linux/fs.h>    
#include <linux/device.h>
#include <linux/cdev.h>



#include "net.h"
#include "mmap.h"
#include "config.h"


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
    
    printk( KERN_NOTICE "vrfm: Device file is written at offset = %i, write bytes count = %u\n", (int)*pos, (unsigned int)count );
    struct mmap_info *info = filp->private_data;

    if (count > MAX_BUFFER ) {
		count = MAX_BUFFER;
	}    
    if ( copy_from_user(procfs_buffer, buf, count) ) {
		return -EFAULT;
	}
    
    memcpy(info->data,buf,count-1);
    
    //memcpy(info->data, "Hello from kernel this is file: ", 32);

    //printk( KERN_NOTICE "vrfm: received %s\n" , procfs_buffer);

    sendpacket(procfs_buffer,count-1);
    return count;
}



static struct file_operations vrfm_driver_fops = 
{
    .owner = THIS_MODULE,
	.open = mmap_open,
	.release = mmapfop_close,
    .read = device_file_read,
    .write = complete_write,    
    .mmap = memory_map,
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