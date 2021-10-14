#include <linux/delay.h>
#include <linux/mm.h>
#include <linux/pagemap.h>
#include <linux/mm_types.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/version.h>
#include <linux/kernel.h>
#include "mmap.h"
#include "net.h"
#include <linux/moduleparam.h>
#include <linux/dma-mapping.h>

//static DEFINE_MUTEX(mmap_device_mutex);
struct mmap_info *info = NULL;




int size = MAP_SIZE; // default
static bool debug = true;
module_param(debug, bool,S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);


#define PAGES_ORDER 0 //LEAVE IT 0!!! can't make it work 
#define PAGES_PER_BLOCK (1<<PAGES_ORDER)


#define LOG if (unlikely(debug)) printk

char** page_array;


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
	vmf->page = page;
	
	return 0;
}


static void fb_deferred_io_work(struct work_struct *work)
{
	struct page* page;
	LOG("get atomic data\n");
    page=container_of(work, struct mmap_info, deferred_work)->page;
	
	
	LOG("waiting for the process to release lock\n");
	lock_page(page);
	LOG("process finished, locked, sending packet\n");
	const char* data=(const char*)page_to_phys(page);

	//sendpacket(addr,PAGE_SIZE);
	LOG("packet sent %p\n",data);//so far it deadlocks if data is accessed
	unlock_page(page);
	LOG("finished unlocked\n");
}


int page_mkwrite(struct vm_area_struct *vma, struct vm_fault *vmf)
{
	struct mmap_info *info = (struct mmap_info *)vma->vm_private_data;
	LOG("page_mkwrite\n");
	lock_page(vmf->page);
	LOG("atomic_set\n");
	info->page=vmf->page;
 	schedule_delayed_work(&info->deferred_work, info->delay);
	LOG("page_mkwrite flags:%x pgoff:%ld max_pgoff:%ld page:%p\n ",vmf->flags,vmf->pgoff,vmf->max_pgoff,vmf->page);
	return VM_FAULT_LOCKED;
}


struct vm_operations_struct mmap_vm_ops = {
	.open = mmap_open1,
	.close = mmap_close,
	.fault = mmap_fault,
	//.map_pages = map_pages,
	.page_mkwrite = page_mkwrite,
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
	vma->vm_flags |= VM_DONTEXPAND | VM_DONTDUMP;
	vma->vm_private_data = f->private_data;
	mmap_open1(vma);    
    return 0;
}


int mmap_ops_init(void)
{
	int pages=size/PAGE_SIZE/PAGES_PER_BLOCK;
	printk("mmap_ops_init: size=%dM blocks:%d with %d pages/block\n",size/1024/1024,pages,PAGES_PER_BLOCK);

	page_array=kmalloc(pages*sizeof(char*), GFP_KERNEL);
	memset(page_array,0,pages*sizeof(char*));

	info = kmalloc(sizeof(struct mmap_info), GFP_KERNEL);
	

	info->data = page_array;
	info->delay=0;

 	INIT_DELAYED_WORK(&info->deferred_work, fb_deferred_io_work);
   //mutex_init(&mmap_device_mutex);
   return 0;
}
void mmap_shutdown()
{
	size_t i;
	int pages=size/PAGE_SIZE/PAGES_PER_BLOCK;
	for (i = 0; i < pages; i++)
	{
		free_pages((unsigned long)page_array[i],PAGES_ORDER);	
	}	
}