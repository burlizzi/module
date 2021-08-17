struct mmap_info {
	char *data;
	int reference;
};

int mmap_ops_init(void);
int mmap_open(struct inode *inode, struct file *filp);
int memory_map (struct file * f, struct vm_area_struct * vma);
void mmap_close(struct vm_area_struct *vma);
int mmapfop_close(struct inode *inode, struct file *filp);