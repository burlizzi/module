#include "config.h"
#include "protocol.h"
#include "net.h"
#include <linux/netdevice.h>
//#include <linux/lz4.h>
#include "mmap.h"
#include <linux/pagemap.h>

extern int size;
extern int pktsize;




int transmitPage(unsigned int offset)
{
    //LOG("------------------------->>>>packet sent %d\n",offset);
    size_t i;
    size_t len=PAGE_SIZE;
    for (i = 0; i < PAGE_SIZE/CHUNK+1; i++)
    {
        sendpacket(offset*PAGE_SIZE+i*CHUNK,len>CHUNK?CHUNK:len);
        len-=CHUNK;
    }
    return false;
}

int receive(struct net_rfm* rec,size_t len)
{
    //int blocks=size;
    
    
    /*
    
    size_t i,j;
    char line[19*3];
    memset16((uint16_t*)line,'\n\0',17*3/2);    
    unsigned char *b=(unsigned char *)rec;
    for ( i = 0; i < sizeof(struct ethhdr); i++)
    {
        sprintf(line+i*3,"%02x ",b[i]);
    }
    line[16*3+1]='\n';
    line[16*3+2]=0;
    LOG(line);
    */


    if (rec->header.offset<0 ||  rec->header.offset>=size)
    {
        LOG("invalid packet received:%d \n",rec->header.offset);
        return -1;
    }
    if (!blocks_array[rec->header.offset/PAGE_SIZE])
	{
		LOG("allocate page chunk:%lu \n",rec->header.offset/PAGE_SIZE);
		blocks_array[rec->header.offset/PAGE_SIZE]=(char *)__get_free_pages(GFP_KERNEL, PAGES_ORDER);
		
	}

    if (len+(rec->header.offset % PAGE_SIZE)>PAGE_SIZE)
    {
        LOG("packet not aligned to page:%lu \n",len+(rec->header.offset % PAGE_SIZE));
        return -1;
    }

    //LOG("copy %d bytes page %d page_offset %d \n",len,rec->header.offset/PAGE_SIZE,rec->header.offset % PAGE_SIZE);
    struct page* page=virt_to_page(blocks_array[rec->header.offset/PAGE_SIZE]);
    lock_page(page);
    memcpy(blocks_array[rec->header.offset/PAGE_SIZE]+(rec->header.offset % PAGE_SIZE),rec->data,len);
    unlock_page(page);
    return 42;
}
