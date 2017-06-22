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

#include <linux/types.h>
#include <linux/module.h>
#include <linux/netdevice.h>
#include <linux/list.h>
#include <linux/kthread.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>

#include "filter.h"
#include "device.h"

//*****************************************************************************
// Type definitions
//*****************************************************************************

struct rtap_device
{
    struct list_head list;
    spinlock_t lock;
    struct packet_type pt;
    struct kthread_worker kworker;
    struct task_struct* kworker_task;
    u8 wrk_count;
    u8 wrk_highwater;
    u16 wrk_drop;
    u32 pkts;
    u32 bytes;
};

struct rtap_device_kwork
{
  struct kthread_work kwork;
  struct net_device *dev;
  struct packet_type *pt;
  struct sk_buff* skb;
  u32 pkts;
  u32 bytes;
};

struct rtap_device_skbmeta
{
  u32 magic; // 'RTAP'
  u8 ver; // Metadata header version; currently 0x01
  u8 hdrlen; // Metadata header length; currently 0x20
  u8 ethaddr[ETH_ALEN]; // Listening device address
  u32 pktid; // Cumulative packet count
  u32 len; // Packet length including metadata header
  u32 bytecnt; // Cumulative byte count as seen by listening device
  u32 secs; // Time packet was received by listening device
  u32 nsecs;
};

#define to_rtap_device(p,e)  ((container_of((p), struct rtap_device, e)))
#define to_rtap_device_kwork(p,e)  ((container_of((p), struct rtap_device_kwork, e)))

#define RTAP_MAGIC              0x52544150 // 'RTAP'
#define RTAP_VER                0x01 // 0.1
#define RTAP_DEVICE_KWORK_MAX   0x10

struct rtap_device_kwork_tbl
{
  u32 freelist_index;
  struct rtap_device_kwork* freelist[RTAP_DEVICE_KWORK_MAX];
  struct rtap_device_kwork data[RTAP_DEVICE_KWORK_MAX];
};

//*****************************************************************************
// Variables
//*****************************************************************************

/* Global */

/* Local */

static struct rtap_device rtap_devices = { { 0 } };
static struct rtap_device_kwork_tbl rtap_kwork_tbl;

//*****************************************************************************
// Local Functions
//*****************************************************************************

static struct rtap_device*
rtap_device_findbyname(const char* devname);

/******************************************************************************
 *
 ******************************************************************************/


/******************************************************************************
 *
 ******************************************************************************/
static void
skb_display(struct sk_buff* skb)
{
  if (skb)
  {
    int size = (skb->len < 32) ? skb->len : 32;
    unsigned char* p = skb->data;
    printk( KERN_INFO "RTAP: Packet info: \n");
    printk( KERN_INFO "RTAP: \tPrev: %p\n", skb->prev);
    printk( KERN_INFO "RTAP: \tNext: %p\n", skb->next);
    if (skb->dev && skb->dev->name)
    {
      printk( KERN_INFO "RTAP: \tDevice: %s\n", skb->dev->name);
      printk( KERN_INFO "RTAP: \tAddress: %02x:%02x:%02x:%02x:%02x:%02x\n",
          skb->dev->perm_addr[0], skb->dev->perm_addr[1], skb->dev->perm_addr[2],
          skb->dev->perm_addr[3], skb->dev->perm_addr[4], skb->dev->perm_addr[5]);
    }
    printk( KERN_INFO "RTAP: \tTime: %lld\n", ktime_to_ns(skb->tstamp));
    printk( KERN_INFO "RTAP: \tType: %hhu\n", skb->pkt_type);
    printk( KERN_INFO "RTAP: \tLength: %u / %u / %u\n", skb->len, skb->data_len, skb->truesize);
    printk( KERN_INFO "RTAP: \tCloned: %hhu\n", skb->cloned);
    printk( KERN_INFO "RTAP: \tUsers: %d\n", skb->users.counter);
    printk( KERN_INFO "RTAP: \tHeadroom: %d\n", skb_headroom(skb));
    printk( KERN_INFO "RTAP: \tTailroom: %d\n", skb_tailroom(skb));
    while (size >= 8)
    {
      printk( KERN_INFO "RTAP: \tData: %02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x\n",
          p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[7]);
      p += 8;
      size -= 8;
    }
  }
}

/******************************************************************************
 *
 ******************************************************************************/
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

/******************************************************************************
 *
 ******************************************************************************/
static struct rtap_device_kwork*
rtap_device_kwork_alloc(void)
{
  struct rtap_device_kwork* wrk = NULL;

  spin_lock(&rtap_devices.lock);
  if (rtap_kwork_tbl.freelist_index)
  {
    rtap_kwork_tbl.freelist_index--;
    //  printk( KERN_INFO "RTAP: Allocating kwork: %u\n", rtap_kwork_tbl.freelist_index);
    wrk = rtap_kwork_tbl.freelist[rtap_kwork_tbl.freelist_index];
    rtap_kwork_tbl.freelist[rtap_kwork_tbl.freelist_index] = NULL;
  }
  spin_unlock(&rtap_devices.lock);

  return(wrk);
}

