#include "cts.h"
#include "gatt.h"
#include "timer.h"
#include "headers.h"

#define CTS_TIMER_ID        0x30
#define CTS_TIMER_INTERVAL  1.0f


uint16_t curr_time_attribute;
uint16_t local_time_attribute;
uint16_t reference_time_attribute;

static int cts_timer_handler(void* arg);

struct hw_timer_t cts_timer = {
    .count  = 0,
    .arg    = NULL,
    .id     = CTS_TIMER_ID,
    .callback = cts_timer_handler,
    .interval = CTS_TIMER_INTERVAL,
};

static int find_attribute(const uint8_t* uuid, uint16_t* attr)
{
    struct gecko_msg_gatt_server_find_attribute_rsp_t* attrrsp;
    
    attrrsp = gecko_cmd_gatt_server_find_attribute(0, 2, uuid);
    if(!attrrsp){
        info("failed to get attr response");
    }else if(attrrsp->result){
        info("result: %s(%.4x)\n", error_summary(attrrsp->result), attrrsp->result);
    }else{
        info("attribute: %.4x", attrrsp->attribute);
        *attr = attrrsp->attribute;
        return 0;
    }

    return -1;
}

static int cts_timer_handler(void* arg)
{
    current_time_service_update();
    gecko_cmd_gatt_server_write_attribute_value(
            curr_time_attribute, 0, 10, (uint8_t*)&curr_time_characteristic);
    gecko_cmd_gatt_server_write_attribute_value(
            local_time_attribute, 0, 2, (uint8_t*)&local_time_characteristic);
    gecko_cmd_gatt_server_write_attribute_value(
            reference_time_attribute, 0, 4, (uint8_t*)&reference_time_characteristic);
    
    return 0;
}

int gatt_bootup_handler(struct option_args_t* args)
{
    find_attribute(ct_uuid, &curr_time_attribute);
    find_attribute(lti_uuid, &local_time_attribute);
    find_attribute(rti_uuid, &reference_time_attribute);
    gecko_cmd_hardware_set_soft_timer(CTS_TIMER_INTERVAL, CTS_TIMER_ID, 0);
    
    hw_timer_add(&cts_timer);

    return 0;
}

int gatt_cmd_handler(struct sock_t* sock, struct option_args_t* args)
{
    return BLE_EVENT_CONTINUE;
}

int gatt_event_handler(struct sock_t* sock, struct option_args_t* args, struct gecko_cmd_packet *evt)
{
    int ret = BLE_EVENT_RETURN;
    return ret;
}

int gatt_cleanup(struct sock_t* sock, struct option_args_t* args)
{
    return 0;
}

