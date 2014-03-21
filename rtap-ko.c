//*****************************************************************************
//    Copyright (C) 2014 ZenoTec LLC (http://www.zenotec.net)
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License along
//    with this program; if not, write to the Free Software Foundation, Inc.,
//    51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
//
//    File: rtap-ko.c
//    Description: Main module for rtap.ko. Contains top level module
//                 initialization and parameter parsing.
//
//*****************************************************************************

//*****************************************************************************
// Includes
//*****************************************************************************

#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/stat.h>

#include <linux/types.h>
#include <linux/version.h>

#include <linux/skbuff.h>
#include <linux/if_ether.h>
#include <linux/byteorder/generic.h>
#include <linux/netdevice.h>
#include <linux/netfilter.h>

#include "rtap-ko.h"
#include "rule.h"
#include "filter.h"
#include "ksocket.h"

/* Module information */
MODULE_AUTHOR( DRIVER_AUTHOR );
MODULE_DESCRIPTION( DRIVER_DESC );
MODULE_LICENSE("GPL");

/* Module parameters */
static char *dev, *ip;
static int port = 8888;
module_param( dev, charp, 0 );
MODULE_PARM_DESC( dev, "Monitor Interface" );
module_param( ip, charp, 0 );
MODULE_PARM_DESC( ip, "Remote listener address" );
module_param( port, int, 0 );
MODULE_PARM_DESC( port, "Remote listen port" );

static struct net_device *netdev = 0;
static ksocket_t sockfd;
static struct sockaddr_in addr;

int rtap_func(struct sk_buff *skb, struct net_device *dev,
              struct packet_type *pt, struct net_device *orig_dev)
{

    rule_t *rp = &ruletbl[0];
    rule_cmd_t cmd = RULE_CMD_NONE;

    // Run all rules on frame until drop/forward is returned
    while( rp->func && (cmd = rp->func( rp->id, rp->cmd, skb->data, rp->val )) == RULE_CMD_NONE )
    {
        rp++;
    } // end while

    // Check if forward command was given
    if( cmd == RULE_CMD_FORWARD )
    {
        printk(KERN_INFO "Forwarding...\n");
        ksendto(sockfd, skb->data, skb->len, 0, (const struct sockaddr *)&addr, sizeof(addr));
    } // end if

    // Free frame
    kfree_skb(skb);

    // Return success
    return(0);
}

static struct packet_type rtap_pt =
{
    .type = __constant_htons(ETH_P_ALL),
    .func = rtap_func,
};

static int __init rtap_init(void)
{

    if (!dev)
    {
        printk(KERN_ERR "Error: missing remote host IP\n");
        printk(KERN_ERR "Usage: insmod rtap.ko dev=devname ip=x.x.x.x port=rport\n\n");
        return -ENXIO;
    } // end if

    if (!ip)
    {
        printk(KERN_ERR "Error: missing remote host IP\n");
        printk(KERN_ERR "Usage: insmod rtap.ko dev=devname ip=x.x.x.x port=rport\n\n");
        return -ENXIO;
    } // end if

    printk( KERN_INFO "rtap.ko: %s, %s, %d\n", dev, ip, port);

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);;
    sockfd = ksocket(AF_INET, SOCK_DGRAM, 0);
    printk(KERN_INFO "sockfd = 0x%p\n", sockfd);
    if (sockfd == NULL)
    {
        printk(KERN_ERR "Cannot create socket\n");
        return -1;
    } // end if

    netdev = first_net_device(&init_net);
    while (netdev)
    {
        printk(KERN_INFO "found [%s]\n", netdev->name);
        if(!strcmp(netdev->name, dev))
        {
            printk("Using device %s!\n", dev);
            break;
        } // end if
        netdev = next_net_device(netdev);
    } // end while

    if (!netdev)
    {
        printk("Did not find device %s!\n", dev);
        return -1;
    } // end if

    rtap_pt.dev = netdev;
    dev_add_pack(&rtap_pt);

    printk(KERN_INFO "RTAP: Driver registered\n" );

    /* Return ok */
    return( 0 );

}

static void __exit rtap_exit(void)
{
    printk( KERN_INFO "RTAP: Unloading module...\n" );
    kclose(sockfd);
    dev_remove_pack(&rtap_pt);
    printk( KERN_INFO "...done.\n" );
    return;
}

module_init(rtap_init);
module_exit(rtap_exit);

