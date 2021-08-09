#include <linux/module.h>     


#include "chdev.h"
#include "net.h"
#include "mmap.h"


MODULE_DESCRIPTION("Virtual Reflective Memory Linux driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Luca Burlizzi");
MODULE_SOFTDEP("e1000e");



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
