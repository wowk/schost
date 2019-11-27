#ifndef SC_CTS_PROFILE_H_
#define SC_CTS_PROFILE_H_

#include <stdint.h>

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

struct reference_time_info_characteristic {
    uint8_t time_source;
    uint8_t time_accuracy;
    uint8_t days_since_update;
    uint8_t hours_since_update;
} __attribute__((packed));

extern struct current_time_information_characteristic curr_time_characteristic;
extern struct local_time_information_characteristic local_time_characteristic;
extern struct reference_time_info_characteristic reference_time_characteristic;


#ifdef __cplusplus
extern "C" {
#endif

void current_time_service_update();

#ifdef __cplusplus
}
#endif

#endif
