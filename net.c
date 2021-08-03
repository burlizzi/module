#include "net.h"
#include <linux/netdevice.h>
#include <linux/moduleparam.h>

unsigned char dest[ETH_ALEN] = {0xff,0xff,0xff,0xff,0xff,0xff}; //broadcast

struct net_device* dev_eth;

static char *device = "eth0";
module_param(device, charp,0);
MODULE_PARM_DESC(device, "network interface to use");



int sendpacket (const char* data, unsigned int count)
{
    
    char * eth;
    struct sk_buff * skbt =alloc_skb(ETH_FRAME_LEN*2+2,GFP_KERNEL);      
    skb_reserve(skbt,ETH_HLEN+count);

    skb_reset_mac_header(skbt);

    eth = (char *) skb_push(skbt, count);
    memcpy(eth,data,count);

    skbt->dev=dev_eth;

    dev_hard_header(skbt,dev_eth,ETH_P_802_3,dest,dev_eth->dev_addr,ETH_P_802_3_MIN+50);
    skbt->protocol = ETH_P_802_3_MIN+50;
    if (skb_network_header(skbt) < skbt->data ||
                skb_network_header(skbt) > skb_tail_pointer(skbt)) 
    {
        printk("error: %d %ld %d \n",skb_network_header(skbt) < skbt->data , (long)skbt->data, (int)(skb_network_header(skbt) > skb_tail_pointer(skbt)));
    }

    if(dev_queue_xmit(skbt)!=NET_XMIT_SUCCESS)
    {
        printk("Not send!!\n");
    }
    netif_wake_queue(dev_eth);

    return 0;
}



void net_init(void)
{
    printk(KERN_INFO "netif is : %s\n", device);
    dev_eth=dev_get_by_name(&init_net,device);
    if (!dev_eth)
    {
        printk("dev not found\n");
    }   

}
void net_shutdown(void)
{
    dev_put(dev_eth);
}
