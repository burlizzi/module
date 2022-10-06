#include "config.h"
#include "net.h"
#include "protocol.h"
#include <linux/crc32.h>
#include <linux/netdevice.h>
#include <linux/moduleparam.h>
#include <linux/version.h>

unsigned char dest[ETH_ALEN] = {0xff,0xff,0xff,0xff,0xff,0xff}; //broadcast

struct net_device* dev_eth;


static char *netdevice = "eth0";
module_param(netdevice, charp,S_IRUGO);
MODULE_PARM_DESC(netdevice, "network interface to use default=eth0");


static int pktsize = ETH_DATA_LEN;
module_param(pktsize, int,S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
MODULE_PARM_DESC(pktsize, "ethernet packet size default=" __stringify(ETH_DATA_LEN) "");

void p(const char* s)
{
    LOG(s);
}


int sendpacket (unsigned int offset,unsigned int length)
{
    struct net_rfm* eth;
    struct sk_buff * skbt;
    int offsetinpage=offset % PAGE_SIZE;
    int block=offset/PAGE_SIZE;


    if(length<50)
        length=50;

    if(length>pktsize)
    {
        LOG("vrfm: too big of a packet\n");
        return -1;
    }


    if(!blocks_array[offset/PAGE_SIZE])
    {
        LOG("vrfm: blocks_array %lu not allocated!!\n",offset/PAGE_SIZE);
        return -1;
    }
    /*if ((offset%PAGE_SIZE) + length>PAGE_SIZE)
    {
        LOG("vrfm: out of page boundaries!!\n");
        return -1;
    }*/
    

    
    skbt =alloc_skb(ETH_HLEN+sizeof(struct rfm_header)+length,GFP_KERNEL);      
    if (!skbt)
    {
        LOG("vrfm: cannot allocate alloc_skb!!\n");
        return -1;
    }
    skb_reserve(skbt,ETH_HLEN+sizeof(struct rfm_header)+length);

    skb_reset_mac_header(skbt);
    
    eth = (struct net_rfm*) skb_push(skbt, sizeof(struct rfm_header)+length);
    if (!eth)
    {
        LOG("vrfm: cannot allocate skb_push!!\n");
        return -1;
    }
    //eth->len=PAGE_SIZE*PAGES_PER_BLOCK;
    //eth->header.size=length;

//    LOG("sendpacket %u %u %u %u |%s\n",offset,length,offset %PAGE_SIZE,(offset %PAGE_SIZE) + length,&blocks_array[block][offsetinpage]);

    if( unlikely(offsetinpage+length>PAGE_SIZE))//we crossed the boundaries
    {
        if(!blocks_array[block+1])
        {
            LOG("vrfm: blocks_array+1=%d not allocated!!\n",block+1);
            return -1;
        }

        memcpy(eth->data,                        &blocks_array[block][offsetinpage],PAGE_SIZE-offsetinpage);
        memcpy(eth->data+PAGE_SIZE-offsetinpage, &blocks_array[block+1][0],         length+offsetinpage-PAGE_SIZE);
    }
    else
        memcpy(eth->data, &blocks_array[block][offsetinpage],length);



    skbt->dev=dev_eth;
    eth->header.offset=offset;
    eth->header.crc=crc32_le(0,eth->data,length);    

    dev_hard_header(skbt,dev_eth,ETH_P_802_3,dest,dev_eth->dev_addr,PROT_NUMBER);
    skbt->protocol = PROT_NUMBER;
    if (skb_network_header(skbt) < skbt->data ||
                skb_network_header(skbt) > skb_tail_pointer(skbt)) 
    {
        LOG("error: %d %ld %d \n",skb_network_header(skbt) < skbt->data , (long)skbt->data, (int)(skb_network_header(skbt) > skb_tail_pointer(skbt)));
    }
    
#if LINUX_VERSION_CODE > KERNEL_VERSION(5,2,0)
retry:
    switch(__dev_direct_xmit(skbt,0))
#else    
    switch(dev_queue_xmit(skbt))
#endif    
    {
        case NET_XMIT_SUCCESS:
        break;
        case NET_XMIT_DROP:
        LOG("vrfm: tx dropped packet, retry!!\n");
        usleep_range(100,1000);
        return 1;
#if LINUX_VERSION_CODE > KERNEL_VERSION(5,2,0)
        case NETDEV_TX_BUSY:
            usleep_range(1,10);
            if (__dev_direct_xmit(skbt,0)==NETDEV_TX_BUSY)
            {
                LOG("vrfm: tx busy, retry!!\n");
                usleep_range(1,10);
                goto retry;
            }
            return 0;
#endif    
        default:
            LOG("vrfm: cannot send packet!!\n");
            return -1;
    }
//    netif_wake_queue(dev_eth);

    return 0;
}



static struct packet_type hook; /* Initialisation routine */


/*Print eth  headers*/
void print_mac_hdr(struct ethhdr *eth)
{
    LOG("dest: %02x:%02x:%02x:%02x:%02x:%02x \n",eth->h_dest[0],eth->h_dest[1],eth->h_dest[2],eth->h_dest[3],eth->h_dest[4],eth->h_dest[5]);
    LOG("orig: %02x:%02x:%02x:%02x:%02x:%02x\n",eth->h_source[0],eth->h_source[1],eth->h_source[2],eth->h_source[3],eth->h_source[4],eth->h_source[5]);
    LOG("Proto: 0x%04x\n",ntohs(eth->h_proto));

}

static int hook_func( struct sk_buff *skb,
					 struct net_device *in,
					 struct packet_type *pt,
					 struct net_device *out)
{
        struct ethhdr *eth;    
        
        eth= eth_hdr(skb);
        //unsigned char* data=skb->data;
        

        if (ntohs(eth->h_proto)==PROT_NUMBER)
        if (memcmp(in->dev_addr,eth->h_source,ETH_ALEN))
        { 
            struct net_rfm* data=(struct net_rfm*)(skb->data);
            unsigned int len=skb->len-sizeof(struct ethhdr);
            LOG("received data\n");
            if (data->header.crc!=crc32(0,data->data,len))
                LOG("CRC ERROR\n");

            
            //if (skb->data) receive((struct net_rfm*)(skb->data));
            
            receive(data,len);
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
        LOG("VRFM Protocol registered with packet type 0x%x. pktsize=%d\n",PROT_NUMBER,pktsize);
        return 0;

}
int net_init(void)
{
    int err=0;
    int i=1;
    
    LOG(KERN_INFO "netif is : %s\n", netdevice);
    dev_eth=dev_get_by_name(&init_net,netdevice);
    if (!dev_eth)
    {
        LOG("dev not found, choose one of the following:\n");
        for (dev_eth=dev_get_by_index(&init_net,1);(dev_eth=dev_get_by_index(&init_net,i)) && i < 100 && dev_eth; i++)
        {
            LOG("dev %d: %s\n",i,dev_eth->name);
            dev_put(dev_eth);
        }
        

        return -1;
    }   

    err=handler_add_config();

  
    if (err) {
            LOG (KERN_ERR "Could not register network hook\n");
            return -1;
    }
    return 0;

}
void net_shutdown(void)
{
    dev_put(dev_eth);
    dev_remove_pack(&hook);
}
