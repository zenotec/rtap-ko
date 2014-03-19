#ifndef __FILTER_H__
#define __FILTER_H__

#include "rtap-ko.h"
#include "rule.h"

typedef enum filter_id
{
    FILTER_ID_NONE = 0,
    FILTER_ID_DROP = 1,
    FILTER_ID_RADIOTAP = 2,
    FILTER_ID_80211_MAC = 3,
    FILTER_ID_LAST
} filter_id_t;

extern rule_cmd_t rtap_filter_drop(rule_id_t id, rule_cmd_t cmd, void *buf, void *val);
extern rule_cmd_t rtap_filter_radiotap(rule_id_t id, rule_cmd_t cmd, void *buf, void *val);
extern rule_cmd_t rtap_filter_80211_mac(rule_id_t id, rule_cmd_t cmd, void *buf, void *val);

#endif
