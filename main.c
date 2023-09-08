#include <linux/module.h>     


#include "chdev.h"
#include "net.h"
#include "mmap.h"
#include "crypt.h"
#include "rfm2g_types.h"

#include <linux/proc_fs.h>
#include <linux/version.h>
#include <linux/seq_file.h>
#include <linux/netdevice.h>



MODULE_DESCRIPTION("Virtual Reflective Memory Linux driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Luca Burlizzi");
MODULE_SOFTDEP("e1000e");
#if LINUX_VERSION_CODE > KERNEL_VERSION(5,2,0)
MODULE_IMPORT_NS(CRYPTO_INTERNAL);
#endif

#define VALUE(string) #string
#define TO_LITERAL(string) VALUE(string)

extern int rfm_instances;
extern char* devices[MAX_RFM2G_DEVICES];
extern struct net_device* dev_eth[MAX_RFM2G_DEVICES];
RFM2G_INT32
RFM2gReadProcPage( char *buf, char **start, off_t offset, int len, int *unused,
                  void *data_unused)
{
    //static char *me = "RFM2GReadProcPage()";
    RFM2G_INT32 bytesRead = 0;  /* Returned length                        */
    RFM2G_INT32 i;              /* Loop variable                          */
    //char device_num[10];        /* Indicates minor number in text message */
    //RFM2G_INT32 j;              /* Loop variable                          */

    //WHENDEBUG(RFM2G_DBTRACE) LOG(KERN_ERR"%s: Entering %s\n", devname, me);

    bytesRead += sprintf( buf+bytesRead, "\nWelcome to the RFM2G proc page!\n\n" );
    if( bytesRead > PAGE_SIZE-80 ) return( bytesRead ); /* This is enough! */

    // bytesRead += sprintf( buf+bytesRead, "COPYRIGHT NOTICE\n" );
    // if( bytesRead > PAGE_SIZE-80 ) return( bytesRead ); /* This is enough! */

    // bytesRead += sprintf( buf+bytesRead, "Copyright (C) 2002 VMIC\n" );
    // if( bytesRead > PAGE_SIZE-80 ) return( bytesRead ); /* This is enough! */

    // bytesRead += sprintf( buf+bytesRead, "International Copyright Secured.  All Rights Reserved.\n" );
    // if( bytesRead > PAGE_SIZE-80 ) return( bytesRead ); /* This is enough! */

    bytesRead += sprintf( buf+bytesRead, "MODULE_NAME                " TO_LITERAL(MODULE_NAME)"\n");
    if( bytesRead > PAGE_SIZE-80 ) return( bytesRead ); /* This is enough! */

    bytesRead += sprintf( buf+bytesRead, "RFM2G_DEVICE_COUNT         %d\n\n",
        rfm_instances );
    if( bytesRead > PAGE_SIZE-80 ) return( bytesRead ); /* This is enough! */

    // Show the contents of the RFM2GDEVICEINFO structures 
    for( i=0; i<rfm_instances; i++ )
    {
        //sprintf( device_num, "DEVICE_%d", i );

        bytesRead += sprintf( buf+bytesRead, "%s_BUS               %s\n",
            devices[i], dev_eth[i]->name );


        bytesRead += sprintf( buf+bytesRead, "\n" );

        if( bytesRead > PAGE_SIZE-80 ) return( bytesRead );
    }
/**/
    //WHENDEBUG(RFM2G_DBTRACE) LOG(KERN_ERR"%s: Exiting %s\n", devname, me);

    return( bytesRead );

}   /* End of RFM2gReadProcPage() */



static char myBuff[1048];
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



#if LINUX_VERSION_CODE >= KERNEL_VERSION(5,6,0)

struct proc_ops rfm2gFOS =
{
	.proc_open = Rfm2gFSOpen,
	.proc_read = seq_read,
	.proc_lseek = seq_lseek,
	.proc_release = single_release,
};

#else

struct file_operations rfm2gFOS =
{
     
	.owner = THIS_MODULE,
	.open = Rfm2gFSOpen,
	.read = seq_read,
	.llseek = seq_lseek,
	.release = single_release,
};

#endif

int stage=0;
static void vrfm_driver_exit(void)
{
    if (stage>=5)
        remove_proc_entry("rfm2g", NULL);

    printk( KERN_NOTICE "vrfm: terminating\n" );
    if (stage>=4)
        net_shutdown();
    printk( KERN_NOTICE "vrfm: net shutdown\n" );
    if (stage>=3)
        chdev_shutdown();
    printk( KERN_NOTICE "vrfm: chdev_shutdown\n" );
    if (stage>=2)
        mmap_shutdown();
    printk( KERN_NOTICE "vrfm: mmap_shutdown\n" );
    crypt_done();
    printk( KERN_NOTICE "vrfm: done\n" );
}


static int vrfm_driver_init(void)
{
  printk( KERN_NOTICE "vrfm: Starting\n" );
  crypt_init();
  stage=1;
  if (mmap_ops_init())
        goto cleanup;
  stage=2;
  if (chdev_init())
        goto cleanup;
  stage=3;
  if (net_init())
        goto cleanup;
  stage=4;
  proc_create("rfm2g",0666,NULL,&rfm2gFOS);
  stage=5;
  return 0;
cleanup:
    printk( KERN_ERR "vrfm: error during initialization\n" );
    vrfm_driver_exit();
    return -1;
}

/*===============================================================================================*/

/*===============================================================================================*/
module_init(vrfm_driver_init);
module_exit(vrfm_driver_exit);
