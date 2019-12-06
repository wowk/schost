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

enum AnsCategoryID {
    ACI_SIMPLE_ALERT = 0x01,
    ACI_EMAIL        = 0x02,
    ACI_NEWS         = 0x04,
    ACI_CALL         = 0x08,
    ACI_MISSED_CALL  = 0x10,
    ACI_SMS_MMS      = 0x20,
    ACI_VOICE_MAIL   = 0x40,
    ACI_SCHEDULE     = 0x80,
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
    gatt_find_local_attributes(ans_attrs, ANS_ATTR_MAX);
    
    uint8_t category_id_mask = ACI_SIMPLE_ALERT;
    gatt_write_local_attribute(ans_attrs[ANS_ATTR_SNAC].attr, 1, &category_id_mask);
}

void alert_notification_service_timer()
{
    
}
