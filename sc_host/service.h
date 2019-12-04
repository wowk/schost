#ifndef BLE_SERVICE_H_
#define BLE_SERVICE_H_

#include <stdint.h>
#include <sys/queue.h>


struct gecko_msg_gatt_service_evt_t;
struct gecko_msg_gatt_characteristic_evt_t;
struct gecko_msg_gatt_descriptor_evt_t;


struct descriptor_t {
    uint16_t handle;
    uint8_t len;
    char* info;
};

struct characteristic_t {
    uint16_t handle;
    uint16_t uuid;
    uint8_t properties;
    struct service_t* service;
    struct descriptor_t desc;
    struct characteristic_list_t* head;
    LIST_ENTRY(characteristic_t) entry;
};

struct service_t {
    uint32_t handle;
    uint16_t uuid;
    struct service_list_t* head;
    LIST_ENTRY(service_t) entry;
};

struct service_list_t {
    struct connection_t* conn;
	struct service_t *lh_first;
};

struct characteristic_list_t {
    struct connection_t* conn;
    struct characteristic_t *lh_first;
};


#ifdef __cplusplus
extern "C" {
#endif

int service_add(struct service_list_t* list, struct gecko_msg_gatt_service_evt_t* evt);
int characteristic_add(struct service_t*, struct characteristic_list_t* list, 
        struct gecko_msg_gatt_characteristic_evt_t* evt);
int descriptor_set(struct characteristic_t*, struct gecko_msg_gatt_descriptor_evt_t* evt);

struct service_t* service_find_by_handle(struct service_list_t*, uint32_t handle);
struct service_t* service_find_by_uuid(struct service_list_t*, uint16_t uuid);
struct characteristic_t* characteristic_find_by_uuid(struct characteristic_list_t*, uint16_t uuid);

int service_clean(struct service_list_t*);
int characteristic_clean(struct characteristic_list_t*);

#ifdef __cplusplus
}
#endif


#endif
