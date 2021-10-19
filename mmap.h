struct mmap_info {
	struct delayed_work deferred_work;
	char **data;
	int reference;
	struct file *lower_file;
	//const struct vm_operations_struct *lower_vm_ops;	
	//int delay;
	struct page* page;
	int x;
};

int mmap_ops_init(void);
int mmap_open(struct inode *inode, struct file *filp);
int memory_map (struct file * f, struct vm_area_struct * vma);
void mmap_close(struct vm_area_struct *vma);
int mmapfop_close(struct inode *inode, struct file *filp);
void mmap_shutdown(void);