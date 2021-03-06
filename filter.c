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
//    File: rtap_filter.c
//    Description:  Filters specific to a group of rules.
//
//*****************************************************************************

//*****************************************************************************
// Includes
//*****************************************************************************

#include <linux/string.h>

#include <linux/module.h>
#include <linux/list.h>
#include <linux/proc_fs.h>
#include <linux/seq_file.h>
#include <linux/ieee80211.h>
#include <net/mac80211.h>
#include <net/ieee80211_radiotap.h>

#include "rtap-ko.h"
#include "device.h"
#include "listener.h"
#include "rule.h"
#include "stats.h"
#include "filter.h"

//*****************************************************************************
// Type definitions
//*****************************************************************************

struct rtap_filter
{
  struct list_head list;
  spinlock_t lock;
  rtap_filter_id_t fid;
  rtap_filter_type_t type;
  rtap_filter_subtype_t subtype;
  struct rtap_rule* rule;
  unsigned int count;
  char *arg;
};

struct rtap_chain
{
  struct list_head list;
  spinlock_t lock;
  char* name;
  struct rtap_filter filter;
};

typedef int
(*rtap_filter_func_t)(struct rtap_filter *fp, struct sk_buff *skb);

//*****************************************************************************
// Function prototypes
//*****************************************************************************

static int
rtap_filter_all(struct rtap_filter *f, struct sk_buff *skb);
static int
rtap_filter_radiotap(struct rtap_filter *fp, struct sk_buff *skb);
static int
rtap_filter_80211(struct rtap_filter *fp, struct sk_buff *skb);
static int
rtap_filter_ip(struct rtap_filter *fp, struct sk_buff *skb);

//*****************************************************************************
// Global variables
//*****************************************************************************

/* Global */

/* Local */

static rtap_filter_func_t rtap_filtertbl[] =
{
    [FILTER_TYPE_NONE] = NULL,
    [FILTER_TYPE_ALL] = &rtap_filter_all,
    [FILTER_TYPE_RADIOTAP] = &rtap_filter_radiotap,
    [FILTER_TYPE_80211] = &rtap_filter_80211,
    [FILTER_TYPE_IP] = &rtap_filter_ip,
    [FILTER_TYPE_UDP] = NULL,
    [FILTER_TYPE_TCP] = NULL,
    [FILTER_TYPE_LAST] = NULL
};

static struct rtap_chain rtap_chains = { { 0 } }; // Dynamic rtap_filter chain

//*****************************************************************************
// Local Functions
//*****************************************************************************

/******************************************************************************
 *
 ******************************************************************************/
static void
rtap_filter_destroy(struct rtap_filter* f)
{
  if (f)
  {
    if (f->arg)
    {
      kfree(f->arg);
    }
    kfree(f);
  }
}

/******************************************************************************
 *
 ******************************************************************************/
static struct rtap_filter*
rtap_filter_create(void)
{

  struct rtap_filter* f = 0;

  // Allocate new filter list item
  f = kmalloc(sizeof(struct rtap_filter), GFP_KERNEL);
  if (!f)
  {
    printk( KERN_CRIT "RTAP: Cannot allocate memory\n");
    return (NULL);
  } // end if
  memset((void *) f, 0, sizeof(struct rtap_filter));

  // Allocate buffer for filter argument string
  f->arg = kmalloc(256, GFP_KERNEL);
  if (!f->arg)
  {
    printk( KERN_CRIT "RTAP: Cannot allocate memory\n");
    rtap_filter_destroy(f);
    return (NULL);
  } // end if
  memset((void *) f->arg, 0, 256);

  return (f);

}

/******************************************************************************
 *
 ******************************************************************************/
static int
rtap_filter_remove(struct rtap_chain* c, struct rtap_filter* f)
{
  int ret = -1;
  if (c && f)
  {
    printk( KERN_INFO "RTAP: Removing filter: %u from chain: %s\n", f->fid, c->name);
    list_del(&f->list);
    rtap_filter_destroy(f);
  }
  // Return NULL on success; negative on error
  return (ret);
}

/******************************************************************************
 *
 ******************************************************************************/
