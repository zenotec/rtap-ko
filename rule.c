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
//    File: rtap_rule.c
//    Description: TODO: Replace temp rtap_rules with those configurable from
//                 user space.
//
//*****************************************************************************

//*****************************************************************************
// Includes
//*****************************************************************************

#include <linux/module.h>
#include <linux/list.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/slab.h>
#include <linux/uaccess.h>

#include "listener.h"
#include "stats.h"
#include "rule.h"

//*****************************************************************************
// Type definitions
//*****************************************************************************

typedef int
(*rtap_rule_action_func)(struct rtap_rule* r, struct sk_buff *skb);

typedef struct rtap_rule
{
  struct list_head list;
  spinlock_t lock;
  rtap_rule_id_t rid;
  rtap_rule_action_t aid;
  rtap_rule_action_func func;
  union
  {
    struct rtap_listener* l;
    struct rtap_stats* s;
  } arg;
} rtap_rule_t;

//*****************************************************************************
// Global variables
//*****************************************************************************

/* Global */

/* Local */

static struct rtap_rule rtap_rules = { { 0 } };

//*****************************************************************************
// Local Functions
//*****************************************************************************

/******************************************************************************
 *
******************************************************************************/
static void
rtap_rule_destroy(struct rtap_rule* r)
{
  if (r)
  {
    kfree(r);
  }
}

/******************************************************************************
 *
******************************************************************************/
static struct rtap_rule*
rtap_rule_create(void)
{

  rtap_rule_t* r = 0;

  // Allocate new rule list item
  r = kmalloc(sizeof(rtap_rule_t), GFP_KERNEL);
  if (!r)
  {
    printk( KERN_CRIT "RTAP: Cannot allocate memory\n");
    return (NULL);
  } // end if
  memset((void *) r, 0, sizeof(rtap_rule_t));

  return (r);

}

/******************************************************************************
 *
******************************************************************************/
static int
rtap_rule_remove(rtap_rule_t* r)
{
  if (r)
  {
    printk( KERN_INFO "RTAP: Removing rule: %u\n", r->rid);
    list_del(&r->list);
    rtap_rule_destroy(r);
  }
  // Return NULL on success; negative on error
  return (0);
}

/******************************************************************************
 *
******************************************************************************/
static int
rtap_rule_add(rtap_rule_t* r)
{

  if (r)
  {
    // Remove any existing matching listener
    rtap_rule_remove(rtap_rule_findbyid(r->rid));

    printk( KERN_INFO "RTAP: Adding rule: %u\n", r->rid);

    // Add device list item to tail of device list
    spin_lock(&rtap_rules.lock);
    list_add_tail(&r->list, &rtap_rules.list);
    spin_unlock(&rtap_rules.lock);
  }
  // Return NULL on success; negative on error
  return (0);
}

/******************************************************************************
 *
******************************************************************************/
static int
rtap_rule_clear(void)
{

  rtap_rule_t *r = NULL;
  rtap_rule_t *tmp = NULL;

  // Remove all rules from list
  spin_lock(&rtap_rules.lock);
  list_for_each_entry_safe(r, tmp, &rtap_rules.list, list)
  {
    rtap_rule_remove(r);
  } // end loop
  spin_unlock(&rtap_rules.lock);

  // Return NULL on success; negative on error
  return (0);
}

//*****************************************************************************

static int
rtap_rule_action_none(struct rtap_rule* r, struct sk_buff *skb)
{
  if (!r || r->aid != ACTION_NONE)
  {
    return (-1);
  }
  // Return NULL on success; negative on error
  return (0);
}

//*****************************************************************************

static int
rtap_rule_action_drop(struct rtap_rule* r, struct sk_buff *skb)
{
  if (!r || r->aid != ACTION_DROP)
  {
    return (-1);
  }
  // Return NULL on success; negative on error
  return (0);
}

//*****************************************************************************

static int
rtap_rule_action_forward(struct rtap_rule* r, struct sk_buff *skb)
{

  if (!r || r->aid != ACTION_FWRD)
  {
    return (-1);
  }

  printk( KERN_INFO "RTAP: Forward action rule\n");

  listener_send(r->arg.l, skb);

  // Return NULL on success; negative on error
  return (0);
}

//*****************************************************************************

static int
rtap_rule_action_count(struct rtap_rule* r, struct sk_buff *skb)
{
  if (!r || r->aid != ACTION_CNT)
  {
    return (-1);
  }

  // Return NULL on success; negative on error
  return (0);
}

//*****************************************************************************
// Global Functions
//*****************************************************************************

/******************************************************************************
 *
******************************************************************************/
int
rtap_rule_init(void)
{
  spin_lock_init(&rtap_rules.lock);
  INIT_LIST_HEAD(&rtap_rules.list);
  return (0);
}

/******************************************************************************
 *
******************************************************************************/
int
rtap_rule_exit(void)
{
  return (rtap_rule_clear());
}

/******************************************************************************
 *
******************************************************************************/
rtap_rule_id_t
rtap_rule_get_id(rtap_rule_t* r)
{
  rtap_rule_id_t id = 0;
  if (r)
  {
    id = r->rid;
  }
  return (id);
}

/******************************************************************************
 *
******************************************************************************/
int
rtap_rule_set_id(rtap_rule_t* r, rtap_rule_id_t id)
{
  int ret = -1;
  if (r && id)
  {
    r->rid = id;
    ret = 0;
  }
  return (ret);
}

