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
//    File: listener.c
//    Description: 
//
//*****************************************************************************

//*****************************************************************************
// Includes
//*****************************************************************************

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/gfp.h>
#include <linux/list.h>
#include <linux/seq_file.h>
#include <linux/if_ether.h>
#include <linux/net.h>
#include <linux/in.h>
#include <net/sock.h>
#include <linux/byteorder/generic.h>

#include "ksocket.h"
#include "listener.h"

struct rtap_listener
{
  struct list_head list;
  spinlock_t lock;
  rtap_listener_id_t lid;
  char *ipaddr;
  uint16_t port;
  struct sockaddr_in in_addr;
  ksocket_t sockfd;
};

//*****************************************************************************
// Variables
//*****************************************************************************

/* Global */

/* Local */

static struct rtap_listener rtap_listeners = { { 0 } };

//*****************************************************************************
// Local Functions
//*****************************************************************************

/******************************************************************************
 *
******************************************************************************/
static void
listener_destroy(struct rtap_listener* l)
{
  if (l)
  {
    if (l->ipaddr)
    {
      kfree(l->ipaddr);
      l->ipaddr = NULL;
    }
    kfree(l);
  }
}

/******************************************************************************
 *
******************************************************************************/
static struct rtap_listener*
listener_create(void)
{
  struct rtap_listener* l = 0;

  // Allocate new listener list item
  l = kmalloc(sizeof(struct rtap_listener), GFP_KERNEL);
  if (!l)
  {
    printk( KERN_CRIT "RTAP: Cannot allocate memory\n");
    return (NULL);
  } // end if
  memset((void *) l, 0, sizeof(struct rtap_listener));

  // Allocate buffer for IP address
  l->ipaddr = kmalloc(32, GFP_KERNEL);
  if (!l->ipaddr)
  {
    printk( KERN_CRIT "RTAP: Cannot allocate memory\n");
    listener_destroy(l);
    return (NULL);
  } // end if
  memset((void *) l->ipaddr, 0, 32);

  // Setup listener socket
  l->sockfd = ksocket(AF_INET, SOCK_DGRAM, 0);
  if (l->sockfd == NULL)
  {
    printk( KERN_ERR "RTAP: Cannot create listener socket\n" );
    listener_destroy(l);
    return (NULL);
  } // end if
  l->in_addr.sin_family = AF_INET;

  return (l);
}

/******************************************************************************
 *
******************************************************************************/
static int
listener_remove(struct rtap_listener* l)
{
  if (l)
  {
    printk( KERN_INFO "RTAP: Removing listener: %s:%hu\n", l->ipaddr, l->port);
    list_del(&l->list);
    listener_destroy(l);
  }
  return (0);
}

/******************************************************************************
 *
******************************************************************************/
static int
listener_add(struct rtap_listener* l)
{

  if (l)
  {
    // Remove any existing matching listener
    listener_remove(listener_findbyipandport(l->ipaddr, l->port));

    // Add listener to tail of device list
    printk( KERN_INFO "RTAP: Adding listener: %s:%hu\n", l->ipaddr, l->port);
    spin_lock(&rtap_listeners.lock);
    list_add_tail(&l->list, &rtap_listeners.list);
    spin_unlock(&rtap_listeners.lock);
  }

  // Return NULL on success; negative on error
  return (0);
}

/******************************************************************************
 *
******************************************************************************/
static int
listener_clear(void)
{
  struct rtap_listener *l = NULL;
  struct rtap_listener *tmp = NULL;

  // Remove all listeners from list
  spin_lock(&rtap_listeners.lock);
  list_for_each_entry_safe(l, tmp, &rtap_listeners.list, list)
  {
    listener_remove(l);
  } // end loop
  spin_unlock(&rtap_listeners.lock);

  return (0);
}

//*****************************************************************************
// Global Functions
//*****************************************************************************

/******************************************************************************
 *
******************************************************************************/
int
listener_init(void)
{
  spin_lock_init(&rtap_listeners.lock);
  INIT_LIST_HEAD(&rtap_listeners.list);
  return (0);
}

/******************************************************************************
 *
******************************************************************************/
int
listener_exit(void)
{
  return (listener_clear());
}

/******************************************************************************
 *
******************************************************************************/
rtap_listener_id_t
listener_get_id(struct rtap_listener* l)
{
  rtap_listener_id_t id = 0;
  if (l)
  {
    id = l->lid;
  }
  return (id);
}

/******************************************************************************
 *
******************************************************************************/
int
listener_set_id(struct rtap_listener* l, rtap_listener_id_t id)
{
  int ret = -1;
  if (l && id)
  {
    l->lid = id;
    ret = 0;
  }
  return (ret);
}

/******************************************************************************
 *
******************************************************************************/
const char*
listener_get_ipaddr(struct rtap_listener* l)
{
  const char* ipaddr = NULL;
  if (l)
  {
    ipaddr = l->ipaddr;
  }
  return (ipaddr);
}

/******************************************************************************
 *
******************************************************************************/
int
listener_set_ipaddr(struct rtap_listener* l, const char* addr)
{
  int ret = -1;
  if (l && addr)
  {
    if (inet_aton(addr, &l->in_addr.sin_addr) )
    {
    strcpy(l->ipaddr, addr);
    ret = 0;
    }
  }
  return (ret);
}

