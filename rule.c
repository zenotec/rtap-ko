
#include <linux/ieee80211.h>
#include <net/mac80211.h>
#include <net/ieee80211_radiotap.h>

#include "rule.h"
#include "filter.h"

static uint8_t mys4[] = { 0xF0, 0x25, 0xB7, 0x00, 0xB5, 0x33 };
static uint8_t bc[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
static uint16_t beacon = { IEEE80211_STYPE_BEACON };
static uint16_t probe = { IEEE80211_STYPE_PROBE_REQ };

rule_t ruletbl[] =
{
    {RULE_ID_MAC_SA, RULE_CMD_FORWARD, &rtap_filter_80211_mac, (void *)&mys4}, // TODO: temp rule for testing
    {RULE_ID_MAC_FCTL, RULE_CMD_FORWARD, &rtap_filter_80211_mac, (void *)&probe}, // TODO: temp rule for testing
    {RULE_ID_MAC_FCTL, RULE_CMD_DROP, &rtap_filter_80211_mac, (void *)&beacon}, // TODO: temp rule for testing
    {RULE_ID_MAC_DA, RULE_CMD_DROP, &rtap_filter_80211_mac, (void *)&bc}, // TODO: temp rule for testing
    {RULE_ID_DROP, RULE_CMD_DROP, &rtap_filter_drop, 0 }, // Must be last
    { 0 } // Safety stop
};


