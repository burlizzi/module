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

static DEFINE_MUTEX(mmap_device_mutex);



int size = 65536; // default
static int order = 5; // default
char* pages;

int size2order(int size)
{
	int i;

	for (i = 0; i <32-PAGE_SHIFT ; i++)
	{
		if ((PAGE_SIZE<<i) >= size) 
		{
            return i+1; 
		}
	}
	return 1;
}

static int notify_param(const char *val, const struct kernel_param *kp)
{
        int res;
		int oldorder=order;
		char* newpages=0;

		printk("notify_param\n");
		res = param_set_int(val, kp); // Use helper for write variable
        if(res==0) {
                printk(KERN_INFO "New value of size = %d\n", size);
				order=size2order(size);

				if ((PAGE_SIZE<<(order-1))!=size)
				{
					size=(PAGE_SIZE<<(order-1));
					printk(KERN_WARNING "size is not a power of 2, new value of size = %d\n", size);
				}
				if (oldorder!=order)
				{
					printk("reallocating...\n");
					newpages= (char *)__get_free_pages(GFP_KERNEL, order);
					if (!pages)
					{
						printk("mmap_ops_init: cannot allocate memory\n");
						return 1;
					}
					memcpy(newpages,pages,min((1<<order),(1<<oldorder)));
					free_pages((unsigned long)pages,oldorder);
				}
				else 
				{
					printk("no need to reallocate\n");

				}

                return 0;
        }
		else
                printk(KERN_WARNING "cannot parse %s\n",val);
        return -1;
}

static int size_op_read_handler(char *buffer, const struct kernel_param *kp)
{
	printk("size_op_read_handler %d\n",size);
	snprintf(buffer,10,"%d",size);

	return strlen(buffer);
}

static const struct kernel_param_ops size_op_ops = {
	.set = &notify_param,
	.get = &size_op_read_handler
};

module_param_cb(size, &size_op_ops, &size, S_IRUGO|S_IWUSR);

int mmap_open(struct inode *inode, struct file *filp)
{
	struct mmap_info *info = NULL;

	if (!mutex_trylock(&mmap_device_mutex)) {
		printk(KERN_WARNING
		       "Another process is accessing the device\n");
		return -EBUSY;
	}

	info = kmalloc(sizeof(struct mmap_info), GFP_KERNEL);
	info->data = pages;
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
    //printk("after:%s\n",info->data);
    //printk("after:%s\n",info->data+PAGE_SIZE-10);
	//sendpacket(info->data,size);
	info->reference--;
}

#if LINUX_VERSION_CODE > KERNEL_VERSION(4,1,39)
static unsigned int mmap_fault( struct vm_fault *vmf)
{
	struct page *page;
	struct mmap_info *info;
	struct vm_area_struct *vma=vmf->vma;
    printk("mmap_fault gfp_mask:%x pgoff:%ld\n",vmf->gfp_mask,vmf->pgoff);
#else

static int mmap_fault(struct vm_area_struct *vma, struct vm_fault *vmf)
{
	struct page *page;
	struct mmap_info *info;
#endif

	info = (struct mmap_info *)vma->vm_private_data;
	if (!info->data) {
		printk("No data\n");
		return 0;
	}

	//page = virt_to_page(info->data+PAGE_SIZE*vmf->pgoff);
	page = virt_to_page(info->data);

	get_page(page);
	vmf->page = page;
    printk("mmap_fault flags:%x pgoff:%ld page:%p\n",vmf->flags,vmf->pgoff,page);
    

	return 0;
}
/*
void map_pages(struct vm_area_struct *fe, struct vm_fault *vmf)
{
	printk("map_pages flags:%x pgoff:%ld max_pgoff:%ld page:%p\n ",vmf->flags,vmf->pgoff,vmf->max_pgoff,vmf->page);

}


int page_mkwrite(struct vm_area_struct *vma, struct vm_fault *vmf)
{
	int ret = VM_FAULT_FALLBACK;//VM_FAULT_LOCKED;
	//lock_page(vmf->page);
	printk("page_mkwrite flags:%x pgoff:%ld max_pgoff:%ld page:%p\n ",vmf->flags,vmf->pgoff,vmf->max_pgoff,vmf->page);
	return ret;
}
*/

struct vm_operations_struct mmap_vm_ops = {
	.open = mmap_open1,
	.close = mmap_close,
	.fault = mmap_fault,
	//.map_pages = map_pages,
	//.page_mkwrite = page_mkwrite,
};




int mmapfop_close(struct inode *inode, struct file *filp)
{

	struct mmap_info *info = filp->private_data;
    printk("mmapfop_close\n");

	//free_page((unsigned long)info->data);
	kfree(info);
	filp->private_data = NULL;

	mutex_unlock(&mmap_device_mutex);

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
	int order;
	order=size2order(size);
	printk("mmap_ops_init: size=%d order=%d\n",size,order);
	pages= (char *)__get_free_pages(GFP_KERNEL, order);
	
	if (!pages)
	{
		printk("mmap_ops_init: cannot allocate memory\n");
		return 1;
	}
	memcpy(pages, "asdf", 5);
    mutex_init(&mmap_device_mutex);
    return 0;
}
void mmap_shutdown()
{
	free_pages((unsigned long)pages,size2order(size));
}