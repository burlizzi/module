#include <linux/uaccess.h>  
#include <linux/fs.h>    
#include <linux/device.h>
#include <linux/cdev.h>
#include <linux/delay.h>
#include <asm/uaccess.h>


#include "net.h"
#include "mmap.h"
#include "config.h"

extern int pktsize;
#define MAX_BUFFER 1024

char procfs_buffer[MAX_BUFFER];

const int unit = 0;





int majorNum[MAX_RFM2G_DEVICES];
dev_t devNo[MAX_RFM2G_DEVICES];  // Major and Minor device numbers combined into 32 bits
char* devices[MAX_RFM2G_DEVICES]={0};  // Major and Minor device numbers combined into 32 bits
struct mmap_info* infos[MAX_RFM2G_DEVICES]={0};
struct class* classesArray[MAX_RFM2G_DEVICES];  // class_create will set this
int rfm_instances;

char *rfmdevice = "rfm2g0";
module_param(rfmdevice, charp,S_IRUGO);
MODULE_PARM_DESC(rfmdevice, "RFM device to create default=rfm2g0");



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
    
    int block;
    int offsetinpage;
    struct mmap_info *info = file_ptr->private_data;
    
    //LOG( KERN_NOTICE "vrfm: Device file is read at offset = %i, read bytes count = %u\n", (int)*position, (unsigned int)count );

    if( *position >= size )
        return 0;

    if( count >= PAGE_SIZE )
        count = PAGE_SIZE;

    if( *position + count > size )
        count = size - *position;


            block=*position/PAGE_SIZE;
            
            
            offsetinpage=*position % PAGE_SIZE;
            
            if( unlikely(offsetinpage+count>PAGE_SIZE))//we crossed the boundaries
            {
                
                if (info->data[block])
                {
                    //LOG( KERN_NOTICE "vrfm: block=%d, offset = %u size=%ld\n",block,offsetinpage,PAGE_SIZE-offsetinpage);
                    
                    if(copy_to_user(user_buffer, &info->data[block][offsetinpage],PAGE_SIZE-offsetinpage))
                        LOG( KERN_NOTICE "vrfm: block=%d, offset = %u size=%ld\n",block,offsetinpage,PAGE_SIZE-offsetinpage);
                }
                else
                {
                    if(clear_user(user_buffer,PAGE_SIZE-offsetinpage))
                        LOG( KERN_NOTICE "vrfm: block=%d, offset = %u size=%ld\n",block,offsetinpage,PAGE_SIZE-offsetinpage);
                }
                    

                if (info->data[block+1])
                {
                    //LOG( KERN_NOTICE "vrfm: block1=%d, offset = %u size=%ld\n",block+1,0,count+offsetinpage-PAGE_SIZE);
                    if(copy_to_user(user_buffer+PAGE_SIZE-offsetinpage, &info->data[block+1][0],count+offsetinpage-PAGE_SIZE))
                        LOG( KERN_NOTICE "vrfm: block1=%d, offset = %u size=%ld\n",block+1,0,count+offsetinpage-PAGE_SIZE);
                }
                else 
                {
                    if (clear_user(user_buffer+PAGE_SIZE-offsetinpage,count+offsetinpage-PAGE_SIZE))
                        LOG( KERN_NOTICE "clear_user failed off=%d count=%ld\n",offsetinpage,count);
                }
                

            }
            else
            {
                if (info->data[block])
                {
                    //LOG( KERN_NOTICE "vrfm: block=%d, offset = %u size=%ld\n",block,offsetinpage,count);
                    if(copy_to_user(user_buffer, &info->data[block][offsetinpage],count))
                        LOG( KERN_NOTICE "vrfm: block=%d, offset = %u size=%ld\n",block,offsetinpage,count);
                }
                else
                {
                    if(clear_user(user_buffer,count))
                        LOG( KERN_NOTICE "clear_user failed count=%ld\n",count);
                } 

                //
            }
                


    //if( copy_to_user(user_buffer, g_s_Hello_World_string + *position, count) != 0 )
    //if( copy_to_user(user_buffer, info->data + *position, count) != 0 )
      //  return -EFAULT;

    *position += count;
    
    return count;
}

