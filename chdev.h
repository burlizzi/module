ssize_t complete_write(struct file *filp,const char __user *buf,size_t count,loff_t *pos);
ssize_t device_file_read(
    struct file *file_ptr
    , char __user *user_buffer
    , size_t count
    , loff_t *position);
int chdev_init(void);
void chdev_shutdown(void);