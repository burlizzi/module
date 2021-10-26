#include "config.h"
#include "protocol.h"
#include "net.h"
#include <linux/netdevice.h>
#include <linux/lz4.h>
#include "mmap.h"


extern int size;


int transmit(unsigned int page)
{
    sendpacket(page);
    return false;
}

int receive(struct net_rfm* rec)
{
    int blocks=size/PAGE_SIZE/PAGES_PER_BLOCK;

    

    if (rec->page<0 ||  rec->page>=blocks)
    {
        LOG("invalid packet received:%d \n",rec->page);
        return -1;
    }
    if (!blocks_array[rec->page])
	{
		LOG("allocate page chunk:%d \n",rec->page);
		blocks_array[rec->page]=(char *)__get_free_pages(GFP_KERNEL, PAGES_ORDER);
		
	}
    //for (x=0;x<100;x++)
    //        printk("%x ", rec->data[x]);

    //printk("\n%x\n", rec->len);

    memcpy(blocks_array[rec->page],rec->data,rec->len);
    return 42;
}