#include <asm/pgtable_types.h>
#include <linux/writeback.h>
#include <linux/mm.h>
#include <linux/pagemap.h>
#include <linux/mm_types.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include "mmap.h"
#include "net.h"
#include "config.h"
#include "protocol.h"
#include <linux/moduleparam.h>
#include <linux/dma-mapping.h>
#include <asm/tlbflush.h>


#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/proc_fs.h>
#include <linux/pci.h>
#include <linux/types.h>
#include <linux/fs.h>
#include <linux/sched.h>
#include <asm/io.h>
#include <asm/errno.h>
#include <asm/uaccess.h>
#include <linux/zconf.h>
#include <linux/spinlock.h>
#include <linux/interrupt.h>
#include <linux/seq_file.h>
#include <linux/kthread.h>  // for threads
#include <linux/delay.h>

//static DEFINE_MUTEX(mmap_device_mutex);
//struct mmap_info *info = NULL;



struct dmutex
{
	struct mutex m;
	const char* prev;
};




void dmutex_init(struct dmutex* m){
	mutex_init(&m->m);
	m->prev="";
}


#define  dmutex_lock(X) \
	{LOG(#X" LOCKING %s<->%s",(X)->prev,__func__);if(mutex_lock_interruptible(X.m))LOG(#X" INTERRUPT %s<->%s",(X)->prev,__func__);LOG(#X" LOCK %s<->%s",(X)->prev,__func__); (X)->prev=__func__;}
//	{LOG(#X"LOCKING %s<->%s",(X)->prev,__func__);mutex_lock(X.m);LOG(#X" LOCK %s<->%s",(X)->prev,__func__); (X)->prev=__func__;}
//	


#define  dmutex_unlock(X) \
	{mutex_unlock(X.m);{LOG(#X" UNLOCK %s<|>%s",(X)->prev,__func__);(X)->prev=__func__+1;}}


wait_queue_head_t wait_queue_transmit;

//struct dmutex mem_mutex; 
extern int rfm_instances;
extern char* devices[MAX_RFM2G_DEVICES];  // Major and Minor device numbers combined into 32 bits
extern struct mmap_info* infos[MAX_RFM2G_DEVICES];


int size = MAP_SIZE; // default

bool debug = false;
module_param(debug, bool,S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);


static int fb_deferred_io_work(void* info);




int blocks=MAP_SIZE/PAGE_SIZE/PAGES_PER_BLOCK;

//char** blocks_array=NULL;
//short* dirt_pages;
//short* current_dirt;

static int notify_param(const char *val, const struct kernel_param *kp)
{
	static int order=0;
	int res;
	int oldorder=order;
	char* newpages=0;

	LOG("notify_param\n");
	res = param_set_int(val, kp); // Use helper for write variable
	if(res==0) {
			LOG(KERN_INFO "New value of size = %d\n", size);
			order=get_order(size);

			if ((PAGE_SIZE<<(order-1))!=size)
			{
				size=(PAGE_SIZE<<(order));
				blocks=size/PAGE_SIZE/PAGES_PER_BLOCK;
				LOG(KERN_WARNING "size is not a power of 2, new value of size = %d\n", size);
			}
			if (oldorder!=order)
			{
				LOG("reallocating...\n");
				newpages= (char *)__get_free_pages(GFP_KERNEL, order);
				if (!newpages)
				{
					LOG("mmap_ops_init: cannot allocate memory\n");
					return 1;
				}
				memset(newpages, 0, size);
				//memcpy(newpages,pages,min((PAGE_SIZE<<order),(PAGE_SIZE<<oldorder)));
				//free_pages((unsigned long)pages,oldorder);
			}
			else 
			{
				LOG("no need to reallocate\n");

			}

			return 0;
	}
	else
			LOG(KERN_WARNING "cannot parse %s\n",val);
	return -1;
}

static int size_op_read_handler(char *buffer, const struct kernel_param *kp)
{
	LOG("size_op_read_handler %d\n",size);
	snprintf(buffer,10,"%d",size);

	return strlen(buffer);
}

static const struct kernel_param_ops size_op_ops = {
	.set = &notify_param,
	.get = &size_op_read_handler
};

module_param_cb(size, &size_op_ops, &size, S_IRUGO|S_IWUSR);








static int fb_deferred_io_set_page_dirty(struct page *page)
{

	//dmutex_lock(&mem_mutex);
	int32_t *index;
	struct mmap_info **i;

	LOG("fb_deferred_io_set_page_dirty :%p",page);
	if (!PageDirty(page))
		SetPageDirty(page);

	for ( i = infos; *i ; i++)
	{
		for ( index = (*i)->dirt_pages; *index>=0  ; index++)
		{
			if(page==virt_to_page((*i)->data[*index]))
			{
				LOG("found: %s",(*i)->name);
				(*i)->dirty = true;
				break;
			}
		}
	}
	

	wake_up(&wait_queue_transmit);
	//dmutex_unlock(&etx_mutex);
	//dmutex_unlock(&mem_mutex);
	
	LOG("done");
	return 0;
}


int write_begin(struct file *file, struct address_space *mapping,
				   loff_t pos, unsigned len, unsigned flags,
				struct page **page, void **fsdata)
{
	LOG("write_begin :%p pos:%lld len:%d ",page,pos,len);
	return 0;
}

int writepage(struct page *page, struct writeback_control *wbc)
{
	LOG("writepage :%p ",page);
	return 0;
}

void invalidatepage (struct page * page, unsigned int pos, unsigned int len)
{
	LOG("invalidatepage :%p pos:%u len:%d ",page,pos,len);
}


static const struct address_space_operations fb_deferred_io_aops = {
	.set_page_dirty = fb_deferred_io_set_page_dirty,
};


int mmap_open(struct inode *inode, struct file *filp)
{
	size_t i;
	struct mmap_info* info=NULL;
	for (i = 0; i < rfm_instances; i++)
	{
		LOG("checking %s / %s\n",devices[i],filp->f_path.dentry->d_iname);
		if (strstr(devices[i],filp->f_path.dentry->d_iname))
		{
			info=infos[i];
			LOG("found info\n");
			break;
		}
	}
	if (!info)
		return -EBUSY;
	
	//inode->i_mapping->a_ops=&nfs_file_aops;
	filp->f_mapping->a_ops = &fb_deferred_io_aops;
#ifdef CONFIG_HAVE_IOREMAP_PROT
	LOG("CONFIG_HAVE_IOREMAP_PROT\n");

#endif

	LOG("mmap_open %s\n",info->name);
	/*if (!mutex_trylock(&mmap_device_mutex)) {
		LOG(KERN_WARNING
		       "Another process is accessing the device\n");
		return -EBUSY;
	}*/

	//info->inode=inode;
	
	filp->private_data = info;    
	printk("mmap_open1 reference %d -- %s\n",info->reference,info->name);


	if (info->reference == 0)
	{
		info->thread = kthread_create(fb_deferred_io_work,info,"thread");
		if((info->thread))
			{
			LOG(KERN_INFO "in if\n");
			wake_up_process(info->thread);
			}

	}
	info->reference++;

    return 0;
}





void mmap_close(struct vm_area_struct *vma)
{
	struct page* page;
	int32_t *index;
	struct mmap_info *info = (struct mmap_info *)vma->vm_private_data;
	LOG("mmap_close %d %s\n",info->reference,info->name);
	for ( index = info->dirt_pages; *index>=0  ; index++)
	{
		page=virt_to_page(info->data[*index]);
		page->mapping = NULL;
	}
}


#if LINUX_VERSION_CODE > KERNEL_VERSION(4,17,0)
static vm_fault_t mmap_fault( struct vm_fault *vmf)
{
	struct page *page;
	struct mmap_info *info;
	struct vm_area_struct *vma=vmf->vma;
#elif LINUX_VERSION_CODE > KERNEL_VERSION(4,10,0)
static int mmap_fault( struct vm_fault *vmf)
{
	struct page *page;
	struct mmap_info *info;
	struct vm_area_struct *vma=vmf->vma;
#else

static int mmap_fault(struct vm_area_struct *vma, struct vm_fault *vmf)
{
	struct page *page;
	struct mmap_info *info;
#endif
// SEE:/usr/src/linux-4.1.39-56/drivers/net/appletalk/ltpc.c:1133

    
	//unsigned long offset = ((vmf->pgoff % PAGES_PER_BLOCK) << PAGE_SHIFT);
	int block=vmf->pgoff>>PAGES_ORDER;
	LOG("mmap_fault gfp_mask:%x pgoff:%ld flags=%x\n",vmf->gfp_mask,vmf->pgoff,vmf->flags);
	//dmutex_lock(&mem_mutex);
	

	info = (struct mmap_info *)vma->vm_private_data;

	if (!info->data) {
		LOG("No data\n");
		return VM_FAULT_SIGBUS;
	}

	if (vmf->pgoff>=(size<<PAGES_ORDER))
	{
		LOG("mmap_fault overflow\n");
		return VM_FAULT_SIGBUS;
	}
	
	if (!info->data[block])
	{
		LOG("allocate page flags:%x chunk:%d address:%lx\n",vmf->flags,block,vmf->address);
		info->data[block]=(char *)get_zeroed_page(GFP_KERNEL|GFP_ATOMIC);
		info->mirror[block]=(char *)get_zeroed_page(GFP_KERNEL|GFP_ATOMIC);
		//info->data[block]=(char *)__get_free_pages(GFP_KERNEL| GFP_DMA | __GFP_NOWARN |__GFP_NORETRY, PAGES_ORDER);
		//memset(info->data[block],0,PAGE_SIZE<<PAGES_ORDER);
	}
	else 
	{
		//LOG("recycle page flags:%x chunk:%d offset:%lx \n",vmf->flags,block,offset);
	}

	page = vmalloc_to_page(info->data[block]);

	if (!page)
	{
		LOG("cannot find page\n");
		return VM_FAULT_SIGBUS;
	}	

	get_page(page);

	if (vma->vm_file)
		page->mapping = vma->vm_file->f_mapping;
	else
		LOG(KERN_ERR "no mapping available\n");


	//set_memory_uc(vmf->virtual_address,1);

	page->index = vmf->pgoff /*% PAGES_PER_BLOCK*/;


	//LOG("page:%p\n",page);

	//#define PAGE_MAPPING_MOVABLE 0x2
	//page->mapping = (long int)page->mapping | PAGE_MAPPING_MOVABLE;

	vmf->page = page;

	lock_page(page);
	//dmutex_unlock(&mem_mutex);
	return VM_FAULT_LOCKED;
}

int page_mkclean(struct page *page);


static int fb_deferred_io_work(void* ptr)
{
	struct mmap_info* info=ptr;
	struct page* page;
	int32_t *index;


	while (info->reference)
	{
		//if(!mutex_trylock(&etx_mutex)) schedule();
		wait_event_interruptible(wait_queue_transmit,  info->dirty||
				kthread_should_stop());
		info->dirty=false;
		mdelay(1);
		//dmutex_lock(&mem_mutex);
		//LOG("XXXX %p\n",info->dirt_pages);
		//LOG("XXXX %d\n",*info->dirt_pages);
		for ( index = info->dirt_pages; *index>=0  ; index++)
		{
			if (!info->data[*index])
			{
				LOG(KERN_ERR "ERROR %p %d\n",info->data[*index],*index);
				continue;
			}
			page=virt_to_page(info->data[*index]);
			//LOG("pre page %d\n",*index);
			lock_page(page);

			//LOG("page %d\n",*index);
			if (PageDirty(page))
			{
				ktime_t time1;
				LOG("dirty %d\n",*index);
				time1=ktime_get();
				
				transmitPage(info,*index);
				
				//LOG("time %lld\n",ktime_get()-time1);
				page_mkclean(page);
				ClearPageDirty(page);
				
			}
	
			unlock_page(page);
		}
		//dmutex_unlock(&mem_mutex);
		if(kthread_should_stop()) {
			LOG(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> terminated\n");
			do_exit(0);
		}

	}


	LOG(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>> done\n");

	return 0;
}




#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0))	
vm_fault_t page_mkwrite(struct vm_fault *vmf)
{
struct vm_area_struct *vma=vmf->vma;
#elif (LINUX_VERSION_CODE >= KERNEL_VERSION(4,10,0))	
int page_mkwrite(struct vm_fault *vmf)
{
struct vm_area_struct *vma=vmf->vma;
#else
int page_mkwrite(struct vm_area_struct *vma, struct vm_fault *vmf)
{
#endif
	int32_t *index;
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,10,0))	
	int myoff=vmf->pgoff;
#else
	unsigned int myoff=((long unsigned int)vmf->virtual_address-vma->vm_start+vmf->pgoff)/PAGE_SIZE;
#endif
	struct mmap_info *info = (struct mmap_info *)vma->vm_private_data;
	
	lock_page(vmf->page);
	
	//info->page=vmf->page;
	//info->x = myoff;
 	
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0))	
	LOG("page_mkwrite flags:%x offset:%ld pgoff:%ld page:%p\n",vmf->flags,vmf->address-vma->vm_start,vmf->pgoff,vmf->page);
#else
	LOG("page_mkwrite flags:%x pgoff:%d page:%p\n",vmf->flags,myoff,vmf->page);
#endif
	//dmutex_lock(&mem_mutex);
	// let's see if it's already there
	for (index = info->dirt_pages; (*index >=0)  && (myoff!=*index) ; index++)
	{
		LOG("page_mkwrite flags:%x pgoff:%d page:%p\n",vmf->flags,myoff,vmf->page);
		if ((index-info->dirt_pages)>blocks)
		{
			LOG("overflow\n");
			return VM_FAULT_SIGBUS;
		}
	}
	*index=myoff;
	//dmutex_unlock(&mem_mutex);
	LOG("index=%d %d\n",*index,myoff);

	//LOG("unlocking \n");
	//dmutex_unlock(&etx_mutex);
	info->dirty=true;
	wake_up(&wait_queue_transmit);
	
	//schedule_delayed_work(&info->deferred_work, 1);



	return VM_FAULT_LOCKED;

}


void map_pages(struct vm_area_struct *vma, struct vm_fault *vmf)
{
#if (LINUX_VERSION_CODE >= KERNEL_VERSION(4,17,0))	
	LOG("map_pages flags:%x pgoff:%ld pgoff:%ld page:%p\n ",vmf->flags,vmf->pgoff,vmf->pgoff,vmf->page);
#else
	LOG("map_pages flags:%x pgoff:%ld page:%p\n ",vmf->flags,vmf->pgoff,vmf->page);
#endif

}


struct page * find_special_page( struct vm_area_struct *vma, unsigned long addr)
{
	LOG("find_special_page\n ");
	return NULL;
}

struct mempolicy *mempolicy(struct vm_area_struct *vma,
					unsigned long addr)
{
	LOG("!!!!!!!!!!!!!!!!mempolicy\n ");
	return NULL;

}




#ifdef CONFIG_NUMA
static int set_policy(struct vm_area_struct *vma,
				 struct mempolicy *new)
{
	int ret;
	LOG("!!!!!!!!!!!!!!!!set mempolicy\n ");
	ret = 0;
	return ret;
}

static struct mempolicy *get_policy(struct vm_area_struct *vma,
					       unsigned long addr)
{
	LOG("!!!!!!!!!!!!!!!!get mempolicy\n ");
		return vma->vm_policy;
}

#endif
struct vm_operations_struct mmap_vm_ops = {
	//.open = mmap_open1,
	.close = mmap_close,
	.fault = mmap_fault,
	//.map_pages = map_pages,
	.page_mkwrite = page_mkwrite,
#ifdef CONFIG_NUMA
	.set_policy	= set_policy,
	.get_policy	= get_policy,
#endif
	//.find_special_page = find_special_page,
};




int mmapfop_close(struct inode *inode, struct file *filp)
{
	int err=0;
	struct mmap_info *info = (struct mmap_info *)filp->private_data;
	info->reference--;

	printk("mmapfop_close reference %d -- %s\n",info->reference,info->name);

	if (info->reference==0)
	{
		//dmutex_unlock(&etx_mutex);
		info->dirty=true;
		//wake_up(&wait_queue_transmit);
	    printk( KERN_ERR "vrfm: stopping thread\n" );
		if((info->thread))
		{
		    printk( KERN_ERR "vrfm: thread true\n" );
			err=kthread_stop(info->thread);
			if(err)
			{
				printk(KERN_INFO "Thread stop error %d\n",err);
			}
			else
			{
				LOG(KERN_INFO "Thread stopped\n");
			}
			info->thread=NULL;
		}
	}
	//  cancel_delayed_work(&info->deferred_work);

	return 0;
}

static inline int private_mapping_ok(struct vm_area_struct *vma)
{
	return vma->vm_flags & VM_MAYSHARE;
}
/*
inline int valid_phys_addr_range(phys_addr_t addr, size_t count)
{
	return addr + count <= __pa(high_memory);
}
*/


int memory_map (struct file * file, struct vm_area_struct * vma)
{


	//if(remap_pfn_range(vma, vma->vm_start, vma->vm_pgoff, vma->vm_end - vma->vm_start, vma->vm_page_prot))   //Create page table 
    //	 return - EAGAIN;
	//size_t size = vma->vm_end - vma->vm_start;

	file->f_flags |= O_DIRECT;


	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);

    vma->vm_ops = &mmap_vm_ops;

	vma->vm_flags |= VM_DONTEXPAND /*| VM_DONTDUMP | VM_IO | VM_MIXEDMAP*/;
	vma->vm_flags |= VM_IO;

	vma->vm_private_data = file->private_data;
	vma->vm_page_prot = pgprot_noncached(vma->vm_page_prot);
	file->f_mapping->a_ops = &fb_deferred_io_aops;


/*
	if (remap_pfn_range(vma,
			    vma->vm_start,
			    vma->vm_pgoff,
			    size,
			    vma->vm_page_prot)) {
		LOG("memory_map: ERROR\n");					
		return -EAGAIN;
	}*/
	//mmap_open1(vma);    
	


	
    return 0;
}


int mmap_ops_init(void)
{
	LOG("mmap_ops_init: size=%dM blocks:%d with %d pages/block\n",size/1024/1024,blocks,PAGES_PER_BLOCK);

	//blocks_array=kmalloc(blocks*sizeof(char*), GFP_KERNEL);
	//memset(blocks_array,0,blocks*sizeof(char*));


	//info = kmalloc(sizeof(struct mmap_info), GFP_KERNEL);
	

	//info->data = blocks_array;
	//info->reference = 0;
	//info->delay=0;

 	//INIT_DELAYED_WORK(&info->deferred_work, fb_deferred_io_work);
	//dmutex_init(&mem_mutex);	

	//dmutex_init(&etx_mutex);	
	init_waitqueue_head(&wait_queue_transmit);
	//dmutex_lock(&etx_mutex);

	
   //mutex_init(&mmap_device_mutex);
   return 0;
}
void mmap_shutdown()
{
	size_t i,j;
	int blocks=size/PAGE_SIZE/PAGES_PER_BLOCK;

	for (j = 0; j < rfm_instances; j++)
	{
		for (i = 0; i < blocks; i++)
		{
			free_pages((unsigned long)infos[j]->data[i],PAGES_ORDER);	
			free_pages((unsigned long)infos[j]->mirror[i],PAGES_ORDER);	
		}	

	}
}
