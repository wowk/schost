#ifndef BLE_SERVICE_H_
#define BLE_SERVICE_H_

#include <sys/queue.h>
#include <host_gecko.h>


struct descriptor_t {
    uint16_t handle;
    char* desc_info;
};

struct characteristic_t {
    uint8array uuid;
    struct service_t* service;
    struct descriptor_t desc;
    struct characteristic_t* next;
};

struct service_t {
    uint8_t connection;
    uint32_t service;
    uint8array uuid;
    struct characteristic_t* characteristics;
};

#if 0
service_add
characteristic_add
descriptor_set;

service_clean();
characteristic_clean();
descriptor_clean();
#endif

#endif
