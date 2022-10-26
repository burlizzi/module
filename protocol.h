#ifndef PROTOCOL_H
#define PROTOCOL_H
#include "net.h"

int transmitPage(struct mmap_info* info,unsigned int page);
int receive(struct mmap_info* info,struct net_rfm* rec,size_t len);
#endif