static int
rtap_filter_add(struct rtap_chain* c, struct rtap_filter* f)
{
  int ret = -1;
  if (c && f)
  {
    // Remove any existing matching filter

    // Add device list item to tail of device list
    printk( KERN_INFO "RTAP: Adding filter: %s:%hu\n", c->name, f->fid);
    spin_lock(&c->lock);
    list_add_tail(&f->list, &c->filter.list);
    spin_unlock(&c->lock);
    ret = 0;
  }
  // Return NULL on success; negative on error
  return (ret);
}

/******************************************************************************
 *
 ******************************************************************************/
static int
rtap_filter_clear(struct rtap_filter* chain)
{
  int ret = 0;
  return (ret);
}

/******************************************************************************
 *
 ******************************************************************************/
static rtap_filter_id_t
rtap_filter_get_id(struct rtap_filter* f)
{
  rtap_filter_id_t id = 0;
  if (f)
  {
    id = f->fid;
  }
  return (id);
}

/******************************************************************************
 *
 ******************************************************************************/
static int
rtap_filter_set_id(struct rtap_filter* f, rtap_filter_id_t id)
{
  int ret = -1;
  if (f && id)
  {
    f->fid = id;
    ret = 0;
  }
  return(ret);
}

/******************************************************************************
 *
 ******************************************************************************/
static rtap_filter_type_t
rtap_filter_get_type(struct rtap_filter* f)
{
  rtap_filter_type_t type = 0;
  if (f)
  {
    type = f->type;
  }
  return (type);
}

/******************************************************************************
 *
 ******************************************************************************/
static int
rtap_filter_set_type(struct rtap_filter* f, rtap_filter_type_t type)
{
  int ret = -1;
  if (f && type)
  {
    f->type = type;
    ret = 0;
  }
  return(ret);
}

/******************************************************************************
 *
 ******************************************************************************/
static rtap_rule_id_t
rtap_filter_get_rule(struct rtap_filter* f)
{
  rtap_rule_id_t id = 0;
  if (f)
  {
    id = rtap_rule_get_id(f->rule);
  }
  return (id);
}

/******************************************************************************
 *
 ******************************************************************************/
static int
rtap_filter_set_rule(struct rtap_filter* f, rtap_rule_id_t id)
{
  int ret = -1;
  if (f && id)
  {
    f->rule = rtap_rule_findbyid(id);
    ret = 0;
  }
  return(ret);
}

/******************************************************************************
 *
 ******************************************************************************/
static rtap_filter_type_t
rtap_filter_get_subtype(struct rtap_filter* f)
{
  rtap_filter_subtype_t subtype = 0;
  if (f)
  {
    subtype = f->subtype;
  }
  return (subtype);
}

/******************************************************************************
 *
 ******************************************************************************/
static int
rtap_filter_set_subtype(struct rtap_filter* f, rtap_filter_subtype_t subtype)
{
  int ret = -1;
  if (f && subtype)
  {
    f->subtype = subtype;
    ret = 0;
  }
  return(ret);
}

/******************************************************************************
 *
 ******************************************************************************/
static unsigned int
rtap_filter_get_count(struct rtap_filter* f)
{
  unsigned int count = 0;
  if (f)
  {
    count = f->count;
  }
  return (count);
}

/******************************************************************************
 *
 ******************************************************************************/
static int
rtap_filter_set_count(struct rtap_filter* f, unsigned int count)
{
  int ret = -1;
  if (f)
  {
    f->count = count;
    ret = 0;
  }
  return(ret);
}

/******************************************************************************
 *
 ******************************************************************************/
static const char*
rtap_filter_get_arg(struct rtap_filter* f)
{
  const char* arg = 0;
  if (f)
  {
    arg = f->arg;
  }
  return (arg);
}

/******************************************************************************
 *
 ******************************************************************************/
static int
rtap_filter_set_arg(struct rtap_filter* f, const char* arg)
{
  int ret = -1;
  if (f && arg)
  {
    strncpy(f->arg, arg, 256);
    ret = 0;
  }
  return(ret);
}

/******************************************************************************
 *
 ******************************************************************************/
static void
rtap_chain_destroy(struct rtap_chain* c)
{
  if (c)
  {
    if (c->name)
    {
      kfree(c->name);
    }
    kfree(c);
  }
}

