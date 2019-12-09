#ifndef BLE_NOTIFICATION_H_
#define BLE_NOTIFICATION_H_

#include <stdint.h>
#include <stdlib.h>
#include <sys/queue.h>
#include <host_gecko.h>


struct notification_characteristic_t {
    uint8_t connection;
    uint8_t status_flags;
    uint16_t characteristic;
    uint16_t client_config_flags;
    uint8_t waiting_confirm;
    LIST_ENTRY(notification_characteristic_t) entry;
};

struct connection_t;

struct notification_characteristic_list_t {
    struct connection_t* conn;
    struct notification_characteristic_t* lh_first;
};

int notification_characteristic_add(struct notification_characteristic_list_t* list, 
        struct gecko_msg_gatt_server_characteristic_status_evt_t* evt);

void notification_characteristic_del(struct notification_characteristic_list_t*, uint16_t characteristic);

struct notification_characteristic_t* 
notification_characteristic_find(struct notification_characteristic_list_t* list, uint16_t characteristic);

void notification_characteristic_clear(struct notification_characteristic_list_t*);

int notification_send(uint8_t connection, uint16_t characteristic, uint8_t len, void* data);

#endif
