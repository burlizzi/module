#ifndef MMAP_H
#define MMAP_H
#include <linux/mm.h>

#define PAGES_ORDER 0 //LEAVE IT 0!!! can't make it work 
#define PAGES_PER_BLOCK (1<<PAGES_ORDER)


struct mmap_info {
	char name[100];
	char **data;
	int reference;
	short* dirt_pages;
	struct task_struct *thread;
	int index;
	struct mutex etx_mutex; 
	struct mutex mem_mutex; 

};
extern int blocks;
int mmap_ops_init(void);
int mmap_open(struct inode *inode, struct file *filp);
int memory_map (struct file * f, struct vm_area_struct * vma);
void mmap_close(struct vm_area_struct *vma);
int mmapfop_close(struct inode *inode, struct file *filp);
void mmap_shutdown(void);
#endif