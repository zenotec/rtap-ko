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
//    File: rule.h
//    Description: TODO: Replace temp rules with those configurable from
//                 user space.
//
//*****************************************************************************

#ifndef __RULE_H__
#define __RULE_H__

//*****************************************************************************
// Type definitions
//*****************************************************************************

typedef enum rule_id
{
    RULE_ID_NONE = 0,
    RULE_ID_MAC_SA = 1,
    RULE_ID_MAC_DA = 2,
    RULE_ID_MAC_FCTL = 3,
    RULE_ID_DROP
} rule_id_t;

typedef enum rule_cmd
{
    RULE_CMD_NONE = 0,
    RULE_CMD_DROP = 1,
    RULE_CMD_FORWARD = 2,
    RULE_CMD_LAST
} rule_cmd_t;

typedef rule_cmd_t (*filter_func_t)(rule_id_t id, rule_cmd_t cmd, void *buf, void *val);

typedef struct rule
{
    uint16_t id;
    uint16_t cmd;
    filter_func_t func;
    void *val;
} rule_t;

extern rule_t ruletbl[];

#endif
