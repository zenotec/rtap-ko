#include <linux/ieee80211.h>
#include <net/mac80211.h>
#include <net/ieee80211_radiotap.h>

#include "filter.h"
#include "rule.h"

rule_cmd_t rtap_filter_drop(rule_id_t id, rule_cmd_t cmd, void *buf, void *val)
{
    return(RULE_CMD_DROP);
}

rule_cmd_t rtap_filter_radiotap(rule_id_t id, rule_cmd_t cmd, void *buf, void *val)
{
    rule_cmd_t ret_cmd = RULE_CMD_NONE;
    struct ieee80211_radiotap_header *rthdr = 0;
    switch(id)
    {
        default:
            break;
    }
    return(ret_cmd);
}

rule_cmd_t rtap_filter_80211_mac(rule_id_t id, rule_cmd_t cmd, void *buf, void *val)
{
    rule_cmd_t ret_cmd = RULE_CMD_NONE;
    struct ieee80211_radiotap_header *rthdr = (struct ieee80211_radiotap_header *)buf;
    struct ieee80211_hdr *fhdr = (struct ieee80211_hdr *)((uint8_t *)rthdr + rthdr->it_len);
    printk(KERN_INFO "1-FCTL: 0x%04x\n", cpu_to_le16(fhdr->frame_control));
    printk(KERN_INFO "1-DA: %x:%x:%x\n", fhdr->addr1[0], fhdr->addr1[1], fhdr->addr1[2]);
    printk(KERN_INFO "1-SA: %x:%x:%x\n", fhdr->addr2[0], fhdr->addr2[1], fhdr->addr2[2]);
    switch(id)
    {
        case RULE_ID_MAC_SA:
            printk(KERN_INFO "2-SA: %x:%x:%x\n", ((uint8_t *)val)[0], ((uint8_t *)val)[1], ((uint8_t *)val)[2]);
            if( !memcmp(fhdr->addr2, val, sizeof(fhdr->addr2)) )
                ret_cmd = cmd;
            break;
        case RULE_ID_MAC_DA:
            printk(KERN_INFO "2-DA: %x:%x:%x\n", ((uint8_t *)val)[0], ((uint8_t *)val)[1], ((uint8_t *)val)[2]);
            if( !memcmp(fhdr->addr1, val, sizeof(fhdr->addr1)) )
                ret_cmd = cmd;
            break;
        case RULE_ID_MAC_FCTL:
            printk(KERN_INFO "2-FCTL: 0x%04x\n", *(uint16_t *)val);
            if( cpu_to_le16(fhdr->frame_control) == *(uint16_t *)val )
                ret_cmd = cmd;
            break;
        default:
            break;
    }
    return(ret_cmd);
}

