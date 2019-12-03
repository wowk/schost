#include "cts.h"
#include "ans.h"
#include "hps.h"
#include "ess.h"
#include "gatt.h"
#include "timer.h"
#include "headers.h"
#include "host_gecko.h"
#include "connection.h"

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

size_t hex2str(uint8_t* data, size_t len, char* buffer)
{
    strcpy(buffer, "0x");
    buffer += 2;
    for(int i = 0 ; i < len ; i ++){
        sprintf(buffer, "%.2X", data[i]);
        buffer += 2;
    }

    return len;
}

int gatt_find_local_attribute(uint16_t uuid, uint16_t* attr)
{
    struct gecko_msg_gatt_server_find_attribute_rsp_t* attrrsp;
    
    attrrsp = gecko_cmd_gatt_server_find_attribute(0, 2, (uint8_t*)&uuid);
    if(!attrrsp){
        error(0, EINVAL, "failed to get attr response");
        return -1;
    }else if(attrrsp->result){
        error(0, EINVAL, "result: %s(%.4x)\n", error_summary(attrrsp->result), attrrsp->result);
        return attrrsp->result;
    }

    *attr = attrrsp->attribute;
    return 0;
}

int gatt_find_local_attributes(struct gatt_attr_t* gas, int size)
{
    for(int i = 0 ; i < size ; i ++){
        gatt_find_local_attribute(gas[i].uuid, &gas[i].attr);
        info("UUID(%.4X) : ATTR(%.4X)", gas[i].uuid, gas[i].attr);
    }

    return 0;
}

int gatt_read_local_attribute(uint16_t attr, uint8_t* len, uint8_t* buf)
{
    struct gecko_msg_gatt_server_read_attribute_value_rsp_t* rsp;

    rsp = gecko_cmd_gatt_server_read_attribute_value(attr, 0);
    if(rsp->result){
        error(0, 0, "READ Attr Error: %s(%d)\n", error_summary(rsp->result), rsp->result);
        return rsp->result;
    }
    memcpy(buf, rsp->value.data, rsp->value.len);
    *len = rsp->value.len;

    return 0;
}

int gatt_read_local_attribute_by_uuid(uint16_t uuid, uint8_t* size, uint8_t* buf)
{
    uint16_t result;
    uint16_t attr = 0;

    result = gatt_find_local_attribute(uuid, &attr);
    if(result){
        return result;
    }
    
    return gatt_read_local_attribute(attr, size, buf);
}

int gatt_write_local_attribute(uint16_t attr, uint8_t size, uint8_t* data)
{
    struct gecko_msg_gatt_server_write_attribute_value_rsp_t* rsp;

    rsp = gecko_cmd_gatt_server_write_attribute_value(attr, 0, size, data);
    if(rsp->result){
        error(0, 0, "Write Attr Error: %s(%d)\n", error_summary(rsp->result), rsp->result);
        return rsp->result;
    }

    return 0;
}

int gatt_write_local_attribute_by_uuid(uint16_t uuid, uint8_t size, uint8_t* data)
{
    uint16_t result;
    uint16_t attr = 0;
    
    result = gatt_find_local_attribute(uuid, &attr);
    if(result){
        return result;
    }
    
    return gatt_write_local_attribute(attr, size, data);
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
    int ret = BLE_EVENT_RETURN;
    uint16_t result;
    uint8_t size = 0;
    uint8_t value[256];
    char sbuffer[258] = "";

    switch(args->gatt.option){
    case OPT_GATT_READ:
        printf_socket(sock, "read: connection: 0x%.2x, uuid: 0x%.4X", args->gatt.connection, args->gatt.uuid);
        if(args->gatt.connection == 0x100){
            result = gatt_read_local_attribute_by_uuid(args->gatt.uuid, &size, value);
            if(result){
                printf_socket(sock, "read failed: %s\n", error_summary(result));
            }else{
                hex2str(value, size, sbuffer);
                printf_socket(sock, "%s", sbuffer);
            }
        }else{
            if(!connection_find_by_conn(args->gatt.connection)){
                printf_socket(sock, "Connection Not Found");
            }else{
            }
        }
        break;
    case OPT_GATT_WRITE:
        printf_socket(sock, "write: connection: 0x%.2x, uuid: 0x%.4X", args->gatt.connection, args->gatt.uuid);
        if(args->gatt.connection == 0x100){
            result = gatt_write_local_attribute_by_uuid(args->gatt.uuid, 
                    args->gatt.write.value.size, args->gatt.write.value.value);
            if(result){
                printf_socket(sock, "write failed: %s\n", error_summary(result));
            }else{
                printf_socket(sock, "write done");
            }
        }else{
            printf_socket(sock, "not supported yet");
        }
        break;
    case OPT_GATT_NOTIFY:
        if(args->gatt.connection == 0x100){
            printf_socket(sock, "connection must be given");
        }
        printf_socket(sock, "notify: connection: 0x%.2x, uuid: 0x%.4X", args->gatt.connection, args->gatt.uuid);
        break;
    default:
        printf_socket(sock, "Unknown GATT command");
        break;
    }
    return ret;
}

int gatt_event_handler(struct sock_t* sock, struct option_args_t* args, struct gecko_cmd_packet *evt)
{
    int ret = BLE_EVENT_CONTINUE;

    switch(BGLIB_MSG_ID(evt->header)){
    case gecko_evt_gatt_service_id:
        printf_socket(sock, "==========GATT Service Discovered========");
        ret = BLE_EVENT_RETURN;;
        break;
    case gecko_evt_gatt_procedure_completed_id:
        printf_socket(sock, "==========Procedure Completed========");
        ret = BLE_EVENT_RETURN;;
        break;
    default:
        //ret = BLE_EVENT_RETURN;
        break;
    }

    return ret;
}

int gatt_cleanup(struct sock_t* sock, struct option_args_t* args)
{
    return 0;
}

