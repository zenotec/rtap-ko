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
//    File: rule.c
//    Description: TODO: Replace temp rules with those configurable from
//                 user space.
//
//*****************************************************************************

//*****************************************************************************
// Includes
//*****************************************************************************

#include <linux/ieee80211.h>
#include <net/mac80211.h>
#include <net/ieee80211_radiotap.h>

#include "rule.h"
#include "filter.h"

//*****************************************************************************
// Global variables
//*****************************************************************************

static const uint8_t mys4[] = { 0xF0, 0x25, 0xB7, 0x00, 0xB5, 0x33 };
static const uint8_t bc[] = { 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF };
static const uint16_t beacon = { IEEE80211_STYPE_BEACON };
static const uint16_t probe = { IEEE80211_STYPE_PROBE_REQ };

rule_t ruletbl[] =
{
    {RULE_ID_MAC_SA, RULE_CMD_FORWARD, &rtap_filter_80211_mac, (void *)&mys4}, // TODO: temp rule for testing
    {RULE_ID_MAC_FCTL, RULE_CMD_FORWARD, &rtap_filter_80211_mac, (void *)&probe}, // TODO: temp rule for testing
    {RULE_ID_MAC_FCTL, RULE_CMD_DROP, &rtap_filter_80211_mac, (void *)&beacon}, // TODO: temp rule for testing
    {RULE_ID_MAC_DA, RULE_CMD_DROP, &rtap_filter_80211_mac, (void *)&bc}, // TODO: temp rule for testing
    {RULE_ID_DROP, RULE_CMD_DROP, &rtap_filter_drop, 0 }, // Must be last
    { 0 } // Safety stop
};