ssize_t complete_write(struct file *filp,const char __user *buf,size_t count,loff_t *pos)
{
    int retval;
    struct mmap_info *info = filp->private_data;

    LOG( KERN_NOTICE "vrfm: Device file is written at offset = %i, write bytes count = %u\n", (int)*pos, (unsigned int)count );

    if (count > MAX_BUFFER ) {
		count = MAX_BUFFER;
	}    
    if ( copy_from_user(procfs_buffer, buf, count) ) {
		return -EFAULT;
	}
    
    memcpy(info->data,procfs_buffer,count-1);
    
    //memcpy(info->data, "Hello from kernel this is file: ", 32);

    //LOG( KERN_NOTICE "vrfm: received %s\n" , procfs_buffer);
    while ((retval=sendpacket(info,(*pos),count,VRFM_MEM_SEND))==1)
                    schedule();
    if (retval==-1)
        return 0;
    return count;
}


void allocatedata(struct mmap_info * info, size_t offset, size_t length)
{

    size_t block=offset/PAGE_SIZE;
    size_t endblock=(offset+length)/PAGE_SIZE+1;
    for ( ; block < endblock; block++)
    {
        
        if (!info->data) {
            LOG("No data\n");
            return ;
        }

        if (offset>=size<<PAGES_ORDER)
        {
            LOG("mmap_fault overflow\n");
            return ;
        }

        if (!info->data[block])
        {
            LOG("allocate page chunk:%lu \n",block);
#if PAGES_ORDER
            info->data[block]=(char *)__get_free_pages(GFP_KERNEL, PAGES_ORDER);
            memset(info->data[block],0,PAGE_SIZE<<PAGES_ORDER);
#else
            info->data[block]=(char *)get_zeroed_page(GFP_KERNEL);
#endif            
        }
        else 
        {
            //LOG("recycle page chunk:%d offset:%lx \n",block,offset);
        }
    }

            
}