/******************************************************************************
 *
 ******************************************************************************/
static struct rtap_chain*
rtap_chain_create(void)
{

  struct rtap_chain* c = 0;

  // Allocate new filter list item
  c = kmalloc(sizeof(struct rtap_chain), GFP_KERNEL);
  if (!c)
  {
    printk( KERN_CRIT "RTAP: Cannot allocate memory\n");
    return (NULL);
  } // end if
  memset((void *) c, 0, sizeof(struct rtap_chain));

  // Allocate buffer for chain name
  c->name = kmalloc(32, GFP_KERNEL);
  if (!c->name)
  {
    printk( KERN_CRIT "RTAP: Cannot allocate memory\n");
    rtap_chain_destroy(c);
    return (NULL);
  } // end if
  memset((void *) c->name, 0, 32);

  // Initialize chain structure
  spin_lock_init(&c->filter.lock);
  INIT_LIST_HEAD(&c->filter.list);

  return (c);

}

/******************************************************************************
 *
 ******************************************************************************/
static const char*
rtap_chain_get_name(struct rtap_chain* c)
{
  const char* name = NULL;
  if (c)
  {
    name = c->name;
  }
  return (name);
}

/******************************************************************************
 *
 ******************************************************************************/
static int
rtap_chain_set_name(struct rtap_chain* c, const char* name)
{
  int ret = -1;
  if (c && name)
  {
    strncpy(c->name, name, 32);
    c->name[31] = 0;
    ret = 0;
  }
  return (ret);
}

/******************************************************************************
 *
 ******************************************************************************/
static struct rtap_chain*
rtap_chain_find(const char* name)
{
  struct rtap_chain* ret = 0;
  struct rtap_chain* chain = 0;
  struct rtap_chain* tmp = 0;

  // Search for specified filter chain in list
  spin_lock(&rtap_chains.lock);
  list_for_each_entry_safe(chain, tmp, &rtap_chains.list, list)
  {
    if( ! strcmp( chain->name, name) )
    {
      ret = chain;
    }
  } // end loop
  spin_unlock(&rtap_chains.lock);

  return (ret);
}

/******************************************************************************
 *
 ******************************************************************************/
static int
rtap_chain_remove(const char* name)
{
  int ret = 0;
  struct rtap_chain *chain = 0;
  struct rtap_chain *tmp = 0;

  // Search for specified filter chain in list
  spin_lock(&rtap_chains.lock);
  list_for_each_entry_safe(chain, tmp, &rtap_chains.list, list)
  {
    if( ! strcmp( chain->name, name) )
    {
      // Remove specified filter chain from list
      printk( KERN_INFO "RTAP: Removing filter chain: %s\n", chain->name );
      rtap_filter_clear( &chain->filter );
      list_del( &chain->list );
      kfree( chain->name );
      chain->name = 0;
      kfree( chain );
      break;
    }
  } // end loop
  spin_unlock(&rtap_chains.lock);

  return (ret);
}

/******************************************************************************
 *
 ******************************************************************************/
static int
rtap_chain_add(struct rtap_chain* chain)
{

  // Add filter chain to tail of filter chain list
  spin_lock(&rtap_chains.lock);
  list_add_tail(&chain->list, &rtap_chains.list);
  spin_unlock(&rtap_chains.lock);

  return (0);
}

/******************************************************************************
 *
 ******************************************************************************/
static int
rtap_chain_clear(void)
{

  struct rtap_chain *chain = 0;
  struct rtap_chain *tmp = 0;

  // Remove all filter chains from list
  spin_lock(&rtap_chains.lock);
  list_for_each_entry_safe(chain, tmp, &rtap_chains.list, list)
  {
    printk( KERN_INFO "RTAP: Removing filter chain\n");
    rtap_filter_clear( &chain->filter );
    list_del( &chain->list );
    if (chain->name)
    {
      kfree( chain->name );
      chain->name = 0;
    }
    kfree( chain );
  } // end loop
  spin_unlock(&rtap_chains.lock);

  return (1);

}

//*****************************************************************************
// Filter Functions
//*****************************************************************************

/******************************************************************************
 *
 ******************************************************************************/
