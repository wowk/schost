#include "cts.h"
#include "ans.h"
#include "hps.h"
#include "ess.h"
#include "gatt.h"
#include "timer.h"
#include "notify.h"
#include "headers.h"
#include "host_gecko.h"
#include "notify.h"
#include "connection.h"

#define GATT_TIMER_ID        0x30
#define GATT_TIMER_INTERVAL  5.0f

static int gatt_timer_handler(struct hw_timer_t* t);

struct hw_timer_t gatt_timer = {
    .count  = HW_TIMER_INFINITY,
    .arg    = NULL,
    .ret    = BLE_EVENT_IGNORE,
    .id     = GATT_TIMER_ID,
    .callback = gatt_timer_handler,
    .interval = GATT_TIMER_INTERVAL,
};

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
    info("UUID: %4X => Attribute: %.4X", uuid, attrrsp->attribute);
    *attr = attrrsp->attribute;
    return 0;
}

int gatt_find_local_attributes(struct gatt_attr_t* gas, int size)
{
    for(int i = 0 ; i < size ; i ++){
        gatt_find_local_attribute(gas[i].uuid, &gas[i].attr);
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
    int result;
    uint8_t connid;
    uint8_t size = 0;
    uint8_t value[256];
    uint16_t characteristic;
    char sbuffer[515] = "";
    struct connection_t* conn;
    struct descriptor_t* descriptor;
    struct characteristic_t* character;

    switch(args->gatt.option){
    case OPT_GATT_READ:
        printf_socket(sock, "read: connection: 0x%.2x, uuid: 0x%.4X", args->gatt.connection, args->gatt.uuid);
        if(args->gatt.connection == 0x100){
            if(args->gatt.descriptor){
                printf_socket(sock, "Not Supported to Read Local Descriptor");
                break;
            }
            result = gatt_read_local_attribute_by_uuid(ntohs(args->gatt.uuid), &size, value);
            if(result){
                printf_socket(sock, "read failed: %s\n", error_summary(result));
            }else{
                hex2str(value, size, sbuffer);
                printf_socket(sock, "value: %s", sbuffer);
            }
        }else{
            conn = connection_find_by_conn(args->gatt.connection);
            if(!conn){
                printf_socket(sock, "Connection Not Found");
                break;
            }
            character = characteristic_find_by_uuid(&conn->characteristic_list, args->gatt.uuid);
            if(!character){
                printf_socket(sock, "Characteristic Not Found");
                break;
            }
            if(args->gatt.descriptor){
                descriptor = descriptor_find_by_handle(&conn->descriptor_list, character->handle, args->gatt.descriptor);
                if(!descriptor){
                    printf_socket(sock, "Descriptor Not Found");
                    break;
                }
                result = gecko_cmd_gatt_read_descriptor_value(conn->connection, descriptor->handle)->result;
                if(result){
                    printf_socket(sock, "Error(%d): %s", result, error_summary(result));
                    break;
                }
            }else{
                printf_socket(sock, "read(Conn: %d, Characteristic: %d)", conn->connection, character->handle);
                result = gecko_cmd_gatt_read_characteristic_value(conn->connection, character->handle)->result;
                if(result){
                    printf_socket(sock, "Error(%d): %s", result, error_summary(result));
                    break;
                }
            }   
            ret = BLE_EVENT_CONTINUE;
        }
        break;
    case OPT_GATT_WRITE:
        printf_socket(sock, "write: connection: 0x%.2x, uuid: 0x%.4X", args->gatt.connection, args->gatt.uuid);
        if(args->gatt.connection == 0x100){
            if(args->gatt.descriptor){
                printf_socket(sock, "Not Supported to Write Local Descriptor");
                break;
            }
            result = gatt_write_local_attribute_by_uuid(ntohs(args->gatt.uuid), 
                    args->gatt.write.value.size, args->gatt.write.value.value);
            if(result){
                printf_socket(sock, "write failed: %s\n", error_summary(result));
            }
        }else{
            conn = connection_find_by_conn(args->gatt.connection);
            if(!conn){
                printf_socket(sock, "Connection Not Found");
                break;
            }
            character = characteristic_find_by_uuid(&conn->characteristic_list, args->gatt.uuid);
            if(!character){
                printf_socket(sock, "Characteristic Not Found");
                break;
            }

            if(args->gatt.descriptor){
                descriptor = descriptor_find_by_handle(&conn->descriptor_list, character->handle, args->gatt.descriptor);
                if(!descriptor){
                    printf_socket(sock, "Descriptor Not Found");
                    break;
                }
                result = gecko_cmd_gatt_write_descriptor_value(conn->connection, descriptor->handle,
                        args->gatt.write.value.size, args->gatt.write.value.value)->result;
                if(result){
                    printf_socket(sock, "Error(%d): %s", result, error_summary(result));
                    break;
                }
                ret = BLE_EVENT_CONTINUE;
            }else{
                if(character->properties & gatt_char_prop_writenoresp){
                    result = gecko_cmd_gatt_write_characteristic_value_without_response(
                            conn->connection, character->handle, 
                            args->gatt.write.value.size, 
                            args->gatt.write.value.value)->result;
                    gecko_cmd_gatt_execute_characteristic_value_write(conn->connection, gatt_commit);
                }else{
                    result = gecko_cmd_gatt_write_characteristic_value(
                            conn->connection, character->handle, 
                            args->gatt.write.value.size, 
                            args->gatt.write.value.value)->result;
                    ret = BLE_EVENT_CONTINUE;
                }
                if(result){
                    printf_socket(sock, "Error(%d): %s", result, error_summary(result));
                    break;
                }
            }
        }
        break;

    case OPT_GATT_NOTIFY:
        if(args->gatt.connection == 0x100){
            printf_socket(sock, "connection must be given");
            break;
        }
        
        if(args->gatt.connection != 0xFF){
            conn = connection_find_by_conn(args->gatt.connection);
            if(!conn){
                printf_socket(sock, "Connection Not Found");
                break;
            }
            connid = conn->connection;
        }else{
            connid = args->gatt.connection;
        }
        
        result = gatt_find_local_attribute(ntohs(args->gatt.uuid), &characteristic);
        if(result){
            if(result > 0){
                printf_socket(sock, "failed to send notification: %s", error_summary(result));
            }else{
                printf_socket(sock, "failed to send notification");
            }
            break;
        }
        result = notification_send(connid, characteristic, 
                args->gatt.notify.value.size, args->gatt.notify.value.value);
        if(result > 0) {
            printf_socket(sock, "Error(%d): %s", result, error_summary(result));
            break;
        }
        break;
    default:
        ret = BLE_EVENT_CONTINUE;
        printf_socket(sock, "Unknown GATT command");
        break;
    }

    return ret;
}

int gatt_event_handler(struct sock_t* sock, struct option_args_t* args, struct gecko_cmd_packet *evt)
{
    int ret = BLE_EVENT_CONTINUE;
    char sbuffer[515] = "";
    struct connection_t* conn;
    struct descriptor_t* descriptor;
    struct characteristic_t* character;
    struct gecko_msg_gatt_characteristic_value_evt_t* read_evt;
    struct gecko_msg_gatt_procedure_completed_evt_t* complete_evt;
    struct gecko_msg_gatt_descriptor_value_evt_t* descriptor_value_evt;
    struct gecko_msg_gatt_server_characteristic_status_evt_t* character_status_evt;

    switch(BGLIB_MSG_ID(evt->header)){
    case gecko_evt_gatt_characteristic_value_id:
        read_evt = &evt->data.evt_gatt_characteristic_value;
        if(args->gatt.connection != read_evt->connection){
            printf_socket(sock, "quit from here 1");
            return BLE_EVENT_CONTINUE;
        }else if(read_evt->att_opcode != gatt_read_response){
            printf_socket(sock, "quit from here 2: %d", read_evt->att_opcode);
            return BLE_EVENT_CONTINUE;
        }

        conn = connection_find_by_conn(args->gatt.connection);
        if(!conn){
            printf_socket(sock, "Connection Down");
            return BLE_EVENT_RETURN;
        }

        character = characteristic_find_by_uuid(&conn->characteristic_list, args->gatt.uuid);
        if(!character){
            printf_socket(sock, "Characteristic Not Found: Unknown Reason");
            return BLE_EVENT_RETURN;
        }
        
        if(character->handle == read_evt->characteristic){
            hex2str(&read_evt->value.data[read_evt->offset], read_evt->value.len, sbuffer);
            printf_socket(sock, "value: %s", sbuffer);
            ret = BLE_EVENT_RETURN;
        }
        break;
 
    case gecko_evt_gatt_server_characteristic_status_id:
        character_status_evt = &evt->data.evt_gatt_server_characteristic_status;
        conn = connection_find_by_conn(character_status_evt->connection);
        if(!conn){
            printf_socket(sock, "Connection Down");
        }else{
            notification_characteristic_add(&conn->notification_list, character_status_evt);
        }
        ret = BLE_EVENT_RETURN;
        break;

    case gecko_evt_gatt_descriptor_value_id:
        descriptor_value_evt = &evt->data.evt_gatt_descriptor_value;
        conn = connection_find_by_conn(descriptor_value_evt->connection);
        if(!conn){
            break;
        }
        character = characteristic_find_by_uuid(&conn->characteristic_list, args->gatt.uuid);
        if(!character){
            break;
        }
        descriptor = descriptor_find_by_handle(&conn->descriptor_list, 
                character->handle,
                descriptor_value_evt->descriptor);
        if(!descriptor){
            break;
        }
        hex2str(descriptor_value_evt->value.data, descriptor_value_evt->value.len, sbuffer);
        printf_socket(sock, "value: %s", sbuffer);

        //info("Service: %.4X -> Characteristic: %.4X ->  Descriptor: %.4X -> Value: %s", 
        //        ntohs(descriptor->characteristic->service->uuid),
        //        ntohs(descriptor->characteristic->uuid),
        //        ntohs(descriptor->uuid), sbuffer);
        break;

    case gecko_evt_gatt_procedure_completed_id:
        complete_evt = &evt->data.evt_gatt_procedure_completed;
        if(complete_evt->result){
            printf_socket(sock, "Error(%d): %s", complete_evt->result, error_summary(complete_evt->result));
        }
        ret = BLE_EVENT_RETURN;
        break;
    default:
        break;
    }

    return ret;
}

int gatt_cleanup(struct sock_t* sock, struct option_args_t* args)
{
    return 0;
}