long rfm2g_ioctl(struct file *filp, unsigned int cmd, unsigned long arg )
{
        static char *me = "rfm2g_ioctl()";
    struct mmap_info *info = filp->private_data;
    	/* Valid Call? */

	if (_IOC_TYPE(cmd) != RFM2G_MAGIC)
	{
//        WHENDEBUG(RFM2G_DBERROR)
        {
            LOG(KERN_ERR"%s: Exiting : invalid ioctl magic num = %d expected %d cmd=%x\n",
                me, _IOC_TYPE(cmd), RFM2G_MAGIC,cmd);
        }
		return -ENOTTY;
	}

            //LOG(KERN_ERR"%s: Exiting : invalid ioctl magic num = %d expected %d\n",me, _IOC_TYPE(cmd), RFM2G_MAGIC);

    switch( cmd )
    {
        case IOCTL_RFM2G_SET_SPECIAL_MMAP_OFFSET:
        {
            return( 0 );
        }
        case IOCTL_RFM2G_ATOMIC_PEEK:
        {
            //char *myself = "ATOMIC_PEEK";
            RFM2GATOMIC  Data;     /* Info necessary to do the access         */
    	    //RFM2G_ADDR rfm2gAddr;  /* RFM address to read data              */
            //RFM2G_ADDR base;     /* Base address of RFM I/O or memory space */
            RFM2G_UINT32 maxsize;  /* Max size of RFM I/O or memory space     */
            
            int offsetinpage;
            int block;
            char* dest;


            if( copy_from_user( (void *)&Data, (void *)arg,
                sizeof(RFM2GATOMIC) ) > 0 )
            {
                WHENDEBUG(RFM2G_DBERROR)
                {
                    LOG(KERN_ERR"%s%d: Exiting %s: copy_from_user() failed\n",
                        info->name, unit, me );
                }

                return( -EFAULT );
            }

            /* Validate the data width */
            switch( Data.width )
            {
                case RFM2G_BYTE: break;
                case RFM2G_WORD: break;
                case RFM2G_LONG: break;
                case RFM2G_LONGLONG: /* Not supported, fall thru to default */
                default:
                {
                    WHENDEBUG(RFM2G_DBERROR)
                    {
                        LOG(KERN_ERR"%s%d: Exiting %s: Invalid data width %d\n",
                            info->name, unit, me, Data.width );
                    }

                    return( -EINVAL );
                }
            }

            //base    =  (RFM2G_ADDR)cfg->pBaseAddress;
            maxsize = size;
            

            block=Data.offset/PAGE_SIZE;
            
            allocatedata(info,Data.offset,Data.width );
            offsetinpage=Data.offset % PAGE_SIZE;
            dest=(char*)&Data.data;
            if( unlikely(offsetinpage+Data.width>PAGE_SIZE))//we crossed the boundaries
            {
                memcpy(dest, &info->data[block][offsetinpage],PAGE_SIZE-offsetinpage);
                memcpy(dest+PAGE_SIZE-offsetinpage, &info->data[block+1][0],Data.width+offsetinpage-PAGE_SIZE);

            }
            else
                memcpy(dest, &info->data[block][offsetinpage],Data.width);
           	


            /*if( rfm2g_peek( cfg, rfm2gAddr, &(Data.data), Data.width ) != 0 )
            {
                WHENDEBUG(RFM2G_DBERROR)
                {
                    LOG(KERN_ERR"%s%d: Exiting %s: rfm2g_peek() failed\n",
                        devname, unit, me );
                }

                return( -EINVAL );
            }*/

            /* Copy the data back to the user */
            if( copy_to_user( (void *)arg, (void *)&Data,
                sizeof(RFM2GATOMIC) ) > 0 )
            {
                WHENDEBUG(RFM2G_DBERROR)
                {
                    LOG(KERN_ERR"%s%d: Exiting %s: copy_to_user() failed\n",
                        info->name, unit, me );
                }

                return( -EFAULT );
            }

            return( 0 );
        }

        case IOCTL_RFM2G_WRITE:
        {
            
			RFM2GTRANSFER rfm2gTransfer;
            size_t start;
            int len;
            int block;
            size_t startOfInput=0;
            struct mmap_info *info = filp->private_data;

            if( copy_from_user( (void *)&rfm2gTransfer, (void *)arg,
                sizeof(RFM2GTRANSFER) ) > 0 )
            {
                LOG(KERN_ERR": Exiting %s: copy_from_user() failed \n",me );
                return( -EFAULT );
            }

            
            block=rfm2gTransfer.Offset/PAGE_SIZE;
            
            allocatedata(info,rfm2gTransfer.Offset,rfm2gTransfer.Length);
            

            

            start=rfm2gTransfer.Offset;
            len=rfm2gTransfer.Length;

            for(;block<=((rfm2gTransfer.Offset+rfm2gTransfer.Length-1)/PAGE_SIZE);block++)
            {
                //size_t i;
                char* thisblock=info->data[block];
                //size_t end=start+len;
                size_t startInThisBlock=start % PAGE_SIZE;
                size_t len2endofpage=PAGE_SIZE-startInThisBlock;
                if(len2endofpage>len)
                    len2endofpage=len;
                //LOG("block %d start %d end %d len=%d len2endofpage %d\n",block,startInThisBlock,end,len,len2endofpage);

                
                if (!thisblock)
                {
                    LOG(KERN_ERR": Exiting %s: unallocated block %d \n",me,block );
                    return( -EFAULT );
                }
                /*for (i = 0; i < rfm2gTransfer.Length; i++)
                {
                    LOG("%x ",((char*)(rfm2gTransfer.Buffer))[i]);
                }
                LOG("\n");*/
                //LOG("thisblock=%d[%d] startOfInput=%d len=%d\n",block,startInThisBlock,startOfInput,len2endofpage);
                if (copy_from_user(&thisblock[startInThisBlock],((char*)rfm2gTransfer.Buffer)+startOfInput,len2endofpage)!=0)
                    printk(KERN_ERR"%s:%d Exiting : copy_from_user() failed\n",
                        __FILE__, __LINE__ );
                start=(block+1)*PAGE_SIZE; // beginning of next page
                len-=len2endofpage;
                startOfInput+=len2endofpage;
            }

            start=rfm2gTransfer.Offset;
            for (len=rfm2gTransfer.Length;len>0;len-=CHUNK )
            {
                while (sendpacket(info, start,len>CHUNK?CHUNK:len,VRFM_MEM_SEND)==1);
                start+=CHUNK;
            }
         return( 0 );
        }
        case IOCTL_RFM2G_READ:
        {
			RFM2GTRANSFER rfm2gTransfer;
            loff_t position;

            if( copy_from_user( (void *)&rfm2gTransfer, (void *)arg,
                sizeof(RFM2GTRANSFER) ) > 0 )
            {
                LOG(KERN_ERR": Exiting %s: copy_from_user() failed \n",me );
                return( -EFAULT );
            }

            position=rfm2gTransfer.Offset;
            device_file_read(filp,rfm2gTransfer.Buffer,rfm2gTransfer.Length,&position);
            return( 0 );
        }
        case IOCTL_RFM2G_READ_REG:
        {
            RFM2GLINUXREGINFO rfm2gLinuxRegInfo;
            static char data[]={1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21};
			void* pAddr = NULL;

            LOG(KERN_ERR": IOCTL_RFM2G_READ_REG\n" );
   			

            if( copy_from_user( (void *)&rfm2gLinuxRegInfo, (void *)arg,
                sizeof(RFM2GLINUXREGINFO) ) > 0 )
            {
                WHENDEBUG(RFM2G_DBERROR)
                {
                    LOG(KERN_ERR": Exiting %s: copy_from_user() failed\n", me );
                }
                return( -EFAULT );
            }
            LOG(KERN_ERR": rfm2gLinuxRegInfo.regset=%d\n",rfm2gLinuxRegInfo.regset );

            switch(rfm2gLinuxRegInfo.regset)
            {
                case RFM2GCFGREGIO:
                    return ( -EINVAL );
                    break;

                case RFM2GCFGREGMEM:
                    LOG(KERN_ERR": Exiting %s: good\n", me );
                    pAddr = (void*) data;
                    break;

                case RFM2GCTRLREGMEM:

                    return ( -EINVAL );
                    break;

                case RFM2GMEM:
                    return ( -EINVAL );
                    break;

                case RFM2GRESERVED0REG:
                    return ( -EINVAL );
                    break;

                case RFM2GRESERVED1REG:
                    return ( -EINVAL );
                    break;

                default:
                    return ( -EINVAL );
            }

            switch( rfm2gLinuxRegInfo.Width )
            {
                case RFM2G_BYTE:
                    return ( -EINVAL );
                    //rfm2gLinuxRegInfo.Value = (RFM2G_ADDR) readb( (char *)(pAddr) );
                    break;
                case RFM2G_WORD:
                    return ( -EINVAL );
                    //rfm2gLinuxRegInfo.Value = (RFM2G_ADDR) readw( (char *)(pAddr) );
                    break;
                case RFM2G_LONG:
                    rfm2gLinuxRegInfo.Value = RFMOR_LAS1RR_RANGE_2M;
                    break;

                case RFM2G_LONGLONG: /* Not supported */
                default:
                    return ( -EINVAL );
            }
            /* Copy the data back to the user */
            if( copy_to_user( (void *)arg, (void *)&rfm2gLinuxRegInfo,
                sizeof(RFM2GLINUXREGINFO) ) > 0 )
            {
                WHENDEBUG(RFM2G_DBERROR)
                {
                    LOG(KERN_ERR": Exiting %s: copy_to_user() failed\n",
                            me );
                }

                return( -EFAULT );
            }



         return( 0 );
        }
        case IOCTL_RFM2G_GET_CONFIG:
        {
			RFM2GCONFIG Config;
            char *myself = "GET_CONFIG";

            WHENDEBUG(RFM2G_DBIOCTL)
            {
                LOG(KERN_ERR "%d: %s cmd = %s\n",0, me, myself);
            }

			/* Copy the common stuff over */
			Config.NodeId              = 1;
			Config.BoardId             = 1;
			Config.Unit                = 1;
			Config.PlxRevision         = 1;
			Config.MemorySize          = 64*1024*1024;
			d_path (&filp->f_path,Config.Device,sizeof(Config.Device));
			strcpy(Config.Name, RFM2G_PRODUCT_STRING);
			strcpy(Config.DriverVersion, RFM2G_PRODUCT_VERSION);
			Config.BoardRevision       = 0x8d;
			Config.RevisionId          = 1;
            Config.BuildId             = 1;
            memset(&Config.PciConfig, 0, sizeof(RFM2GPCICONFIG));

			/* Read LCSR1 */
			Config.Lcsr1 = 0;// (RFM2G_UINT32) readl( (char *)(( (RFM2G_ADDR) cfg->pCsRegisters + rfm2g_lcsr )));

            /* Copy the data back to the user */
            if( copy_to_user( (void *)arg, (void *)&Config,
                sizeof(RFM2GCONFIG) ) > 0 )
            {
                WHENDEBUG(RFM2G_DBERROR)
                {
                    LOG(KERN_ERR "%d: Exiting %s: copy_to_user() failed\n",
                        0, me );
                }

                return( -EFAULT );
            }
        }
        return( 0 );
        break;
        case IOCTL_RFM2G_WAIT_FOR_EVENT:
        {
            RFM2GEVENTINFO info;
            if( copy_from_user( (void *)&info, (void *)arg,
                sizeof(RFM2GEVENTINFO) ) > 0 )
            {
                //WHENDEBUG(RFM2G_DBERROR)
                {
                    LOG(KERN_ERR": Exiting %s: copy_from_user() failed\n", me );
                }
                return( -EFAULT );
            }
            LOG("waiting for %u\n",info.Timeout);
            switch (info.Timeout)
            {
            case RFM2G_NO_WAIT:
                break;
            
            case RFM2G_INFINITE_TIMEOUT:
                schedule();
                break;
            default:
                info.Timeout = schedule_timeout( info.Timeout );
                break;
            } 
            LOG("enough waiting %d\n",info.Timeout);

        }
        
        return( 0 );
    }

    return( -EFAULT );

}


