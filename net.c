#include "config.h"
#include "net.h"
#include "protocol.h"
#include <linux/netdevice.h>
#include <linux/moduleparam.h>

unsigned char dest[ETH_ALEN] = {0xff,0xff,0xff,0xff,0xff,0xff}; //broadcast

struct net_device* dev_eth;

static char *device = "eth0";
module_param(device, charp,S_IRUGO);
MODULE_PARM_DESC(device, "network interface to use");

static bool jumbo = true;
module_param(jumbo, bool,S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(jumbo, "use jumbo packet");

void p(const char* s)
{
    printk(s);
}

int sendpacket (unsigned int offset)
{
    struct net_rfm* eth;
    struct sk_buff * skbt =alloc_skb(ETH_HLEN+sizeof(struct net_rfm),GFP_KERNEL);      
    if (!skbt)
    {
        printk("vrfm: cannot allocate alloc_skb!!\n");
        return 0;
    }
    skb_reserve(skbt,ETH_HLEN+sizeof(struct net_rfm));

    skb_reset_mac_header(skbt);
    
    eth = (struct net_rfm*) skb_push(skbt, sizeof(struct net_rfm));
    if (!eth)
    {
        printk("vrfm: cannot allocate skb_push!!\n");
        return 0;
    }
    //eth->len=PAGE_SIZE*PAGES_PER_BLOCK;
    eth->offset=offset;
    memcpy(eth->data,blocks_array[offset/PAGE_SIZE] + (offset%PAGE_SIZE),CHUNK);
    skbt->dev=dev_eth;

    dev_hard_header(skbt,dev_eth,ETH_P_802_3,dest,dev_eth->dev_addr,0x612);
    skbt->protocol = 0x612;
    if (skb_network_header(skbt) < skbt->data ||
                skb_network_header(skbt) > skb_tail_pointer(skbt)) 
    {
        printk("error: %d %ld %d \n",skb_network_header(skbt) < skbt->data , (long)skbt->data, (int)(skb_network_header(skbt) > skb_tail_pointer(skbt)));
    }

    if(dev_queue_xmit(skbt)!=NET_XMIT_SUCCESS)
    {
        printk("vrfm: cannot send packet!!\n");
    }
    netif_wake_queue(dev_eth);

    return 0;
}



static struct packet_type hook; /* Initialisation routine */


/*Print eth  headers*/
void print_mac_hdr(struct ethhdr *eth)
{
    printk("dest: %02x:%02x:%02x:%02x:%02x:%02x \n",eth->h_dest[0],eth->h_dest[1],eth->h_dest[2],eth->h_dest[3],eth->h_dest[4],eth->h_dest[5]);
    printk("orig: %02x:%02x:%02x:%02x:%02x:%02x\n",eth->h_source[0],eth->h_source[1],eth->h_source[2],eth->h_source[3],eth->h_source[4],eth->h_source[5]);
    printk("Proto: 0x%04x\n",ntohs(eth->h_proto));

}

static int hook_func( struct sk_buff *skb,
					 struct net_device *in,
					 struct packet_type *pt,
					 struct net_device *out)
{
        struct ethhdr *eth;    
        size_t i,j;
        //int len;
        char line[17*3];
        
        eth= eth_hdr(skb);
        unsigned char* data=skb->data;
        memset(line,0,17*3);
        

        if (ntohs(eth->h_proto)==0x0612)
        { 
            //print_mac_hdr(eth);
            //len=(int)skb->tail-(int)skb->data;
/*
            LOG("p %d %p %p %x",skb->len,skb->data,skb->tail,eth->h_proto);
            //for (i = 0; i < 1; i++)
            {
              //  for ( j = 0; j < skb->len>14?14:skb->len; j++)
                for ( i = 0; i < sizeof(struct ethhdr); i++)
                {
                    sprintf(line+i*3,"%02x ",data[i]);
                }
                line[16*3+1]='\n';
                line[16*3+2]=0;
                LOG(line);
            }
/**/
            //if (skb->data) receive((struct net_rfm*)(skb->data));
            receive((struct net_rfm*)(skb->data),skb->len-4);
        }
        

    kfree_skb(skb);
    return NF_DROP;
}



int handler_add_config (void)
{
        hook.type = htons(ETH_P_ALL);
        hook.func = (void *)hook_func;
        hook.dev = dev_eth;
        dev_add_pack(&hook);
        printk("VRFM Protocol registered.\n");
        return 0;

}
int net_init(void)
{
    int err=0;
    int i=1;
    
    printk(KERN_INFO "netif is : %s\n", device);
    dev_eth=dev_get_by_name(&init_net,device);
    if (!dev_eth)
    {
        printk("dev not found, choose one of the following:\n");
        for (dev_eth=dev_get_by_index(&init_net,1);(dev_eth=dev_get_by_index(&init_net,i)) && i < 100 && dev_eth; i++)
        {
            printk("dev %d: %s\n",i,dev_eth->name);
            dev_put(dev_eth);
        }
        

        return -1;
    }   

    err=handler_add_config();


  
    if (err) {
            printk (KERN_ERR "Could not register hook\n");
            return -1;
    }
    return 0;

}
void net_shutdown(void)
{
    dev_put(dev_eth);
    dev_remove_pack(&hook);
}
