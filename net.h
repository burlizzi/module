#ifndef NET_H
#define NET_H

#include "mmap.h"
#define ETH_ALEN 6
#define CHUNK 1024
struct net_rfm
{
    u_int32_t offset;
    char data[CHUNK];
}__attribute__((packed));


int sendpacket (unsigned int offset);
void net_shutdown(void);
int net_init(void);

#endif