/******************************************************************************
 *
 ******************************************************************************/
static void
rtap_device_kwork_free(struct rtap_device_kwork* wrk)
{
  if (wrk)
  {
    // Return work back to free list
    spin_lock(&rtap_devices.lock);
//    printk( KERN_INFO "RTAP: Freeing kwork: %u\n", rtap_kwork_tbl.freelist_index);
    rtap_kwork_tbl.freelist[rtap_kwork_tbl.freelist_index] = wrk;
    rtap_kwork_tbl.freelist_index++;
    spin_unlock(&rtap_devices.lock);
  }
}

/******************************************************************************
 *
 ******************************************************************************/
static void
rtap_device_rx_worker(struct kthread_work* work)
{

  if (work)
  {
    struct sk_buff* nskb = NULL;
    struct rtap_device_skbmeta* skbmeta = NULL;
    struct timespec ts = { 0 };
    struct rtap_device_kwork* wrk = NULL;
    wrk = to_rtap_device_kwork(work, kwork);

//    printk( KERN_INFO "RTAP:\n");
//    printk( KERN_INFO "RTAP: Received packet on device: %s\n", wrk->dev->name);
//    skb_display(wrk->skb);

    // Convert sk_buff ktime_t timestamp
    ts = ktime_to_timespec(wrk->skb->tstamp);

    // Create copy of socket buffer while adding headroom for metadata
    nskb = skb_copy_expand(wrk->skb, sizeof(struct rtap_device_skbmeta), 0, GFP_ATOMIC);
    skbmeta = (struct rtap_device_skbmeta* )skb_push(nskb, sizeof(struct rtap_device_skbmeta));

    // Populate metadata header
    skbmeta->magic = cpu_to_be32(RTAP_MAGIC);
    skbmeta->ver = RTAP_VER;
    skbmeta->hdrlen = sizeof(struct rtap_device_skbmeta);
    memcpy(&skbmeta->ethaddr, &wrk->dev->perm_addr, ETH_ALEN);
    skbmeta->pktid = cpu_to_be32(wrk->pkts);
    skbmeta->len = cpu_to_be32(nskb->len);
    skbmeta->bytecnt = cpu_to_be32(wrk->bytes);
    skbmeta->secs = cpu_to_be32(ts.tv_sec);
    skbmeta->nsecs = cpu_to_be32(ts.tv_nsec);

    if (nskb->next || nskb->prev || nskb->data_len)
    {
      // Flag this as a possible loss of packet data
      printk( KERN_WARNING "RTAP: Possible data loss");
      skb_display(nskb);
    }

    // Forward packet to filter
    rtap_filter_recv(nskb);

    // Free frame
    kfree_skb(nskb);
    kfree_skb(wrk->skb);

    // Return work back to free list
    rtap_device_kwork_free(wrk);

  }

  return;
}

/******************************************************************************
 *
 ******************************************************************************/
//static void
//rtap_device_tx_worker(struct kthread_work* work)
//{
//  struct rtap_device_kwork* wrk = NULL;
//  printk( KERN_INFO "RTAP: rtap_device_tx_worker()\n");
//  if (work)
//  {
//    wrk = to_rtap_device_kwork(work, kwork);
//  }
//  return;
//}

/******************************************************************************
 *
 ******************************************************************************/
static int
rtap_device_recv(struct sk_buff *skb, struct net_device *dev,
    struct packet_type *pt, struct net_device *orig_dev)
{

  int ret = 0;
  struct rtap_device* d = rtap_device_findbyname(dev->name);

  if (d)
  {
    struct rtap_device_kwork* wrk = rtap_device_kwork_alloc();

    // Lock device list
    spin_lock(&rtap_devices.lock);

    if (wrk)
    {
      // Initialize work structure
      memset((void *) wrk, 0, sizeof(struct rtap_device_kwork));

      // Update device pkt counters
      d->pkts++;
      d->bytes += skb->len;

      // Initialize work structure
      init_kthread_work(&wrk->kwork, rtap_device_rx_worker);
      wrk->dev = dev;
      wrk->pt = pt;
      wrk->skb = skb;
      wrk->pkts = d->pkts;
      wrk->bytes = d->bytes;

      // Queue work
      queue_kthread_work(&d->kworker, &wrk->kwork);
    }
    else
    {
      ret = -1;
      d->wrk_drop++;
    }

    // Unlock device list
    spin_unlock(&rtap_devices.lock);
  }
  else
  {
    printk( KERN_WARNING "RTAP: Cannot find device: %s\n", dev->name);
    ret = -1;
  }

  // Return success
  return (ret);
}

/******************************************************************************
 *
 ******************************************************************************/
