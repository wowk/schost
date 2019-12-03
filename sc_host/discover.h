#ifndef BLE_DISCOVER_H_
#define BLE_DISCOVER_H_

#include <args.h>
#include <sock.h>
#include <host_gecko.h>
#include <discover.h>
#include <stdint.h>

enum DiscoverReqEnum {
    DISCOVER_CHARACTERISTIC,
    DISCOVER_DESCRIPTORS,
    DISCOVER_SERVICES,
};

struct discover_request_t {
    uint8_t type;
    uint8_t result;
    uint8_t connection;
    uint32_t service;
    uint16_t characteristic;
    uint16_t descriptor;
    uint8_t expired;
};

#ifdef __cplusplus
extern "C" {
#endif

int discover_services(uint8_t connection);
int discover_characteristics(uint8_t connection, uint32_t service);
int discover_descriptors(uint8_t connection, uint16_t characteristic);

#ifdef __cplusplus
};
#endif

#endif