static int
rtap_filter_all(struct rtap_filter *f, struct sk_buff *skb)
{
  int ret = -1;
  if (f && (f->type == FILTER_TYPE_ALL) && skb)
  {
    switch (f->subtype)
    {

    case FILTER_SUBTYPE_ALL_ALL:
      f->count++;
      ret = rtap_rule_invoke(f->rule, skb);
      break;

    case FILTER_SUBTYPE_ALL_SIZE_EQ:
    {
      int size = 0;
      int cnt = sscanf(f->arg, "%d", &size);
      if ((cnt == 1) && (skb->len == size))
      {
        f->count++;
        ret = rtap_rule_invoke(f->rule, skb);
      }
      break;
    }

    case FILTER_SUBTYPE_ALL_SIZE_GE:
    {
      int size = 0;
      int cnt = sscanf(f->arg, "%d", &size);
      if ((cnt == 1) && (skb->len >= size))
      {
        f->count++;
        ret = rtap_rule_invoke(f->rule, skb);
      }
      break;
    }

    case FILTER_SUBTYPE_ALL_SIZE_LE:
    {
      int size = 0;
      int cnt = sscanf(f->arg, "%d", &size);
      if ((cnt == 1) && (skb->len <= size))
      {
        f->count++;
        ret = rtap_rule_invoke(f->rule, skb);
      }
      break;
    }

    default:
      break;
    }
  }
  return(ret);
}

/******************************************************************************
 *
 ******************************************************************************/
static int
rtap_filter_radiotap(struct rtap_filter *f, struct sk_buff *skb)
{
  int ret = -1;
  if (f && (f->type == FILTER_TYPE_RADIOTAP) && skb)
  {
    switch (f->subtype)
    {
    default:
      break;
    }
  }
  return(ret);
}

/******************************************************************************
 *
 ******************************************************************************/
static int
rtap_filter_80211(struct rtap_filter *f, struct sk_buff *skb)
{
  int ret = -1;
  if (f && (f->type == FILTER_TYPE_80211) && skb)
  {
    switch (f->subtype)
    {
    default:
      break;
    }
  }
  return(ret);
}

/******************************************************************************
 *
 ******************************************************************************/
static int
rtap_filter_ip(struct rtap_filter *f, struct sk_buff *skb)
{
  int ret = -1;
  if (f && (f->type == FILTER_TYPE_IP) && skb)
  {
    switch (f->subtype)
    {
    default:
      break;
    }
  }
  return(ret);
}

//*****************************************************************************
// Global Functions
//*****************************************************************************

/******************************************************************************
 *
 ******************************************************************************/
int
rtap_filter_recv(struct sk_buff *skb)
{

  struct rtap_chain *c = NULL;
  struct rtap_chain *tmp_c = NULL;
  struct rtap_filter* f = NULL;
  struct rtap_filter* tmp_f = NULL;
  struct sk_buff* skb_cloned = 0;

//  printk( KERN_INFO "RTAP: Received by filter\n");

  // Clone socket buffer before messing with it
  skb_cloned = skb_clone(skb, GFP_ATOMIC);

  // Loop through all rtap_filters
  spin_lock(&rtap_chains.lock);
  list_for_each_entry_safe(c, tmp_c, &rtap_chains.list, list)
  {
    list_for_each_entry_safe(f, tmp_f, &c->filter.list, list)
    {
      rtap_filtertbl[f->type]( f, skb_cloned );
    }
  } // end loop
  spin_unlock(&rtap_chains.lock);

  // Free cloned socket buffer
  kfree_skb(skb_cloned);

  // Return success
  return (0);
}

#if 0
//*****************************************************************************


//*****************************************************************************


//*****************************************************************************


//*****************************************************************************

void
rtap_filter_radiotap( struct rtap_filter *f, struct sk_buff *skb )
{
  struct ieee80211_radiotap_header *rthdr = (struct ieee80211_radiotap_header *)skb->data;
  return;
}

//*****************************************************************************

