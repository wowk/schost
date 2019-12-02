#include "gatt.h"
#include <stdint.h>

enum ESSAttrEnum {
    ESS_ATTR_AWD,
    ESS_ATTR_AWS,
    ESS_ATTR_DVC,
    ESS_ATTR_DP,
    ESS_ATTR_GF,
    ESS_ATTR_HI,
    ESS_ATTR_IR,
    ESS_ATTR_PC,
    ESS_ATTR_TWD,
    ESS_ATTR_RF,
    ESS_ATTR_TWS,
    ESS_ATTR_UI,
    ESS_ATTR_WC,
    ESS_ATTR_EV,
    ESS_ATTR_HD,
    ESS_ATTR_PS,
    ESS_ATTR_TM,
    ESS_ATTR_BP,
    ESS_ATTR_MD,
    ESS_ATTR_MFD2D,
    ESS_ATTR_MFD3D,
    ESS_ATTR_MAX,
};

struct gatt_attr_t ess_attrs[] = {
    [ESS_ATTR_AWD]  = {0x2a73, 0},
    [ESS_ATTR_AWS]  = {0x2a72, 0},
    [ESS_ATTR_DVC]  = {0x2a7d, 0},
    [ESS_ATTR_DP]   = {0x2a7b, 0},
    [ESS_ATTR_GF]   = {0x2a74, 0},
    [ESS_ATTR_HI]   = {0x2a7a, 0},
    [ESS_ATTR_IR]   = {0x2a77, 0},
    [ESS_ATTR_PC]   = {0x2a75, 0},
    [ESS_ATTR_TWD]  = {0x2a71, 0},
    [ESS_ATTR_RF]   = {0x2a78, 0},
    [ESS_ATTR_TWS]  = {0x2a70, 0},
    [ESS_ATTR_UI]   = {0x2a76, 0},
    [ESS_ATTR_WC]   = {0x2a79, 0},
    [ESS_ATTR_EV]   = {0x2a6c, 0},
    [ESS_ATTR_HD]   = {0x2a6f, 0},
    [ESS_ATTR_PS]   = {0x2a6d, 0},
    [ESS_ATTR_TM]   = {0x2a6e, 0},
    [ESS_ATTR_BP]   = {0x2aa3, 0},
    [ESS_ATTR_MD]   = {0x2a2c, 0},
    [ESS_ATTR_MFD2D] = {0x2aa0, 0},
    [ESS_ATTR_MFD3D] = {0x2aa1, 0},
};

void environment_sensing_service_init()
{
    gatt_find_attributes(ess_attrs, ESS_ATTR_MAX);
}

void environment_sensing_service_timer()
{

}

