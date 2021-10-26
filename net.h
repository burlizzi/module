#ifndef NET_H
#define NET_H

#include "mmap.h"
#define ETH_ALEN 6

struct net_rfm
{
    short page;
    short len;
    char data[PAGES_PER_BLOCK*PAGE_SIZE];
    //char data[100];
}__attribute__((packed));


int sendpacket (unsigned int page);
void net_shutdown(void);
int net_init(void);

#endif