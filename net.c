#include "net.h"
#include <linux/version.h>
#include <linux/netdevice.h>
#include <linux/moduleparam.h>

#include <linux/netfilter.h>
#include <linux/netfilter_ipv4.h>
unsigned char dest[ETH_ALEN] = {0xff,0xff,0xff,0xff,0xff,0xff}; //broadcast

struct net_device* dev_eth;

static char *device = "eth0";
module_param(device, charp,0);
MODULE_PARM_DESC(device, "network interface to use");


void print_mac_hdr(struct ethhdr *eth);
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



static struct packet_type hook; /* Initialisation routine */




static int hook_func( struct sk_buff *skb)
{
        int x;
        struct ethhdr *eth;    
        

        eth= eth_hdr(skb);
        if (ntohs(eth->h_proto)==0x0632)
        { 
            print_mac_hdr(eth);
            for (x=0;x<skb->len;x++)
                    printk("%x ", skb->data[x]);
            printk("\n%x\n", skb->data_len);

        }

    kfree_skb(skb);
    return NF_DROP;
}

/*Print eth  headers*/
void print_mac_hdr(struct ethhdr *eth)
{
    printk("dest: %02x:%02x:%02x:%02x:%02x:%02x \n",eth->h_dest[0],eth->h_dest[1],eth->h_dest[2],eth->h_dest[3],eth->h_dest[4],eth->h_dest[5]);
    printk("orig: %02x:%02x:%02x:%02x:%02x:%02x\n",eth->h_source[0],eth->h_source[1],eth->h_source[2],eth->h_source[3],eth->h_source[4],eth->h_source[5]);
    printk("Proto: 0x%04x\n",ntohs(eth->h_proto));

}

void handler_add_config (void)
{
        hook.type = htons(ETH_P_ALL);
        hook.func = (void *)hook_func;
        hook.dev = NULL;
        dev_add_pack(&hook);
        printk(" Protocol adicionado!!!!\n");

}
void net_init(void)
{
    int err=0;
    printk(KERN_INFO "netif is : %s\n", device);
    dev_eth=dev_get_by_name(&init_net,device);
    if (!dev_eth)
    {
        printk("dev not found\n");
    }   

    handler_add_config();


  
    if (err) {
            printk (KERN_ERR "Could not register hook\n");
    }
    

}
void net_shutdown(void)
{
    dev_put(dev_eth);
    dev_remove_pack(&hook);

}
