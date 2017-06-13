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
//    51 Franklin Street, Fifth Floor, Boston, MA 021100301 USA.
//
//    File: device.c
//    Description: 
//
//*****************************************************************************

//*****************************************************************************
// Includes
//*****************************************************************************

#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/list.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include "filter.h"
#include "device.h"

//*****************************************************************************
// Type definitions
//*****************************************************************************

typedef struct rtap_device
{
    struct list_head list;
    spinlock_t lock;
    struct packet_type pt;
} rtap_device_t;

//*****************************************************************************
// Variables
//*****************************************************************************

/* Global */

/* Local */

static rtap_device_t rtap_devices = { { 0 } };

//*****************************************************************************
// Functions
//*****************************************************************************

static void
skb_display(struct sk_buff* skb)
{
  if (skb)
  {
    int size = (skb->len < 32) ? skb->len : 32;
    unsigned char* p = skb->data;
    printk( KERN_INFO "RTAP: Packet info: \n");
    if (skb->dev && skb->dev->name)
      printk( KERN_INFO "RTAP: \tDevice: %s\n", skb->dev->name);
    printk( KERN_INFO "RTAP: \tTime: %lld\n", ktime_to_us(skb->tstamp));
    printk( KERN_INFO "RTAP: \tLength: %u (%u)\n", skb->len, skb->data_len);
    printk( KERN_INFO "RTAP: \tCloned: %hhu\n", skb->cloned);
    printk( KERN_INFO "RTAP: \tUsers: %d\n", skb->users.counter);
    while (size >= 8)
    {
      printk( KERN_INFO "RTAP: \tData: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n",
          p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]);
      p += 8;
      size -= 8;
    }
  }
}

//*****************************************************************************

static struct net_device *
get_devbyname(const char *devname)
{

  struct net_device *netdev = first_net_device(&init_net);
  while (netdev)
  {
    if (!strcmp(netdev->name, devname))
    {
      printk( KERN_INFO "RTAP: Found device: %s\n", netdev->name);
      break;
    } // end if
    netdev = next_net_device(netdev);
  } // end while

  return (netdev);

}

//*****************************************************************************

static int
rtap_device_recv(struct sk_buff *skb, struct net_device *dev,
    struct packet_type *pt, struct net_device *orig_dev)
{

  printk( KERN_INFO "RTAP:\n");
  printk( KERN_INFO "RTAP: Received packet on device: %s (%s)\n", dev->name, orig_dev->name);
  skb_display(skb);

  // Pass to filters
  rtap_filter_recv(skb);

  // Free frame
  kfree_skb(skb);

  // Return success
  return (0);
}

//*****************************************************************************

static int
rtap_device_remove(const char *devname)
{
  rtap_device_t *dev = 0;
  rtap_device_t *tmp = 0;
  int ret = -1;

  // Search for device in list and remove
  spin_lock(&rtap_devices.lock);
  list_for_each_entry_safe(dev, tmp, &rtap_devices.list, list)
  {
    if( ! strcmp( dev->pt.dev->name, devname ) )
    {
      printk( KERN_INFO "RTAP: Removing device: %s\n", dev->pt.dev->name );
      dev_remove_pack( &dev->pt );
      list_del( &dev->list );
      kfree( dev );
      ret = 0;
      break;
    } // end if
  } // end loop
  spin_unlock(&rtap_devices.lock);

  // Return non-null network device pointer on success; null on error
  return (ret);
}

//*****************************************************************************

static struct net_device *
rtap_device_add(const char *devname)
{
  rtap_device_t *dev = 0;
  struct net_device *netdev = 0;

  // First remove any existing devices with same name
  rtap_device_remove(devname);

  // Lookup network device by given name
  netdev = get_devbyname(devname);
  if (!netdev)
  {
    printk( KERN_ERR "RTAP: Device '%s' not found\n", devname);
    return (0);
  } // end if

  // Allocate new device list item
  dev = kmalloc(sizeof(rtap_device_t), GFP_ATOMIC);
  if (!dev)
  {
    printk( KERN_CRIT "RTAP: Cannot allocate memory: dev[%s]\n", devname);
    return (0);
  } // end if
  memset((void *) dev, 0, sizeof(rtap_device_t));

  // Populate device list item
  dev->pt.dev = netdev;
  dev->pt.type = htons(ETH_P_ALL);
  dev->pt.func = rtap_device_recv;

  // Add device list item to tail of device list
  spin_lock(&rtap_devices.lock);
  list_add_tail(&dev->list, &rtap_devices.list);
  spin_unlock(&rtap_devices.lock);

  // Register for packet
  dev_add_pack(&dev->pt);

  printk( KERN_INFO "RTAP: Added device: %s\n", devname);

  // Return non-null network device pointer on success; null on error
  return (netdev);
}