rtap_filter_cmd_t
rtap_filter_80211_mac( struct rtap_filter *fp, struct sk_buff *skb )
{
  rtap_filter_cmd_t ret_cmd = FILTER_CMD_NONE;
  struct ieee80211_radiotap_header *rthdr =
  (struct ieee80211_radiotap_header *)skb->data;
  struct ieee80211_hdr *fhdr =
  (struct ieee80211_hdr *)((uint8_t *)rthdr + cpu_to_le16( rthdr->it_len ) );
  uint8_t mac[6] =
  { 0};
  uint16_t fctl = 0;

  switch( fp->rid )
  {
    case RULE_ID_MAC_SA:
    // Convert mac string to binary
    sscanf( fp->arg, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
        &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
    //printk( KERN_INFO "RTAP: SA(exp): %02x:%02x:%02x:%02x:%02x:%02x\n",
    //     mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    //printk(KERN_INFO "RTAP: SA(obs): %02x:%02x:%02x:%02x:%02x:%02x\n",
    //         fhdr->addr2[0], fhdr->addr2[1], fhdr->addr2[2],
    //         fhdr->addr2[3], fhdr->addr2[4], fhdr->addr2[5]);
    if( ! memcmp( fhdr->addr2, mac, sizeof( fhdr->addr2 ) ) )
    {
      ret_cmd = fp->cmd;
    } // end if
    break;
    case RULE_ID_MAC_DA:
    // Convert mac string to binary
    sscanf( fp->arg, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx",
        &mac[0], &mac[1], &mac[2], &mac[3], &mac[4], &mac[5]);
    //printk(KERN_INFO "RTAP: DA(exp): %02x:%02x:%02x:%02x:%02x:%02x\n",
    //     mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    //printk(KERN_INFO "RTAP: DA(obs): %02x:%02x:%02x:%02x:%02x:%02x\n",
    //         fhdr->addr1[0], fhdr->addr1[1], fhdr->addr1[2],
    //         fhdr->addr1[3], fhdr->addr1[4], fhdr->addr1[5]);
    if( ! memcmp( fhdr->addr1, fp->arg, sizeof( fhdr->addr1 ) ) )
    {
      ret_cmd = fp->cmd;
    } // end if
    break;
    case RULE_ID_MAC_FCTL:
    // Convert fctl string to binary
    sscanf( fp->arg, "%hx", &fctl );
    //printk(KERN_INFO "RTAP: FCTL(exp): 0x%04x\n", fctl);
    //printk(KERN_INFO "RTAP: FCTL(obs): 0x%04x\n", cpu_to_le16(fhdr->frame_control));
    if( cpu_to_le16( fhdr->frame_control ) == fctl )
    {
      ret_cmd = fp->cmd;
    } // end if
    break;
    default:
    break;
  }
  return( ret_cmd );
}

//*****************************************************************************

static int
rtap_filter_remove( rtap_filter_id_t fid )
{
  struct rtap_filter *rtap_filter = 0;
  struct rtap_filter *tmp = 0;
  int ret = -1;

  // Search for device in list and remove
  spin_lock( &rtap_filters.lock );
  list_for_each_entry_safe( rtap_filter, tmp, &rtap_filters.list, list )
  {
    if( rtap_filter->fid == fid )
    {
      printk( KERN_INFO "RTAP: Removing rtap_filter: %d\n", fid );
      // Add rtap_filter statistics entry
      stats_remove( fid );
      // Remove rtap_filter from list
      list_del( &rtap_filter->list );
      kfree( rtap_filter->arg );
      kfree( rtap_filter );
      ret = 0;
      break;
    } // end if
  } // end loop
  spin_unlock( &rtap_filters.lock );

  // Return non-null network device pointer on success; null on error
  return( ret );
}

//*****************************************************************************

static int
rtap_filter_add( rtap_filter_id_t fid, struct rtap_filterype_t type,
    rtap_rule_id_t rid, rtap_filter_cmd_t cmd, char *arg )
{
  struct rtap_filter *rtap_filter = 0;

  // Remove any duplicate rtap_filters
  rtap_filter_remove( fid );

  // Validate rtap_filter type
  if( (type <= FILTER_TYPE_NONE) || (type >= FILTER_TYPE_LAST) )
  {
    printk( KERN_ERR "RTAP: Filter type %d not supported\n", type );
    return( -1 );
  } // end if

  // Validate rule id
  if( (rid < RULE_ID_NONE) || (rid >= RULE_ID_LAST) )
  {
    printk( KERN_ERR "RTAP: Rule id %d not supported\n", rid );
    return( -1 );
  } // end if

  // Validate rtap_filter command
  if( (cmd <= FILTER_CMD_NONE) || (cmd >= FILTER_CMD_LAST) )
  {
    printk( KERN_ERR "RTAP: Filter command %d not supported\n", cmd );
    return( -1 );
  } // end if

  // Allocate new rtap_filter list item
  rtap_filter = kmalloc( sizeof(struct rtap_filter), GFP_ATOMIC );
  if( ! rtap_filter )
  {
    printk( KERN_CRIT "RTAP: Cannot allocate memory\n" );
    return( -1 );
  } // end if
  memset( (void *)rtap_filter, 0, sizeof( struct rtap_filter ) );

  // Populate rtap_filter list item
  rtap_filter->fid = fid;
  rtap_filter->type = type;
  rtap_filter->cmd = cmd;
  rtap_filter->rid = rid;
  rtap_filter->arg = arg;

  printk( KERN_INFO "RTAP: Adding rtap_filter: #%d Type[%s] Cmd[%s] Arg: %s\n",
      fid, struct rtap_filterype_str(type), rtap_filter_cmd_str(cmd), arg );

  // Add rtap_filter statistics entry
  stats_add( fid );

  // Add rtap_filter list item to tail of device list
  spin_lock( &rtap_filters.lock );
  list_add_tail( &rtap_filter->list, &rtap_filters.list );
  spin_unlock( &rtap_filters.lock );

  // Return non-null network device pointer on success; null on error
  return( 0 );
}

//*****************************************************************************

static int
rtap_filter_clear( void )
{
  struct rtap_filter *rtap_filter = 0;
  struct rtap_filter *tmp = 0;

  // Remove all rtap_filters from list
  spin_lock( &rtap_filters.lock );
  list_for_each_entry_safe( rtap_filter, tmp, &rtap_filters.list, list )
  {
    printk( KERN_INFO "RTAP: Removing rtap_filter: %d\n", rtap_filter->fid );
    list_del( &rtap_filter->list );
    kfree( rtap_filter->arg );
    kfree( rtap_filter );
  } // end loop
  spin_unlock( &rtap_filters.lock );

  return( 0 );
}

#endif

//*****************************************************************************
// Global Functions
//*****************************************************************************

/******************************************************************************
 *
 ******************************************************************************/
int
rtap_filter_init(void)
{
  spin_lock_init(&rtap_chains.lock);
  INIT_LIST_HEAD(&rtap_chains.list);
  return (0);
}

/******************************************************************************
 *
 ******************************************************************************/
int
rtap_filter_exit(void)
{
  return (rtap_chain_clear());
}

//*****************************************************************************
// Proc Filesystem Functions
//*****************************************************************************

/******************************************************************************
 *
 ******************************************************************************/
static const char *
rtap_filter_type_str(struct rtap_filter* f)
{
  const char *str = 0;
  switch (f->type)
  {
  case FILTER_TYPE_NONE:
    str = "None";
    break;
  case FILTER_TYPE_ALL:
    str = "All";
    break;
  case FILTER_TYPE_RADIOTAP:
    str = "RadioTap";
    break;
  case FILTER_TYPE_80211:
    str = "802.11 MAC";
    break;
  case FILTER_TYPE_IP:
    str = "IP";
    break;
  case FILTER_TYPE_UDP:
    str = "UDP";
    break;
  case FILTER_TYPE_TCP:
    str = "TCP";
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
rtap_filter_subtype_str(struct rtap_filter* f)
{
  const char *str = 0;
  switch (f->type)
  {
  case FILTER_TYPE_ALL:
  {
    switch (f->subtype)
    {
    case FILTER_SUBTYPE_NONE:
      str = "None";
      break;
    case FILTER_SUBTYPE_ALL_ALL:
      str = "All";
      break;
    case FILTER_SUBTYPE_ALL_SIZE_EQ:
      str = "Equal to size";
      break;
    case FILTER_SUBTYPE_ALL_SIZE_GE:
      str = "Greater than, equal to size";
      break;
    case FILTER_SUBTYPE_ALL_SIZE_LE:
      str = "Less than, equal to size";
      break;
    default:
      str = "Unknown";
      break;
    }
    break;
  }
  case FILTER_TYPE_RADIOTAP:
  {
    break;
  }
  case FILTER_TYPE_80211:
  {
    break;
  }
  case FILTER_TYPE_IP:
  {
    break;
  }
  case FILTER_TYPE_UDP:
  {
    break;
  }
  case FILTER_TYPE_TCP:
  {
    break;
  }
  default:
    str = "Unknown";
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

  struct rtap_chain* c = NULL;
  struct rtap_chain* tmp_c = NULL;
  struct rtap_filter* f = NULL;
  struct rtap_filter* tmp_f = NULL;

  // Iterate over all rtap_filters in list
  spin_lock(&rtap_chains.lock);
  list_for_each_entry_safe(c, tmp_c, &rtap_chains.list, list)
  {
    seq_printf( file, "Chain: %s\n", c->name );
    // Iterate over all rtap_filters in list
    spin_lock( &c->lock );
    list_for_each_entry_safe( f, tmp_f, &c->filter.list, list )
    {
      seq_printf( file, "\t[%u]\t%d\t%16s\t%16s\t%d\t%32s\n",
          f->count, f->fid, rtap_filter_type_str(f),
          rtap_filter_subtype_str(f), rtap_filter_get_rule(f), f->arg);
    } // end loop
    spin_unlock( &c->lock );
  } // end loop
  spin_unlock(&rtap_chains.lock);

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
  char fltrstr[256 + 1] = { 0 };
  char name[32] = { 0 }; // chain name
  int fid = 0; // filter id
  int type = 0; // filter type
  int rid = 0; // rule id
  int subtype = 0; // filter subtype
  char arg[256] = { 0 }; // filter argument string
  int ret = 0;

  if (!cnt)
  {
    return (cnt);
  } // end if

  cnt = (cnt >= 256) ? 256 : cnt;
  copy_from_user(fltrstr, buf, cnt);
  ret = sscanf(fltrstr, "%256s %d %d %d %d %256s", name, &fid, &type, &rid, &subtype, arg);

  printk( KERN_INFO "RTAP: Filter: %s %d %d %d %d %s", name, fid, type, rid, subtype, arg);

  if ((ret == 0) && (strlen(fltrstr) == 1) && (fltrstr[0] == '-'))
  {
    rtap_chain_clear();
  } // end if
  else if ((ret == 1) && (name[0] == '-'))
  {
    rtap_chain_remove(&name[1]);
  }
  else if ((ret == 2) && (fid < 0))
  {
//    struct rtap_chain* chain = rtap_chain_find(name);
//    rtap_filter_remove(&chain->filter, -fid);
  }
  else if ((ret >= 5) && (fid > 0))
  {
    struct rtap_filter* filter = rtap_filter_create();
    struct rtap_chain* chain = rtap_chain_find(name);
    if (!chain)
    {
      if ((chain = rtap_chain_create()))
      {
        rtap_chain_set_name(chain, name);
        rtap_chain_add(chain);
      }
    }
    if (chain && filter)
    {
      if (rtap_filter_set_id(filter, fid) || rtap_filter_set_type(filter, type) ||
          rtap_filter_set_rule(filter, rid) || rtap_filter_set_subtype(filter, subtype) ||
          rtap_filter_set_arg(filter, arg))
      {
        printk( KERN_ERR "RTAP: Invalid arguments\n");
        rtap_filter_destroy(filter);
        return(-1);
      }
      rtap_filter_add(chain, filter);
    }
    else
    {
      printk( KERN_ERR "RTAP: Failed to add filter\n");
      return(-1);
    }
  } // end else if
  else
  {
    printk( KERN_ERR "RTAP: Failed parsing rtap_filter string: %s\n", fltrstr);
    return (-1);
  } // end else

  // Return number of bytes written
  return (cnt);
}

/******************************************************************************
 *
******************************************************************************/
const struct file_operations rtap_filter_fops =
{
    .owner      = THIS_MODULE,
    .open       = proc_open,
    .release    = proc_close,
    .read       = proc_read,
    .llseek     = proc_lseek,
    .write      = proc_write,
};

