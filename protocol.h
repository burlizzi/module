#ifndef PROTOCOL_H
#define PROTOCOL_H
#include "net.h"

int transmitPage(unsigned int page);
int receive(struct net_rfm* rec,size_t len);
#endif