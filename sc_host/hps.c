#include "gatt.h"
#include <stdint.h>

enum HPSAttrEnum {
    HPS_ATTR_URI,
    HPS_ATTR_HH,
    HPS_ATTR_HSC,
    HPS_ATTR_HEB,
    HPS_ATTR_HCP,
    HPS_ATTR_HS,
    HPS_ATTR_MAX,
};

struct gatt_attr_t hps_attrs[] = {
    [HPS_ATTR_URI]  = {0x2ab6, 0},
    [HPS_ATTR_HH]   = {0x2ab7, 0},
    [HPS_ATTR_HSC]  = {0x2ab8, 0},
    [HPS_ATTR_HEB]  = {0x2ab9, 0},
    [HPS_ATTR_HCP]  = {0x2aba, 0},
    [HPS_ATTR_HS]   = {0x2abb, 0},
};

void http_proxy_service_init()
{
    gatt_find_attributes(hps_attrs, HPS_ATTR_MAX);
}

void http_proxy_service_timer()
{

}
