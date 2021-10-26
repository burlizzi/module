#include <linux/delay.h>
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

//static DEFINE_MUTEX(mmap_device_mutex);
struct mmap_info *info = NULL;




int size = MAP_SIZE; // default

bool debug = true;
module_param(debug, bool,S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);





int blocks=MAP_SIZE/PAGE_SIZE/PAGES_PER_BLOCK;

char** blocks_array=NULL;
short* dirt_pages;
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



static void fb_deferred_io_work(struct work_struct *work);

int mmap_open(struct inode *inode, struct file *filp)
{


	/*if (!mutex_trylock(&mmap_device_mutex)) {
		LOG(KERN_WARNING
		       "Another process is accessing the device\n");
		return -EBUSY;
	}*/

	//info->inode=inode;
	filp->private_data = info;    
    return 0;
}

void mmap_open1(struct vm_area_struct *vma)
{
	struct mmap_info *info = (struct mmap_info *)vma->vm_private_data;
	info->reference++;
}



void mmap_close(struct vm_area_struct *vma)
{
	struct mmap_info *info = (struct mmap_info *)vma->vm_private_data;
	LOG("data close=%s\n",info->data[info->x]);

	LOG("mmap_close %d\n",info->reference);

	info->reference--;
}

#if LINUX_VERSION_CODE > KERNEL_VERSION(4,1,39)
static unsigned int mmap_fault( struct vm_fault *vmf)
{
	struct page *page;
	struct mmap_info *info;
	struct vm_area_struct *vma=vmf->vma;
    LOG("mmap_fault gfp_mask:%x pgoff:%ld\n",vmf->gfp_mask,vmf->pgoff);
#else

static int mmap_fault(struct vm_area_struct *vma, struct vm_fault *vmf)
{
	struct page *page;
	struct mmap_info *info;
#endif
// SEE:/usr/src/linux-4.1.39-56/drivers/net/appletalk/ltpc.c:1133

	unsigned long offset = ((vmf->pgoff % PAGES_PER_BLOCK) << PAGE_SHIFT);
	int block=vmf->pgoff>>PAGES_ORDER;

	info = (struct mmap_info *)vma->vm_private_data;

	if (!info->data) {
		LOG("No data\n");
		return VM_FAULT_SIGBUS;
	}

	if (vmf->pgoff>=size<<PAGES_ORDER)
	{
		LOG("mmap_fault overflow\n");
		return VM_FAULT_SIGBUS;
	}
	
	if (!info->data[block])
	{
		LOG("allocate page flags:%x chunk:%d \n",vmf->flags,block);
		info->data[block]=(char *)__get_free_pages(GFP_KERNEL, PAGES_ORDER);
		memset(info->data[block],0,PAGE_SIZE<<PAGES_ORDER);
	}
	else 
	{
		LOG("recycle page flags:%x chunk:%d offset:%lx \n",vmf->flags,block,offset);
	}

	page = virt_to_page(info->data[block]+offset);

	if (!page)
	{
		LOG("cannot find page\n");
		return VM_FAULT_SIGBUS;
	}	

	get_page(page);
	

	page->index = vmf->pgoff % PAGES_PER_BLOCK;


	LOG("page:%p\n",page);

	#define PAGE_MAPPING_MOVABLE 0x2
	//page->mapping = (long int)page->mapping | PAGE_MAPPING_MOVABLE;

	vmf->page = page;
	return 0;
}


static void fb_deferred_io_work(struct work_struct *work)
{
	struct page* page;
	short *index;
	//const char* data;
	//struct mmap_info* info=(struct mmap_info*)container_of(work, struct mmap_info, deferred_work);
	//LOG("get atomic data\n");
    
	
	//udelay(1000);
	//LOG("waiting for the process to release lock\n");
	
	LOG("fb_deferred_io_work %p(%d)\n",dirt_pages,*dirt_pages);
	//data=info->data[info->x];
	for ( index = dirt_pages; *index>=0  ; index++)
	{
		page=virt_to_page(blocks_array[*index]);
		lock_page(page);
		transmit(*index);
		LOG("packet sent %p %d\n",blocks_array[*index],*index);
		unlock_page(page);
		//mlock(blocks_array[*index],PAGE_SIZE);
		//page->
		//put_page(page);
	}
	
	
	
	//LOG("data io_work= %s\n",data);
	//LOG("finished unlocked\n");
}


int page_mkwrite(struct vm_area_struct *vma, struct vm_fault *vmf)
{
	short *index;
	int myoff=((long unsigned int)vmf->virtual_address-vma->vm_start)/PAGE_SIZE/PAGES_PER_BLOCK;
	struct mmap_info *info = (struct mmap_info *)vma->vm_private_data;
	LOG("page_mkwrite\n");
	lock_page(vmf->page);
	LOG("atomic_set\n");
	info->page=vmf->page;
	info->x = myoff;
 	
	LOG("page_mkwrite flags:%x pgoff:%d max_pgoff:%ld page:%p\n ",vmf->flags,myoff,vmf->max_pgoff,vmf->page);
	for ( index = dirt_pages; *index>=0  && myoff!=*index ; index++)
	{
		if (index-dirt_pages>blocks)
		{
			return VM_FAULT_SIGBUS;
		}
	}
	*index=myoff;
	schedule_delayed_work(&info->deferred_work, 1);
	return VM_FAULT_LOCKED;

}


int access(struct vm_area_struct *vma, unsigned long addr,
		      void *buf, int len, int write)
{
	LOG("access addr:%lx buf:%p len:%d write:%d\n ",addr,buf,len,write);
	return 0;
}

void map_pages(struct vm_area_struct *vma, struct vm_fault *vmf)
{
	LOG("map_pages flags:%x pgoff:%ld max_pgoff:%ld page:%p\n ",vmf->flags,vmf->pgoff,vmf->max_pgoff,vmf->page);

}

struct vm_operations_struct mmap_vm_ops = {
	.open = mmap_open1,
	.close = mmap_close,
	.fault = mmap_fault,
	.map_pages = map_pages,
	.page_mkwrite = page_mkwrite,
	.access = access
};




int mmapfop_close(struct inode *inode, struct file *filp)
{
    LOG("mmapfop_close\n");
	//mutex_unlock(&mmap_device_mutex);
	return 0;
}




int memory_map (struct file * f, struct vm_area_struct * vma)
{
    vma->vm_ops = &mmap_vm_ops;
	vma->vm_flags |= VM_DONTEXPAND | VM_DONTDUMP/* | VM_IO | VM_MIXEDMAP*/;
	vma->vm_private_data = f->private_data;
	mmap_open1(vma);    
	
    return 0;
}


int mmap_ops_init(void)
{
	printk("mmap_ops_init: size=%dM blocks:%d with %d pages/block\n",size/1024/1024,blocks,PAGES_PER_BLOCK);

	blocks_array=kmalloc(blocks*sizeof(char*), GFP_KERNEL);
	memset(blocks_array,0,blocks*sizeof(char*));

	dirt_pages=kmalloc(size/PAGE_SIZE*sizeof(short), GFP_KERNEL);
	memset(dirt_pages,-1,size/PAGE_SIZE*sizeof(short));

	info = kmalloc(sizeof(struct mmap_info), GFP_KERNEL);
	

	info->data = blocks_array;
	//info->delay=0;

 	INIT_DELAYED_WORK(&info->deferred_work, fb_deferred_io_work);
	
   //mutex_init(&mmap_device_mutex);
   return 0;
}
void mmap_shutdown()
{
	size_t i;
	int blocks=size/PAGE_SIZE/PAGES_PER_BLOCK;
	for (i = 0; i < blocks; i++)
	{
		free_pages((unsigned long)blocks_array[i],PAGES_ORDER);	
	}	
}