static int
rtap_device_remove(const char *devname)
{
  struct rtap_device *dev = 0;
  struct rtap_device *tmp = 0;
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
      kthread_stop(dev->kworker_task);
      kfree( dev );
      ret = 0;
      break;
    } // end if
  } // end loop
  spin_unlock(&rtap_devices.lock);

  // Return non-null network device pointer on success; null on error
  return (ret);
}

/******************************************************************************
 *
 ******************************************************************************/
static struct net_device *
rtap_device_add(const char *devname)
{
  struct rtap_device *dev = 0;
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
  dev = kmalloc(sizeof(struct rtap_device), GFP_ATOMIC);
  if (!dev)
  {
    printk( KERN_CRIT "RTAP: Cannot allocate memory: dev[%s]\n", devname);
    return (0);
  } // end if
  memset((void *) dev, 0, sizeof(struct rtap_device));

  // Populate device list item
  dev->pt.dev = netdev;
  dev->pt.type = htons(ETH_P_ALL);
  dev->pt.func = rtap_device_recv;

  // Initialize workers
  init_kthread_worker(&dev->kworker);
  dev->kworker_task = kthread_run(kthread_worker_fn, &dev->kworker, "rtap-%s", devname);

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

/******************************************************************************
 *
 ******************************************************************************/
static int
rtap_device_clear(void)
{
  struct rtap_device *dev = NULL;
  struct rtap_device *tmp = NULL;

  // Remove all devices from list
  spin_lock(&rtap_devices.lock);
  list_for_each_entry_safe(dev, tmp, &rtap_devices.list, list)
  {
    printk( KERN_INFO "RTAP: Removing device: %s\n", dev->pt.dev->name );
    dev_remove_pack( &dev->pt );
    list_del( &dev->list );
    kthread_stop(dev->kworker_task);
    kfree( dev );
  } // end loop
  spin_unlock(&rtap_devices.lock);

  return (0);
}

/******************************************************************************
 *
 ******************************************************************************/
static struct rtap_device*
rtap_device_findbyname(const char* devname)
{
  struct rtap_device* dev = NULL;
  struct rtap_device *d = NULL;

  spin_lock(&rtap_devices.lock);
  list_for_each_entry(d, &rtap_devices.list, list)
  {
    if(! (strcmp(d->pt.dev->name, devname)) )
    {
      dev = d;
      break;
    }
  } // end loop
  spin_unlock(&rtap_devices.lock);

  return(dev);
}

/******************************************************************************
 *
 ******************************************************************************/
int
rtap_device_init(void)
{
  int i;
  spin_lock_init(&rtap_devices.lock);
  INIT_LIST_HEAD(&rtap_devices.list);
  rtap_kwork_tbl.freelist_index = 0;
  for (i = 0; i < RTAP_DEVICE_KWORK_MAX; i++)
  {
    rtap_kwork_tbl.freelist[i] = &rtap_kwork_tbl.data[i];
    rtap_kwork_tbl.freelist_index++;
  }
  return (0);
}

/******************************************************************************
 *
 ******************************************************************************/
int
rtap_device_exit(void)
{
  return (rtap_device_clear());
}

/******************************************************************************
 *
 ******************************************************************************/
static int
proc_show(struct seq_file *file, void *arg)
{
  struct rtap_device *dev = 0;
  struct rtap_device *tmp = 0;

  // Iterate over all devices in list
  spin_lock(&rtap_devices.lock);
  list_for_each_entry_safe(dev, tmp, &rtap_devices.list, list)
  {
    seq_printf( file, "dev[%s]\tpkts[%u]\tbytes[%u]\tdropped[%u]\n",
        dev->pt.dev->name, dev->pkts, dev->bytes, dev->wrk_drop );
  } // end loop
  spin_unlock(&rtap_devices.lock);

  return (0);
}

/******************************************************************************
 *
 ******************************************************************************/
static int
proc_open(struct inode *inode, struct file *file)
{
  return (single_open(file, proc_show, NULL));
}

/******************************************************************************
 *
 ******************************************************************************/
static int
proc_close(struct inode *inode, struct file *file)
{
  return (single_release(inode, file));
}

/******************************************************************************
 *
 ******************************************************************************/
static ssize_t
proc_read(struct file *file, char __user *buf, size_t cnt, loff_t *off)
{
  return (seq_read(file, buf, cnt, off));
}

/******************************************************************************
 *
 ******************************************************************************/
static loff_t
proc_lseek(struct file *file, loff_t off, int cnt)
{
  return (seq_lseek(file, off, cnt));
}

/******************************************************************************
 *
 ******************************************************************************/
static ssize_t
proc_write(struct file *file, const char __user *buf, size_t cnt, loff_t *off)
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

/******************************************************************************
 *
 ******************************************************************************/
const struct file_operations rtap_device_fops =
{
    .owner      = THIS_MODULE,
    .open       = proc_open,
    .release    = proc_close,
    .read       = proc_read,
    .llseek     = proc_lseek,
    .write      = proc_write,
};

