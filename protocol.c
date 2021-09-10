#include "protocol.h"
#include "net.h"
#include <linux/netdevice.h>



int receive(const char*data, unsigned int size)
{
    int x;
    for (x=sizeof(struct ethhdr);x<size;x++)
            printk("%x ", data[x]);
    printk("\n%x\n", size);
    return 42;
}