#include "config.h"
#include "protocol.h"
#include "net.h"
#include <linux/netdevice.h>
#include <linux/lz4.h>
#include "mmap.h"


extern int size;





int transmit(unsigned int offset)
{
    sendpacket(offset);
    return false;
}

int receive(struct net_rfm* rec,size_t len)
{
    int blocks=size/PAGE_SIZE/PAGES_PER_BLOCK;
    
    
    /*
    size_t i,j;
    char line[17*3];
    unsigned char *b=(unsigned char *)rec;
    
    for ( i = 0; i < sizeof(struct ethhdr); i++)
    {
        sprintf(line+i*3,"%02x ",b[i]);
    }
    line[16*3+1]='\n';
    line[16*3+2]=0;
    LOG(line);
    */


    if (rec->offset<0 ||  rec->offset>=size)
    {
        LOG("invalid packet received:%d \n",rec->offset);
        return -1;
    }
    if (!blocks_array[rec->offset/PAGE_SIZE])
	{
		LOG("allocate page chunk:%d \n",rec->offset/PAGE_SIZE);
		blocks_array[rec->offset/PAGE_SIZE]=(char *)__get_free_pages(GFP_KERNEL, PAGES_ORDER);
		
	}

    if (len+(rec->offset % PAGE_SIZE)>PAGE_SIZE)
    {
        LOG("packet not aligned to page:%d \n",len+(rec->offset % PAGE_SIZE));
        return -1;
    }

    //LOG("copy %d bytes page %d page_offset %d \n",len,rec->offset/PAGE_SIZE,rec->offset % PAGE_SIZE);
    memcpy(blocks_array[rec->offset/PAGE_SIZE]+(rec->offset % PAGE_SIZE),rec->data,len);
    return 42;
}