/******************************************************************************
 *
******************************************************************************/
uint16_t
listener_get_port(struct rtap_listener* l)
{
  uint16_t port = 0;
  if (l)
  {
    port = l->port;
  }
  return (port);
}

/******************************************************************************
 *
******************************************************************************/
int
listener_set_port(struct rtap_listener* l, uint16_t port)
{
  int ret = -1;
  if (l && port)
  {
    l->in_addr.sin_port = htons(port);
    l->port = port;
    ret = 0;
  }
  return (ret);
}

/******************************************************************************
 *
******************************************************************************/
struct rtap_listener*
listener_findbyid(rtap_listener_id_t lid)
{

  struct rtap_listener *listener = NULL;
  struct rtap_listener *l = NULL;
  struct rtap_listener *tmp = NULL;

  // Search for listener in list and remove
  spin_lock(&rtap_listeners.lock);
  list_for_each_entry_safe(l, tmp, &rtap_listeners.list, list)
  {
    if (l->lid == lid)
    {
      printk( KERN_INFO "RTAP: Found listener: %s:%hu\n", l->ipaddr, l->port );
      listener = l;
      break;
    } // end if
  } // end loop
  spin_unlock(&rtap_listeners.lock);

  return (listener);

}

/******************************************************************************
 *
******************************************************************************/
struct rtap_listener*
listener_findbyipandport(const char* addr, uint16_t port)
{

  struct rtap_listener *listener = NULL;
  struct rtap_listener *l = NULL;
  struct rtap_listener *tmp = NULL;

  // Search for listener in list and remove
  if (addr && port)
  {
    spin_lock(&rtap_listeners.lock);
    list_for_each_entry_safe(l, tmp, &rtap_listeners.list, list)
    {
      if (!(strcmp(l->ipaddr, addr)) && (l->port == port))
      {
        printk( KERN_INFO "RTAP: Found listener: %s:%hu\n", l->ipaddr, l->port );
        listener = l;
        break;
      } // end if
    } // end loop
    spin_unlock(&rtap_listeners.lock);
  }

  return (listener);

}

/******************************************************************************
 *
******************************************************************************/
int
listener_send(struct rtap_listener* l, struct sk_buff* skb)
{
  int ret = 0;
  if (l && skb)
  {
//    printk( KERN_INFO "RTAP: Sending to listener: %s:%hu\n", l->ipaddr, l->port);
    ret = ksendto(l->sockfd, skb->data, skb->len, 0,
        (const struct sockaddr *) &l->in_addr, sizeof(l->in_addr));
  }
  return (ret);
}

//*****************************************************************************
// Proc Filesystem Functions
//*****************************************************************************

/******************************************************************************
 *
******************************************************************************/
static int
proc_show(struct seq_file *file, void *arg)
{
  struct rtap_listener *listener = NULL;
  struct rtap_listener *tmp = NULL;

  // Iterate over all listeners in list
  spin_lock(&rtap_listeners.lock);
  list_for_each_entry_safe(listener, tmp, &rtap_listeners.list, list)
  {
    seq_printf(file, "[%u] %s:%hu\n", listener->lid, listener->ipaddr, listener->port);
  } // end loop
  spin_unlock(&rtap_listeners.lock);

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
proc_read( struct file *file, char __user *buf, size_t cnt, loff_t *off )
{
  return( seq_read( file, buf, cnt, off ) );
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
proc_write( struct file *file, const char __user *buf, size_t cnt, loff_t *off )
{
  char cmdstr[256+1] =  { 0 };
  int lid = 0;
  char ipaddr[256+1] = { 0 };
  short port = 8888;
  int ret = 0;

  cnt = (cnt >= 256) ? 256 : cnt;
  copy_from_user( cmdstr, buf, cnt );
  ret = sscanf( cmdstr, "%d %s %hu", &lid, ipaddr, &port );

  printk( KERN_INFO "RTAP: ID: %d, IP: %s, PORT: %hu\n", lid, ipaddr, port);

  if ((ret == 0) && (strlen(cmdstr) == 1) && (cmdstr[0] == '-'))
  {
    listener_clear();
  }
  else if ((ret == 1) && (lid < 0))
  {
    struct rtap_listener* l = listener_findbyid(-lid);
    listener_remove(l);
  } // end if
  else if( (ret >= 2) && (strlen(ipaddr) > 1) )
  {
    struct rtap_listener* l = listener_create();
    if (listener_set_id(l, lid) || listener_set_ipaddr(l, ipaddr) ||
        listener_set_port(l, port))
    {
      printk( KERN_ERR "RTAP: Invalid arguments\n");
      listener_destroy(l);
      return(-1);
    }
    listener_add(l);
  } // end else if
  else
  {
    printk( KERN_ERR "RTAP: Failed parsing IP string: %s\n", cmdstr );
    return( -1 );
  } // end else

  return( cnt );
}

/******************************************************************************
 *
******************************************************************************/
const struct file_operations listener_fops =
{
    .owner = THIS_MODULE,
    .open = proc_open,
    .release = proc_close,
    .read = proc_read,
    .llseek = proc_lseek,
    .write = proc_write
};

