
#include <linux/version.h>

#include <linux/netdevice.h>
#include <linux/ip.h>
#include <linux/in.h>

int ksocket_send(struct socket *sock, struct sockaddr_in *addr, unsigned char *buf, int len)
{
        struct msghdr msg;
        struct iovec iov;
        mm_segment_t oldfs;
        int size = 0;

        if (sock->sk==NULL)
           return 0;

        iov.iov_base = buf;
        iov.iov_len = len;

        msg.msg_flags = 0;
        msg.msg_name = addr;
        msg.msg_namelen  = sizeof(struct sockaddr_in);
        msg.msg_control = NULL;
        msg.msg_controllen = 0;




#if LINUX_VERSION_CODE < KERNEL_VERSION(3,19,0)
msg.msg_iov = &iov;
msg.msg_iovlen = 1;
#else
iov_iter_init(&msg.msg_iter, READ, &iov, 1, len);
#endif



        msg.msg_control = NULL;

        oldfs = get_fs();
        set_fs(KERNEL_DS);
        size = sock_sendmsg(sock,&msg);
        set_fs(oldfs);

        return size;
}