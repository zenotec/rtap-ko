#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/stat.h>

#include <linux/version.h>

#include <linux/skbuff.h>
#include <linux/if_ether.h>
#include <linux/byteorder/generic.h>
#include <linux/netdevice.h>
#include <linux/netfilter.h>

#include <net/mac80211.h>
#include <net/ieee80211_radiotap.h>

#define DRIVER_VERSION  "v1.0"
#define DRIVER_AUTHOR   "Kevin Mahoney <kevin.mahoney@zenotec.net>"
#define DRIVER_NAME     "rtap"
#define DRIVER_DESC     "RadioTap forwarder"

/* Module information */
MODULE_AUTHOR( DRIVER_AUTHOR );
MODULE_DESCRIPTION( DRIVER_DESC );
MODULE_LICENSE("GPL");

/* Module parameters */
static char *devname, *ipaddr;
static int port;
module_param( devname, charp, 0 );
MODULE_PARM_DESC( devname, "Monitor Interface" );
module_param( ipaddr, charp, 0 );
MODULE_PARM_DESC( ipaddr, "Remote listener" );
module_param( port, int, 0 );
MODULE_PARM_DESC( port, "Remote listen port" );

static struct net_device *netdev = 0;

static int cnt = 100;

int rtap_func(struct sk_buff *skb, struct net_device *dev,
              struct packet_type *pt, struct net_device *orig_dev)
{
    struct ethhdr *mh = skb->mac_header;
    struct ieee80211_radiotap_header *rthdr = (struct ieee80211_radiotap_header *)skb->data;
    static struct ieee80211_radiotap_iterator rti;
    int ret;
    if(cnt)
    {
    	cnt--;
        printk(KERN_INFO "Received packet\n");
        printk(KERN_INFO "            Type: 0x%x\n", skb->pkt_type);
        printk(KERN_INFO "        Protocol: 0x%x\n", ntohs(skb->protocol));
        printk(KERN_INFO "    Total Length: %d\n", skb->len);
        printk(KERN_INFO "     Data Length: %d\n", skb->data_len);
        printk(KERN_INFO "      Mac Header: 0x%x\n", skb->mac_header);
        printk(KERN_INFO "  Network Header: 0x%x\n", skb->network_header);
        printk(KERN_INFO "Transport Header: 0x%x\n", skb->transport_header);
	if(mh)
        {
/*
            printk(KERN_INFO "   Dest Mac: %x:%x:%x:%x:%x:%x\n", mh->h_dest[0], mh->h_dest[1],
                   mh->h_dest[2], mh->h_dest[3], mh->h_dest[4], mh->h_dest[5]);
            printk(KERN_INFO "    Src Mac: %x:%x:%x:%x:%x:%x\n", mh->h_source[0], mh->h_source[1],
                   mh->h_source[2], mh->h_source[3], mh->h_source[4], mh->h_source[5]);
            printk(KERN_INFO "      Proto: 0x%x\n", ntohs(mh->h_proto));
*/
        } // end if
        if(rthdr)
        {
            printk(KERN_INFO "RadioTap Version: 0x%x\n", rthdr->it_version);
            printk(KERN_INFO "    RadioTap Pad: 0x%x\n", rthdr->it_pad);
            printk(KERN_INFO " RadioTap Length: 0x%x\n", rthdr->it_len);
            printk(KERN_INFO "RadioTap Present: 0x%x\n", rthdr->it_present);
            ret = ieee80211_radiotap_iterator_init(&rti, rthdr, skb->len);
            while(!ret)
            {
                ret = ieee80211_radiotap_iterator_next(&rti);
                switch (rti.this_arg_index)
                {
                    case IEEE80211_RADIOTAP_FLAGS:
                        printk(KERN_INFO "      RadioTap Flags: %x\n", *(u8 *)rti.this_arg);
                        break;
                    case IEEE80211_RADIOTAP_RATE:
                        printk(KERN_INFO "       RadioTap Rate: %d (Kbps)\n", (*(u8 *)rti.this_arg * 500));
                        break;
                    case IEEE80211_RADIOTAP_CHANNEL:
                        printk(KERN_INFO "RadioTap Channel Freq: %d\n", *(u16 *)rti.this_arg);
                        printk(KERN_INFO "RadioTap Channel Flag: %x\n", *(u16 *)(rti.this_arg + 2));
                        break;
                    case IEEE80211_RADIOTAP_ANTENNA:
                        printk(KERN_INFO "    RadioTap Antenna: %d\n", *(u8 *)rti.this_arg);
                        break;
                    case IEEE80211_RADIOTAP_DBM_ANTSIGNAL:
                        printk(KERN_INFO "  RadioTap RX Signal: %d\n", *(s8 *)rti.this_arg);
                        break;
                    default:
                        printk(KERN_INFO "      RadioTap Param: %d\n", rti.this_arg_index);
                        break;
                } // end switch
            } // end while
        } // end if
    } // end if
    kfree_skb(skb);
    return(0);
}

static struct packet_type rtap_pt =
{
    .type = __constant_htons(ETH_P_ALL),
    .func = rtap_func,
};

static int __init rtap_init(void)
{

    if (!devname)
    {
        printk( KERN_ERR "Error: missing remote host IP\n");
        printk( KERN_ERR "Usage: insmod rtap.ko dev=devname ip=x.x.x.x port=rport\n\n");
        return -ENXIO;
    } // end if

    if(!ipaddr)
    {
        printk( KERN_ERR "Error: missing remote host IP\n");
        printk( KERN_ERR "Usage: insmod rtap.ko dev=devname ip=x.x.x.x port=rport\n\n");
        return -ENXIO;
    } // end if

    netdev = first_net_device(&init_net);
    while (netdev)
    {
        printk(KERN_INFO "found [%s]\n", netdev->name);
        if(!strcmp(netdev->name, devname))
            break;
        netdev = next_net_device(netdev);
    } // end while

    if (!netdev)
    {
        printk("Did not find device %s!\n", devname);
        return -ENXIO;
    } // end if

    rtap_pt.dev = netdev;
    dev_add_pack(&rtap_pt);

    printk( KERN_INFO "RTAP: Driver registered\n" );

    /* Return ok */
    return( 0 );

}


static void __exit rtap_exit(void)
{
    printk( KERN_INFO "RTAP: Unloading module...\n" );
    dev_remove_pack(&rtap_pt);
    printk( KERN_INFO "...done.\n" );
    return;
}

module_init(rtap_init);
module_exit(rtap_exit);