//*****************************************************************************
static int
rtap_device_clear(void)
{
  rtap_device_t *dev = 0;
  rtap_device_t *tmp = 0;

  // Remove all devices from list
  spin_lock(&rtap_devices.lock);
  list_for_each_entry_safe(dev, tmp, &rtap_devices.list, list)
  {
    printk( KERN_INFO "RTAP: Removing device: %s\n", dev->pt.dev->name );
    dev_remove_pack( &dev->pt );
    list_del( &dev->list );
    kfree( dev );
  } // end loop
  spin_unlock(&rtap_devices.lock);

  return (0);
}

//*****************************************************************************
int
rtap_device_init(void)
{
  spin_lock_init(&rtap_devices.lock);
  INIT_LIST_HEAD(&rtap_devices.list);
  return (0);
}

//*****************************************************************************
int
rtap_device_exit(void)
{
  return (rtap_device_clear());
}

//*****************************************************************************
//*****************************************************************************
static int
rtap_device_show(struct seq_file *file, void *arg)
{
  rtap_device_t *dev = 0;
  rtap_device_t *tmp = 0;

  // Iterate over all devices in list
  spin_lock(&rtap_devices.lock);
  list_for_each_entry_safe(dev, tmp, &rtap_devices.list, list)
  {
    seq_printf( file, "dev[%s]\n", dev->pt.dev->name );
  } // end loop
  spin_unlock(&rtap_devices.lock);

  return (0);
}

//*****************************************************************************
static int
rtap_device_open(struct inode *inode, struct file *file)
{
  return (single_open(file, rtap_device_show, NULL));
}

//*****************************************************************************
static int
rtap_device_close(struct inode *inode, struct file *file)
{
  return (single_release(inode, file));
}

//*****************************************************************************
static ssize_t
rtap_device_read(struct file *file, char __user *buf, size_t cnt, loff_t *off)
{
  return (seq_read(file, buf, cnt, off));
}

//*****************************************************************************
static loff_t
rtap_device_lseek(struct file *file, loff_t off, int cnt)
{
  return (seq_lseek(file, off, cnt));
}

//*****************************************************************************
static ssize_t
rtap_device_write(struct file *file, const char __user *buf, size_t cnt, loff_t *off)
{
  char devstr[256 + 1] = { 0 };
  char devname[256 + 1] = { 0 };
  int ret = 0;

  cnt = (cnt >= 256) ? 256 : cnt;
  copy_from_user(devstr, buf, cnt);
  ret = sscanf(devstr, "%256s", devname);

  if ((ret == 1) && (strlen(devname) == 1) && (devname[0] == '-'))
  {
    rtap_device_clear();
  } // end if
  else if ((ret == 1) && (strlen(devname) > 1))
  {
    if (devname[0] == '-')
    {
      rtap_device_remove(&devname[1]);
    } // end if
    else if (devname[0] == '+')
    {
      rtap_device_add(&devname[1]);
    } // end else
    else
    {
      rtap_device_add(devname);
    } // end else
  } // end else if
  else
  {
    printk( KERN_ERR "RTAP: Failed parsing device string: %s\n", devstr);
    return (-1);
  } // end else

  return (cnt);

}

//*****************************************************************************
//*****************************************************************************
const struct file_operations rtap_device_fops =
{
    .owner      = THIS_MODULE,
    .open       = rtap_device_open,
    .release    = rtap_device_close,
    .read       = rtap_device_read,
    .llseek     = rtap_device_lseek,
    .write      = rtap_device_write,
};

