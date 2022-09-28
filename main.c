#include <linux/module.h>     


#include "chdev.h"
#include "net.h"
#include "mmap.h"

#include <linux/proc_fs.h>
#include <linux/version.h>
#include <linux/seq_file.h>
#include "rfm2g_types.h"


MODULE_DESCRIPTION("Virtual Reflective Memory Linux driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Luca Burlizzi");
MODULE_SOFTDEP("e1000e");


RFM2G_INT32
RFM2gReadProcPage( char *buf, char **start, off_t offset, int len, int *unused,
                  void *data_unused)
{
    //static char *me = "RFM2GReadProcPage()";
    RFM2G_INT32 bytesRead = 0;  /* Returned length                        */
    //RFM2G_INT32 i;              /* Loop variable                          */
    //char device_num[10];        /* Indicates minor number in text message */
    //RFM2G_INT32 j;              /* Loop variable                          */

    //WHENDEBUG(RFM2G_DBTRACE) LOG(KERN_ERR"%s: Entering %s\n", devname, me);

    bytesRead = sprintf( buf, "\nWelcome to the RFM2G proc page!\n\n" );
    if( bytesRead > PAGE_SIZE-80 ) return( bytesRead ); /* This is enough! */

    bytesRead = sprintf( buf, "COPYRIGHT NOTICE\n" );
    if( bytesRead > PAGE_SIZE-80 ) return( bytesRead ); /* This is enough! */

    bytesRead = sprintf( buf, "Copyright (C) 2002 VMIC\n" );
    if( bytesRead > PAGE_SIZE-80 ) return( bytesRead ); /* This is enough! */

    bytesRead = sprintf( buf, "International Copyright Secured.  All Rights Reserved.\n" );
    if( bytesRead > PAGE_SIZE-80 ) return( bytesRead ); /* This is enough! */

    //bytesRead += sprintf( buf+bytesRead, "MODULE_NAME                %s\n",
     //   devname );
    if( bytesRead > PAGE_SIZE-80 ) return( bytesRead ); /* This is enough! */

    bytesRead += sprintf( buf+bytesRead, "RFM2G_DEVICE_COUNT         %d\n\n",
        1 );
    if( bytesRead > PAGE_SIZE-80 ) return( bytesRead ); /* This is enough! */

    /* Show the contents of the RFM2GDEVICEINFO structures 
    for( i=0; i<rfm2g_device_count; i++ )
    {
        sprintf( device_num, "DEVICE_%d", i );

        bytesRead += sprintf( buf+bytesRead, "%s_BUS               %d\n",
            device_num, (int) rfm2gDeviceInfo[i].Config.PCI.bus );

        bytesRead += sprintf( buf+bytesRead, "%s_FUNCTION          %d\n",
            device_num, (int) rfm2gDeviceInfo[i].Config.PCI.function );

        bytesRead += sprintf( buf+bytesRead, "%s_DEVFN             0x%08X\n",
            device_num, (int) rfm2gDeviceInfo[i].Config.PCI.devfn );

        bytesRead += sprintf( buf+bytesRead, "%s_TYPE              0x%04X\n",
            device_num, (int) rfm2gDeviceInfo[i].Config.PCI.type );

        bytesRead += sprintf( buf+bytesRead, "%s_REVISION          %d\n",
            device_num, (int) rfm2gDeviceInfo[i].Config.PCI.revision );

        bytesRead += sprintf( buf+bytesRead, "%s_OR_REGISTER_BASE  0x%08X\n",
            device_num, (int) rfm2gDeviceInfo[i].Config.PCI.rfm2gOrBase );

        bytesRead += sprintf( buf+bytesRead, "%s_OR_REGISTER_SIZE  %d\n",
            device_num,
            (int) rfm2gDeviceInfo[i].Config.PCI.rfm2gOrWindowSize );

        bytesRead += sprintf( buf+bytesRead, "%s_CS_REGISTER_BASE  0x%08X\n",
            device_num, (int) rfm2gDeviceInfo[i].Config.PCI.rfm2gCsBase );

        bytesRead += sprintf( buf+bytesRead, "%s_CS_REGISTER_SIZE  %d\n",
            device_num,
            (int) rfm2gDeviceInfo[i].Config.PCI.rfm2gCsWindowSize );

        bytesRead += sprintf( buf+bytesRead, "%s_MEMORY_BASE       0x%08X\n",
            device_num, (int) rfm2gDeviceInfo[i].Config.PCI.rfm2gBase );

        bytesRead += sprintf( buf+bytesRead, "%s_MEMORY_SIZE       %d\n",
            device_num, (int) rfm2gDeviceInfo[i].Config.MemorySize );

        bytesRead += sprintf( buf+bytesRead, "%s_INTERRUPT         %d\n",
            device_num,
            (int) rfm2gDeviceInfo[i].Config.PCI.interruptNumber );

        bytesRead += sprintf( buf+bytesRead, "rfm2gDebugFlags            0x%08X\n",
            rfm2gDebugFlags);

        bytesRead += sprintf( buf+bytesRead, "RFM Flags                  0x%08X\n",
            rfm2gDeviceInfo[i].Flags);

        bytesRead += sprintf( buf+bytesRead, "Capability Flags           0x%08X\n",
            rfm2gDeviceInfo[i].Config.Capabilities);

        bytesRead += sprintf( buf+bytesRead, "Instance counter           %d\n",
            rfm2gDeviceInfo[i].Instance);

        bytesRead += sprintf( buf+bytesRead, "Queue Flags = " );

        for(j=0; j<RFM2GEVENT_LAST; j++)
        {
            bytesRead += sprintf( buf+bytesRead, "0x%02X  ",
                rfm2gDeviceInfo[i].EventQueue[j].req_header.reqh_flags);
        }

        bytesRead += sprintf( buf+bytesRead, "\n" );

        if( bytesRead > PAGE_SIZE-80 ) return( bytesRead );
    }
*/
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

static int vrfm_driver_init(void)
{
  printk( KERN_NOTICE "vrfm: Starting\n" );
  proc_create("rfm2g",0666,NULL,&rfm2gFOS);

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
    remove_proc_entry("rfm2g", NULL);

    printk( KERN_NOTICE "vrfm: terminating\n" );
    net_shutdown();
    chdev_shutdown();
    mmap_shutdown();
    printk( KERN_NOTICE "vrfm: done\n" );
}

/*===============================================================================================*/
module_init(vrfm_driver_init);
module_exit(vrfm_driver_exit);