/******************************************************************************
 *
******************************************************************************/
rtap_rule_action_t
rtap_rule_get_action(rtap_rule_t* r)
{
  rtap_rule_action_t action = 0;
  if (r)
  {
    action = r->aid;
  }
  return (action);
}

/******************************************************************************
 *
******************************************************************************/
int
rtap_rule_set_action(rtap_rule_t* r, rtap_rule_action_t aid, void* arg)
{
  int ret = -1;
  if (r && aid)
  {
    r->aid = aid;
    switch (aid)
    {
    case ACTION_NONE:
      r->func = rtap_rule_action_none;
      ret = 0;
      break;
    case ACTION_DROP:
      r->func = rtap_rule_action_drop;
      ret = 0;
      break;
    case ACTION_FWRD:
      r->func = rtap_rule_action_forward;
      r->arg.l = (void*) listener_findbyid(*(unsigned int*) arg);
      if (r->arg.l)
      {
        ret = 0;
      }
      else
      {
        printk( KERN_ERR "RTAP: Cannot find listener identifier: %u\n", *(unsigned int*) arg);
      }
      break;
    case ACTION_CNT:
      r->func = rtap_rule_action_count;
      ret = 0;
      break;
    default:
      break;
    }
  }
  return (ret);
}

/******************************************************************************
 *
******************************************************************************/
struct rtap_rule*
rtap_rule_findbyid(const rtap_rule_id_t rid)
{
  struct rtap_rule *ret = 0;
  struct rtap_rule *r = 0;
  struct rtap_rule *tmp = 0;

  // Search for filter id in list and remove
  spin_lock(&rtap_rules.lock);
  list_for_each_entry_safe(r, tmp, &rtap_rules.list, list)
  {
    if( r->rid == rid )
    {
      ret = r;
      break;
    } // end if
  } // end loop
  spin_unlock(&rtap_rules.lock);

  // Return non-null on success; null on error
  return (ret);
}


//*****************************************************************************
// Proc Filesystem Functions
//*****************************************************************************

/******************************************************************************
 *
******************************************************************************/
static const char *
rtap_rule_action_str(struct rtap_rule* r)
{
  const char *str = 0;
  switch (r->aid)
  {
  case ACTION_NONE:
    str = "None";
    break;
  case ACTION_DROP:
    str = "Drop";
    break;
  case ACTION_FWRD:
    str = "Forward";
    break;
  case ACTION_CNT:
    str = "Count";
    break;
  default:
    str = "Unknown";
    break;
  }
  return (str);
}

/******************************************************************************
 *
******************************************************************************/
static const char *
rtap_rule_arg_str(struct rtap_rule* r, char* str, size_t len)
{
  switch (r->aid)
  {
  case ACTION_NONE:
  case ACTION_DROP:
    break;
  case ACTION_FWRD:
    snprintf(str, len, " -> %s:%hu", listener_get_ipaddr(r->arg.l),
        listener_get_port(r->arg.l));
    break;
  case ACTION_CNT:
    break;
  default:
    strncpy(str, "Unknown", len);
    break;
  }
  return (str);
}

/******************************************************************************
 *
******************************************************************************/
static int
proc_show(struct seq_file *file, void *arg)
{
  struct rtap_rule *r = 0;
  struct rtap_rule *tmp = 0;

  // Print header
  seq_printf(file, "------------------------------------------------\n");
  seq_printf(file, "|  Id |    Action   |      Argument            |\n");
  seq_printf(file, "|----------------------------------------------|\n");

  // Iterate over all filter stats in list
  spin_lock(&rtap_rules.lock);
  list_for_each_entry_safe(r, tmp, &rtap_rules.list, list)
  {
    char arg_str[24 + 1] = { 0 };
    rtap_rule_arg_str( r, arg_str, sizeof(arg_str) );
    seq_printf( file, "| %3d | %11s | %-24s |\n",
        r->rid, rtap_rule_action_str( r ), arg_str );
  } // end loop
  spin_unlock(&rtap_rules.lock);

  seq_printf(file, "------------------------------------------------\n");

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
  char cmdstr[256 + 1] = { 0 };
  unsigned int rid = 0;
  unsigned int aid = 0;
  unsigned int arg = 0;
  int ret = 0;

  cnt = (cnt >= 256) ? 256 : cnt;
  copy_from_user(cmdstr, buf, cnt);
  ret = sscanf(cmdstr, "%u %u %u", &rid, &aid, &arg);

  if ((ret == 0) && (strlen(cmdstr) == 1) && (cmdstr[0] == '-'))
  {
    rtap_rule_clear();
  }
  else if ((ret == 1) && (rid < 0))
  {
    rtap_rule_t* r = rtap_rule_findbyid(-rid);
    rtap_rule_remove(r);
  } // end if
  else if ((ret > 1) && (rid > 0) && (aid > 0))
  {
    rtap_rule_t* r = rtap_rule_create();
    if (rtap_rule_set_id(r, rid) || rtap_rule_set_action(r, aid, (void*)&arg))
    {
      printk( KERN_ERR "RTAP: Invalid arguments\n");
      rtap_rule_destroy(r);
      return(-1);
    }
    rtap_rule_add(r);
  }
  else
  {
    printk( KERN_ERR "RTAP: Failed parsing command string: %s\n", cmdstr);
    return (-1);
  } // end else

  return (cnt);

}

/******************************************************************************
 *
******************************************************************************/
const struct file_operations rtap_rule_fops =
{
    .owner = THIS_MODULE,
    .open = proc_open,
    .release = proc_close,
    .read = proc_read,
    .llseek = proc_lseek,
    .write = proc_write
};

