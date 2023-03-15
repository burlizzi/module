#include "config.h"
#include "protocol.h"
#include "net.h"
#include "crypt.h"

#include <linux/netdevice.h>
//#include <linux/lz4.h>
#include "mmap.h"
#include <linux/pagemap.h>
#include <linux/kthread.h>  // for threads

extern int size;
extern int pktsize;

extern struct mutex mem_mutex; 

inline int
memcmp1 (const void *__s1, const void *__s2, size_t __n)
{
  register unsigned long int __d0, __d1;
  register unsigned int __d2;
  __asm__ __volatile__
    ("std\n\t"
     "repe; cmpsb\n\t"
     "cld\n\t"
     : "=&S" (__d0), "=&D" (__d1), "=&c" (__d2)
     : "0" (__s1 + __n -1), "1" (__s2 + __n -1), "2" (__n+1),
       "m" ( *(struct { __extension__ char __x[__n]; } *)__s1),
       "m" ( *(struct { __extension__ char __x[__n]; } *)__s2)
     : "cc");

  return __d2;
}


int transmitPage(struct mmap_info* info,unsigned int offset   )
{
    //LOG("------------------------->>>>packet sent %d\n",offset);
    size_t i;
    int len=PAGE_SIZE;
    unsigned char* A=info->data[offset*PAGE_SIZE];
    unsigned char* B=info->mirror[offset*PAGE_SIZE];
    len=memcmp1(A,B,PAGE_SIZE);
    memcpy(B,A,PAGE_SIZE);
    for (i = 0; i < PAGE_SIZE/CHUNK+1 && len>0; i++)
    {
        sendpacket(info,offset*PAGE_SIZE+i*CHUNK,len>CHUNK?CHUNK:len,VRFM_MEM_SEND);
        len-=CHUNK;
    }
    return false;
}

int receive(struct mmap_info* info,struct net_rfm* rec,size_t len)
{
    //struct page* page;
    //mutex_lock(&mem_mutex);

    
    if (rec->header.offset<0 ||  rec->header.offset>=size)
    {
        LOG("invalid packet received:%d \n",rec->header.offset);

        return -1;
    }
    if (!info->data[rec->header.offset/PAGE_SIZE])
	{
		LOG("allocate page chunk:%lu \n",rec->header.offset/PAGE_SIZE);
		info->data[rec->header.offset/PAGE_SIZE]=(char *)__get_free_pages(GFP_KERNEL, PAGES_ORDER);
		
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
                if (info->data[i])
                    transmitPage(info,i);
            }

        }
        break;
    case VRFM_MEM_SEND:
        if(unlikely(rec->header.offset+len>PAGE_SIZE))//we crossed the boundaries
        {
            int offsetinpage=rec->header.offset % PAGE_SIZE;
            decrypt(info->data[rec->header.offset/PAGE_SIZE]+(offsetinpage),rec->data,PAGE_SIZE-offsetinpage);
            decrypt(info->data[rec->header.offset/PAGE_SIZE+1]+(offsetinpage),rec->data,len+offsetinpage-PAGE_SIZE);
        }
        else
            decrypt(info->data[rec->header.offset/PAGE_SIZE]+(rec->header.offset % PAGE_SIZE),rec->data,len);
        break;
    
    default:
        break;
    }
    /**/
    //mutex_unlock(&mem_mutex);
    //unlock_page(page);
    return 42;
}
