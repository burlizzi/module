#include "config.h"
#include "protocol.h"
#include "net.h"
#include <linux/netdevice.h>
//#include <linux/lz4.h>
#include "mmap.h"
#include <linux/pagemap.h>
#include <linux/kthread.h>  // for threads

extern int size;
extern int pktsize;

extern struct mutex mem_mutex; 


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
    //struct page* page;
    //mutex_lock(&mem_mutex);

    
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
    //page=virt_to_page(blocks_array[rec->header.offset/PAGE_SIZE]);
    //lock_page(page);
    switch (rec->header.cmd)
    {
    case VRFM_DUMP_ALL:
        {
            size_t i;
            for (i = 0; i < MAP_SIZE/PAGE_SIZE/PAGES_PER_BLOCK; i++)
            {
                if (blocks_array[i])
                    transmitPage(i);
            }

        }
        break;
    case VRFM_MEM_SEND:
        if(unlikely(rec->header.offset+len>PAGE_SIZE))//we crossed the boundaries
        {
            int offsetinpage=rec->header.offset % PAGE_SIZE;
            memcpy(blocks_array[rec->header.offset/PAGE_SIZE]+(offsetinpage),rec->data,PAGE_SIZE-offsetinpage);
            memcpy(blocks_array[rec->header.offset/PAGE_SIZE+1]+(offsetinpage),rec->data,len+offsetinpage-PAGE_SIZE);
        }
        else
            memcpy(blocks_array[rec->header.offset/PAGE_SIZE]+(rec->header.offset % PAGE_SIZE),rec->data,len);
        break;
    
    default:
        break;
    }
    
    //mutex_unlock(&mem_mutex);
    //unlock_page(page);
    return 42;
}
