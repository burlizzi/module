#include <linux/module.h>     
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include "chdev.h"
#include "net.h"
#include "mmap.h"


MODULE_DESCRIPTION("Virtual Reflective Memory Linux driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Luca Burlizzi");
MODULE_SOFTDEP("e1000e");

static char myBuff[1048];
typedef signed int              RFM2G_INT32;
const char *devname = "rfm2g";


RFM2G_INT32
RFM2gReadProcPage( char *buf, char **start, off_t offset, int len, int *unused,
                  void *data_unused)
{
    static char *me = "RFM2GReadProcPage()";
    RFM2G_INT32 bytesRead = 0;  /* Returned length                        */
    RFM2G_INT32 i;              /* Loop variable                          */
    char device_num[10];        /* Indicates minor number in text message */
    RFM2G_INT32 j;              /* Loop variable                          */

//    WHENDEBUG(RFM2G_DBTRACE) printk(KERN_ERR"%s: Entering %s\n", devname, me);

    bytesRead = sprintf( buf, "\nWelcome to the RFM2G proc page!\n\n" );
    if( bytesRead > PAGE_SIZE-80 ) return( bytesRead ); /* This is enough! */

    bytesRead = sprintf( buf, "COPYRIGHT NOTICE\n" );
    if( bytesRead > PAGE_SIZE-80 ) return( bytesRead ); /* This is enough! */

    bytesRead = sprintf( buf, "Copyright (C) 2002 VMIC\n" );
    if( bytesRead > PAGE_SIZE-80 ) return( bytesRead ); /* This is enough! */

    bytesRead = sprintf( buf, "International Copyright Secured.  All Rights Reserved.\n" );
    if( bytesRead > PAGE_SIZE-80 ) return( bytesRead ); /* This is enough! */

    bytesRead += sprintf( buf+bytesRead, "MODULE_NAME                %s\n",
        devname );
    if( bytesRead > PAGE_SIZE-80 ) return( bytesRead ); /* This is enough! */


    return( bytesRead );

}   /* End of RFM2gReadProcPage() */



static int rfm2gFSshow(struct seq_file *m, void *v)
{
	RFM2gReadProcPage(myBuff, 0, 0, 0, 0,0);
	seq_write(m, myBuff, strlen(myBuff));
	return 0;
}


static int Rfm2gFSOpen(struct inode *inode, struct  file *file)
{
	return single_open(file, rfm2gFSshow, NULL);
}


struct file_operations rfm2gFOS =
{
	.owner = THIS_MODULE,
	.open = Rfm2gFSOpen,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};


static int vrfm_driver_init(void)
{
  printk( KERN_NOTICE "vrfm: Starting\n" );
   if (net_init())
        return -1;
   if (mmap_ops_init())
        return -1;
   if (chdev_init())
        return -1;
     proc_create("rfm2g", 0, NULL, &rfm2gFOS);
  return 0;
}

/*===============================================================================================*/
static void vrfm_driver_exit(void)
{
    printk( KERN_NOTICE "vrfm: Exiting\n" );
    net_shutdown();
    chdev_shutdown();
    mmap_shutdown();
    remove_proc_entry("rfm2g", NULL);

}

/*===============================================================================================*/
module_init(vrfm_driver_init);
module_exit(vrfm_driver_exit);
