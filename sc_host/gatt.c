#include "cts.h"
#include "ans.h"
#include "hps.h"
#include "ess.h"
#include "gatt.h"
#include "timer.h"
#include "headers.h"
#include "host_gecko.h"


#define GATT_TIMER_ID        0x30
#define GATT_TIMER_INTERVAL  1.0f

static int gatt_timer_handler(struct hw_timer_t* t);

struct hw_timer_t gatt_timer = {
    .count  = HW_TIMER_INFINITY,
    .arg    = NULL,
    .ret    = BLE_EVENT_IGNORE,
    .id     = GATT_TIMER_ID,
    .callback = gatt_timer_handler,
    .interval = GATT_TIMER_INTERVAL,
};

int gatt_find_attribute(const uint8_t* uuid, uint16_t* attr)
{
    struct gecko_msg_gatt_server_find_attribute_rsp_t* attrrsp;
    
    attrrsp = gecko_cmd_gatt_server_find_attribute(0, 2, uuid);
    if(!attrrsp){
        error(0, EINVAL, "failed to get attr response");
    }else if(attrrsp->result){
        error(0, EINVAL, "result: %s(%.4x)\n", error_summary(attrrsp->result), attrrsp->result);
    }else{
        *attr = attrrsp->attribute;
        return 0;
    }

    return -1;
}

int gatt_find_attributes(struct gatt_attr_t* gas, int size)
{
    for(int i = 0 ; i < size ; i ++){
        gatt_find_attribute((uint8_t*)&gas[i].uuid, &gas[i].attr);
        info("UUID(%.4X) : ATTR(%.4X)", gas[i].uuid, gas[i].attr);
    }

    return 0;
}

void gatt_write_attribute(uint16_t attr, uint16_t offset, uint8_t value_len, uint8_t* value)
{
    gecko_cmd_gatt_server_write_attribute_value(attr, offset, value_len, value);
}

static int gatt_timer_handler(struct hw_timer_t* t)
{
    current_time_service_timer();
    alert_notification_service_timer();
    environment_sensing_service_timer();
    http_proxy_service_timer();

    return BLE_EVENT_IGNORE;
}

int gatt_bootup_handler(struct sock_t* sock, struct option_args_t* args)
{
    hw_timer_add(&gatt_timer);

    current_time_service_init();
    alert_notification_service_init();
    environment_sensing_service_init();
    http_proxy_service_init();

    return 0;
}

int gatt_cmd_handler(struct sock_t* sock, struct option_args_t* args)
{
    return BLE_EVENT_CONTINUE;
}

int gatt_event_handler(struct sock_t* sock, struct option_args_t* args, struct gecko_cmd_packet *evt)
{
    return BLE_EVENT_RETURN;
}

int gatt_cleanup(struct sock_t* sock, struct option_args_t* args)
{
    return 0;
}

