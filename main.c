#include <linux/init.h>       /* module_init, module_exit */
#include <linux/module.h>     /* version info, MODULE_LICENSE, MODULE_AUTHOR, printk() */
#include <linux/uaccess.h>  
#include <linux/fs.h>    
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/mutex.h>
#include <linux/kernel.h>
#include <linux/slab.h>



#include "chdev.h"
#include "net.h"
#include "mmap.h"


MODULE_DESCRIPTION("Virtual Reflective Memory Linux driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Luca Burlizzi");
MODULE_SOFTDEP("e1000e");


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

static int vrfm_driver_init(void)
{
  printk( KERN_NOTICE "vrfm: Starting\n" );
   if (net_init())
        return -1;
   if (mmap_ops_init())
        return -1;
   if (chdev_init())
        return -1;

  return 0;
}

/*===============================================================================================*/
static void vrfm_driver_exit(void)
{
    printk( KERN_NOTICE "vrfm: Exiting\n" );
    net_shutdown();
    chdev_shutdown();
}

/*===============================================================================================*/
module_init(vrfm_driver_init);
module_exit(vrfm_driver_exit);
