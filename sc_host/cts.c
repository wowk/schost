#include <cts.h>
#include <time.h>

struct current_time_information_characteristic curr_time_characteristic;
struct local_time_information_characteristic local_time_characteristic;
struct reference_time_info_characteristic reference_time_characteristic;

void current_time_service_update()
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
}
