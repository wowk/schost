#include "debug.h"
#include "sock.h"
#include "args.h"
#include <notify.h>
#include <connection.h>



int notification_characteristic_add(struct notification_characteristic_list_t* list, 
        struct gecko_msg_gatt_server_characteristic_status_evt_t* evt)
{
    struct notification_characteristic_t* n;
    
    n = notification_characteristic_find(list, evt->characteristic);
    switch(evt->status_flags){
    case gatt_server_confirmation:
        if(n){
            n->waiting_confirm = 0;
            return 0;
        }
        break;
    case gatt_server_client_config:
        if(evt->client_config_flags == gatt_disable){
            notification_characteristic_del(list, n->characteristic);
            return 0;
        }
        break;
    default:
        return 0;
    }
    
    if(!n) {
        n = (struct notification_characteristic_t*)calloc(1, sizeof(*n));
        if(!n){
            return -1;
        }
        LIST_INSERT_HEAD(list, n, entry);
    }

    //info("Connection: %d, Characteristic: %d", evt->connection, evt->characteristic);
    n->waiting_confirm = 0;
    n->connection       = evt->connection;
    n->characteristic   = evt->characteristic;
    n->client_config_flags = evt->client_config_flags;

    return 0;
}

void notification_characteristic_del(struct notification_characteristic_list_t* list, uint16_t characteristic)
{
    struct notification_characteristic_t* n;

    n = notification_characteristic_find(list, characteristic);
    if(n){
        LIST_REMOVE(n, entry);
        free(n);
    }
}

void notification_characteristic_clear(struct notification_characteristic_list_t* list)
{
    struct notification_characteristic_t* n;
    struct notification_characteristic_t* d = NULL;

    LIST_FOREACH(n, list, entry) {
        if(d){
            LIST_REMOVE(d, entry);
            free(d);
            d = NULL;
        }
        d = n;
    }
    if(d){
        LIST_REMOVE(d, entry);
        free(d);
    }
}

struct notification_characteristic_t* 
notification_characteristic_find(struct notification_characteristic_list_t* list, uint16_t characteristic)
{
    struct notification_characteristic_t* n;
    
    LIST_FOREACH(n, list, entry) {
        //info("Find: %d, %d", n->connection, n->characteristic);
        if(n->connection == list->conn->connection && n->characteristic == characteristic){
            return n;
        }
    }
    
    return NULL;
}

int notification_send(uint8_t connection, uint16_t characteristic, uint8_t len, void* data)
{
    int result;
    struct connection_t* conn;
    //struct notification_characteristic_t* n;
    
    conn = connection_find_by_conn(connection);
    if(!conn) {
        info("Connection Not Found");
        return -1;
    }

#if 0
    n = notification_characteristic_find(&conn->notification_list, characteristic);
    if(!n){
        info("n=%p, confirm=%d", n, n?n->waiting_confirm:999);
        return -1;
    }
#endif

    result = gecko_cmd_gatt_server_send_characteristic_notification(conn->connection, characteristic, len, data)->result;
    if(result){
        return result;    
    }
    
    //n->waiting_confirm = 1;

    return 0;
}
