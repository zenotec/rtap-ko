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
//    File: filter.c
//    Description:  Filters specific to a group of rules.
//
//*****************************************************************************

//*****************************************************************************
// Includes
//*****************************************************************************

#include <linux/ieee80211.h>
#include <net/mac80211.h>
#include <net/ieee80211_radiotap.h>

#include "rtap-ko.h"
#include "rule.h"
#include "filter.h"

//*****************************************************************************
// Functions
//*****************************************************************************

//*****************************************************************************
rule_cmd_t rtap_filter_drop(rule_id_t id, rule_cmd_t cmd, void *buf, void *val)
{
    return(RULE_CMD_DROP);
}

//*****************************************************************************
rule_cmd_t rtap_filter_radiotap(rule_id_t id, rule_cmd_t cmd, void *buf, void *val)
{
    rule_cmd_t ret_cmd = RULE_CMD_NONE;
    struct ieee80211_radiotap_header *rthdr = 0;
    if ( rthdr ); // TODO: Implement
    switch(id)
    {
        default:
            break;
    }
    return(ret_cmd);
}

//*****************************************************************************
rule_cmd_t rtap_filter_80211_mac( rule_id_t id, rule_cmd_t cmd, void *buf, void *val )
{
    rule_cmd_t ret_cmd = RULE_CMD_NONE;
    struct ieee80211_radiotap_header *rthdr = (struct ieee80211_radiotap_header *)buf;
    struct ieee80211_hdr *fhdr = (struct ieee80211_hdr *)((uint8_t *)rthdr + rthdr->it_len);
    switch( id )
    {
        case RULE_ID_MAC_SA:
            printk( KERN_INFO "RTAP: SA(exp): %02x:%02x:%02x:%02x:%02x:%02x\n",
                     ((uint8_t *)val)[0], ((uint8_t *)val)[1], ((uint8_t *)val)[2],
                     ((uint8_t *)val)[3], ((uint8_t *)val)[4], ((uint8_t *)val)[5] );
            printk(KERN_INFO "RTAP: SA(obs): %02x:%02x:%02x:%02x:%02x:%02x\n",
                     fhdr->addr2[0], fhdr->addr2[1], fhdr->addr2[2],
                     fhdr->addr2[3], fhdr->addr2[4], fhdr->addr2[5]);
            if( !memcmp(fhdr->addr2, val, sizeof(fhdr->addr2)) )
            {
                ret_cmd = cmd;
            } // end if
            break;
        case RULE_ID_MAC_DA:
            printk(KERN_INFO "RTAP: DA(exp): %02x:%02x:%02x:%02x:%02x:%02x\n",
                     ((uint8_t *)val)[0], ((uint8_t *)val)[1], ((uint8_t *)val)[2],
                     ((uint8_t *)val)[3], ((uint8_t *)val)[4], ((uint8_t *)val)[5]);
            printk(KERN_INFO "RTAP: DA(obs): %02x:%02x:%02x:%02x:%02x:%02x\n",
                     fhdr->addr1[0], fhdr->addr1[1], fhdr->addr1[2],
                     fhdr->addr1[3], fhdr->addr1[4], fhdr->addr1[5]);
            if( !memcmp(fhdr->addr1, val, sizeof(fhdr->addr1)) )
            {
                ret_cmd = cmd;
            } // end if
            break;
        case RULE_ID_MAC_FCTL:
            printk(KERN_INFO "RTAP: FCTL(exp): 0x%04x\n", *(uint16_t *)val);
            printk(KERN_INFO "RTAP: FCTL(obs): 0x%04x\n", cpu_to_le16(fhdr->frame_control));
            if( cpu_to_le16(fhdr->frame_control) == *(uint16_t *)val )
            {
                ret_cmd = cmd;
            } // end if
            break;
        default:
            break;
    }
    return(ret_cmd);
}

