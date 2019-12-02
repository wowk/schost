#include "debug.h"
#include "gatt.h"
#include <stdint.h>

enum ANSAttrEnum {
    ANS_ATTR_SNAC,
    ANS_ATTR_SUAC,
    ANS_ATTR_NA,
    ANS_ATTR_UAS,
    ANS_ATTR_ANCP,
    ANS_ATTR_MAX,
};

struct gatt_attr_t ans_attrs[] = {
    [ANS_ATTR_SNAC] = {0x2a47, 0},
    [ANS_ATTR_SUAC] = {0x2a48, 0},
    [ANS_ATTR_NA]   = {0x2a46, 0},
    [ANS_ATTR_UAS]  = {0x2a45, 0},
    [ANS_ATTR_ANCP] = {0x2a44, 0},
};

void alert_notification_service_init()
{
    gatt_find_attributes(ans_attrs, ANS_ATTR_MAX);

}

void alert_notification_service_timer()
{
    
}
