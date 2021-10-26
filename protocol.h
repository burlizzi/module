#ifndef PROTOCOL_H
#define PROTOCOL_H
#include "net.h"

int transmit(unsigned int page);
int receive(struct net_rfm* rec);
#endif