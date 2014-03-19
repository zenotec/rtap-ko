#ifndef __RULE_H__
#define __RULE_H__

#include "rtap-ko.h"

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
