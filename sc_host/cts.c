#include "gatt.h"
#include <cts.h>
#include <time.h>
#include <stdint.h>

enum CTSAttrEnum {
    CTS_ATTR_CT,
    CTS_ATTR_LTI,
    CTS_ATTR_RTI,
    CTS_ATTR_MAX,
};

enum AdjustReasonEnum {
    ADJUST_REASON_MANUAL_UPDATE     = 0x1,
    ADJUST_REASON_TIMEZONE_CHANGE   = 0x2,
    ADJUST_REASON_DST_CHANGE        = 0x4,//(Day Light Savings Time)
};

enum DstOffsetEnum {
    DST_OFFSET_STANDARD_TIME            = 0x1,
    DST_OFFSET_HALF_HOUR_DAY_LIGHT_TIME = 0x2,
    DST_OFFSET_DAY_LIGHT_TIME           = 0x4,
    DST_OFFSET_DOUBLE_DAY_LIGHT_TIME    = 0x8,
};

enum TimeSourceEnum {
    TIME_SOURCE_UNKNOWN,
    TIME_SOURCE_NETWORK_PROTOCOL,
    TIME_SOURCE_GPS,
    TIME_SOURCE_RADIO_TIME_SIGNAL,
    TIME_SOURCE_MANUAL,
    TIME_SOURCE_ATOMIC_CLOCK,
    TIME_SOURCE_CELLULAR_NETWORK,
};

struct current_time_information_characteristic {
    uint16_t year;      //[1582, 9999]
    uint8_t month;      //[0, 12] ; 0 means unknown
    uint8_t day;        //[1, 31]
    uint8_t hour;       //[0, 23]
    uint8_t minute;     //[0, 59]
    uint8_t second;     //[0, 59]
    uint8_t day_of_week;//[0, 7]  ; 0 means unknown
    uint8_t frac_256;   //[0, 255]; 0 means not support
    uint8_t adjust_reason;//(bitset) 
} __attribute__((packed));

struct local_time_information_characteristic {
    int8_t time_zone;  //[-48, 56] //15min timezone, -128 means unknown
    uint8_t dst_offset; //[0, 8] ; (single bit)
} __attribute__((packed));

struct reference_time_information_characteristic {
    uint8_t time_source;
    uint8_t time_accuracy;
    uint8_t days_since_update;
    uint8_t hours_since_update;
} __attribute__((packed));

struct gatt_attr_t cts_attrs[] = {
    [CTS_ATTR_CT]   = {0x2a2b, 0},
    [CTS_ATTR_LTI]  = {0x2a0f, 0},
    [CTS_ATTR_RTI]  = {0x2a14, 0},
};

struct current_time_information_characteristic curr_time_characteristic;
struct local_time_information_characteristic local_time_characteristic;
struct reference_time_information_characteristic reference_time_characteristic;

void current_time_service_init()
{
    gatt_find_attributes(cts_attrs, CTS_ATTR_MAX);
}

void current_time_service_timer()
{
    time_t tm_val;
    struct tm tm_st;

    reference_time_characteristic.time_source  = TIME_SOURCE_NETWORK_PROTOCOL;
    reference_time_characteristic.time_accuracy= 8 ;// unit 125ms
    reference_time_characteristic.days_since_update = 0;
    reference_time_characteristic.hours_since_update = 0;
    
    local_time_characteristic.time_zone = 9;
    local_time_characteristic.dst_offset = DST_OFFSET_DAY_LIGHT_TIME;

    time(&tm_val);
    localtime_r(&tm_val, &tm_st);
    curr_time_characteristic.year  = tm_st.tm_year + 1900;
    curr_time_characteristic.month = tm_st.tm_mon;
    curr_time_characteristic.day   = tm_st.tm_mday;
    curr_time_characteristic.hour  = tm_st.tm_hour;
    curr_time_characteristic.minute = tm_st.tm_min;
    curr_time_characteristic.second = tm_st.tm_sec;
    curr_time_characteristic.frac_256 = 0;
    curr_time_characteristic.day_of_week = tm_st.tm_wday;
    curr_time_characteristic.adjust_reason = 0;

    gatt_write_attribute(cts_attrs[CTS_ATTR_CT].attr, 0, 
            sizeof(curr_time_characteristic), (uint8_t*)&curr_time_characteristic);
    gatt_write_attribute(cts_attrs[CTS_ATTR_LTI].attr, 0,
            sizeof(local_time_characteristic), (uint8_t*)&local_time_characteristic);
    gatt_write_attribute(cts_attrs[CTS_ATTR_RTI].attr, 0,
            sizeof(reference_time_characteristic), (uint8_t*)&reference_time_characteristic);
}

