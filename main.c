#include <linux/init.h>       /* module_init, module_exit */
#include <linux/module.h>     /* version info, MODULE_LICENSE, MODULE_AUTHOR, printk() */
#include <linux/uaccess.h>  
#include <linux/fs.h>    
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/mm.h>
#include <linux/mutex.h>
#include <linux/kernel.h>
#include <linux/slab.h>



#include "net.h"
#include "mmap.h"
#include "config.h"

MODULE_DESCRIPTION("Virtual Reflective Memory Linux driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Luca Burlizzi");
MODULE_SOFTDEP("e1000e");

static DEFINE_MUTEX(mmap_device_mutex);








char procfs_buffer[MAX_BUFFER];


struct class *cl;
struct cdev cdev;
dev_t dev=0;





static const char g_s_Hello_World_string[] = "Hello world from kernel mode!\n\0";
static const ssize_t g_s_Hello_World_size = sizeof(g_s_Hello_World_string);

/*===============================================================================================*/
static ssize_t device_file_read(
    struct file *file_ptr
    , char __user *user_buffer
    , size_t count
    , loff_t *position)
{
    printk( KERN_NOTICE "vrfm: Device file is read at offset = %i, read bytes count = %u\n"
        , test1()
        , (unsigned int)count );

    if( *position >= g_s_Hello_World_size )
        return 0;

    if( *position + count > g_s_Hello_World_size )
        count = g_s_Hello_World_size - *position;

    if( copy_to_user(user_buffer, g_s_Hello_World_string + *position, count) != 0 )
        return -EFAULT;

    *position += count;
    
    return count;
}

/*

int dad_transfer(struct dad_dev *dev, int write, void *buffer, 
                 size_t count)
{
    dma_addr_t bus_addr;

    // Map the buffer for DMA 
    dev->dma_dir = (write ? DMA_TO_DEVICE : DMA_FROM_DEVICE);
    dev->dma_size = count;
    bus_addr = dma_map_single(&dev->pci_dev->dev, buffer, count, 
                              dev->dma_dir);
    dev->dma_addr = bus_addr;

    // Set up the device 

    writeb(dev->registers.command, DAD_CMD_DISABLEDMA);
    writeb(dev->registers.command, write ? DAD_CMD_WR : DAD_CMD_RD);
    writel(dev->registers.addr, cpu_to_le32(bus_addr));
    writel(dev->registers.len, cpu_to_le32(count));

    // Start the operation 
    writeb(dev->registers.command, DAD_CMD_ENABLEDMA);
    return 0;
}
 */


ssize_t complete_write(struct file *filp,const char __user *buf,size_t count,loff_t *pos)
{
    
    //printk( KERN_NOTICE "vrfm: Device file is written at offset = %i, write bytes count = %u\n", (int)*pos, (unsigned int)count );

    if (count > MAX_BUFFER ) {
		count = MAX_BUFFER;
	}    
    if ( copy_from_user(procfs_buffer, buf, count) ) {
		return -EFAULT;
	}

    //printk( KERN_NOTICE "vrfm: received %s\n" , procfs_buffer);

    sendpacket(procfs_buffer,count-1);
    return count;
}



struct mmap_info {
	char *data;
	int reference;
};


int mmap_open(struct inode *inode, struct file *filp)
{
	struct mmap_info *info = NULL;

	if (!mutex_trylock(&mmap_device_mutex)) {
		printk(KERN_WARNING
		       "Another process is accessing the device\n");
		return -EBUSY;
	}

	info = kmalloc(sizeof(struct mmap_info), GFP_KERNEL);
	info->data = (char *)get_zeroed_page(GFP_KERNEL);
	memcpy(info->data, "Hello from kernel this is file: ", 32);
	memcpy(info->data + 32, filp->f_path.dentry->d_name.name,
	       strlen(filp->f_path.dentry->d_name.name));
	/* assign this info struct to the file */
	filp->private_data = info;    
    printk( "mmap open\n");
    return 0;
}

void mmap_open1(struct vm_area_struct *vma)
{
    //printk("mmap1 enter %x\n", vma);
    //printk("mmap1 enter %x\n", vma->vm_private_data);
	struct mmap_info *info = (struct mmap_info *)vma->vm_private_data;
	info->reference++;
}



void mmap_close(struct vm_area_struct *vma)
{
	struct mmap_info *info = (struct mmap_info *)vma->vm_private_data;
	info->reference--;
}


static int mmap_fault(struct vm_area_struct *vma, struct vm_fault *vmf)
{
	struct page *page;
	struct mmap_info *info;

	info = (struct mmap_info *)vma->vm_private_data;
	if (!info->data) {
		printk("No data\n");
		return 0;
	}

	page = virt_to_page(info->data);

	get_page(page);
	vmf->page = page;

	return 0;
}

struct vm_operations_struct mmap_vm_ops = {
	.open = mmap_open1,
	.close = mmap_close,
	.fault = mmap_fault,
};


int memory_map (struct file * f, struct vm_area_struct * vma)
{
    printk("mmap1 enter\n");
    vma->vm_ops = &mmap_vm_ops;
    printk("mmap1 enter\n");
	vma->vm_flags |= VM_DONTEXPAND | VM_DONTDUMP;
    printk("mmap1 enter\n");
	vma->vm_private_data = f->private_data;
    //printk("mmap1 enter %x\n", vma);
	mmap_open1(vma);    
    printk("mmap1 \n");
    return 0;
}


int mmapfop_close(struct inode *inode, struct file *filp)
{
	struct mmap_info *info = filp->private_data;

	free_page((unsigned long)info->data);
	kfree(info);
	filp->private_data = NULL;

	mutex_unlock(&mmap_device_mutex);

	return 0;
}


/*===============================================================================================*/
static struct file_operations vrfm_driver_fops = 
{
    .owner = THIS_MODULE,
	.open = mmap_open,
	.release = mmapfop_close,
    .read = device_file_read,
    .write = complete_write,    
    .mmap = memory_map,
};




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




/*===============================================================================================*/
static int vrfm_driver_init(void)
{
  

   struct device *pDev;
    mutex_init(&mmap_device_mutex);
   net_init();

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

/*===============================================================================================*/
static void vrfm_driver_exit(void)
{
    printk( KERN_NOTICE "vrfm: Exiting\n" );
    net_shutdown();
    device_destroy(pClass, devNo);  // Remove the /dev/DEVICE_NAME
    class_destroy(pClass);  // Remove class /sys/class/DEVICE_NAME
    unregister_chrdev(majorNum, xstr(DEVICE_NAME));  // Unregister the device    
}

/*===============================================================================================*/
module_init(vrfm_driver_init);
module_exit(vrfm_driver_exit);