static struct file_operations vrfm_driver_fops = 
{
    .owner = THIS_MODULE,
	.open = mmap_open,
	.release = mmapfop_close,
    .read = device_file_read,
    .write = complete_write,    
    .mmap = memory_map,
    .unlocked_ioctl = rfm2g_ioctl,

};

int chdev_init(void)
{
    
    struct device *pDev;
    char* device=NULL;
    static char save[128];
    char* point;
    struct class* pClass=classesArray[0];

    // Create /dev/DEVICE_NAME for this char dev
    printk("a %s\n",rfmdevice);
    strcpy(save,rfmdevice);
    printk("b %s \n",save);
    point=&save[0];
    rfm_instances=0;
    while ((device = strsep(&point, ","))) 
    {
        struct mmap_info* info=NULL;
        devices[rfm_instances]=device;
        LOG("device %s\n",device);
        infos[rfm_instances]=info=kmalloc(sizeof(struct mmap_info), GFP_KERNEL);
       	info->data = kmalloc(blocks*sizeof(char*), GFP_KERNEL);
	    memset(info->data,0,blocks*sizeof(char*));
    	info->reference = 0;
        info->index = rfm_instances;



        pClass=classesArray[rfm_instances];
        // Register character device
        majorNum[rfm_instances] = register_chrdev(0, device, &vrfm_driver_fops);
        if (majorNum[rfm_instances] < 0) {
            LOG(KERN_ALERT "Could not register device: %d\n", majorNum[rfm_instances]);
            return majorNum[rfm_instances];
        }
        devNo[rfm_instances] = MKDEV(majorNum[rfm_instances], 0);  // Create a dev_t, 32 bit version of numbers

        // Create /sys/class/DEVICE_NAME in preparation of creating /dev/DEVICE_NAME
        pClass = class_create(THIS_MODULE, device);

        pClass->devnode = vrfm_devnode;


        if (IS_ERR(pClass)) {
            LOG(KERN_WARNING "\ncan't create class");
            unregister_chrdev_region(devNo[rfm_instances], 1);
            return -1;
        }

        printk("c %s\n",device);
        if (IS_ERR(pDev = device_create(pClass, NULL, devNo[rfm_instances], NULL, device))) {
            LOG(KERN_WARNING xstr(MODULE_NAME)".ko can't create device /dev/%s\n",device);
            class_destroy(pClass);
            unregister_chrdev_region(devNo[rfm_instances], 1);
            return -1;
        }
        printk("VRFM device created on /dev/%s\n",device);
        classesArray[rfm_instances]=pClass;
        rfm_instances++;

    }
    printk("d\n");

    return 0;
}

void chdev_shutdown(void)
{
   char* device=NULL;
   int i=0;
   struct class* pClass=classesArray[0];
   while ((device = strsep(&rfmdevice, ","))) 
   {
    pClass=classesArray[i];
    device_destroy(pClass, devNo[i]);  // Remove the /dev/DEVICE_NAME
    class_destroy(pClass);  // Remove class /sys/class/DEVICE_NAME
    unregister_chrdev(majorNum[i], device);  // Unregister the device    
    printk("VRFM device deleted on /dev/%s\n",device);
    i++;
   }
}



    