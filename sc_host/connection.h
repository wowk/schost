#ifndef BLE_CONNECTION_H_
#define BLE_CONNECTION_H_

#include "host_gecko.h"
#include <stdint.h>

struct connection_t {
    bool    used;
    bd_addr address;
    uint8_t addrtype;
    uint8_t master;
    uint8_t connection;
    uint8_t bonding;
    uint8_t advertiser;
    uint8_t reason;
};

#ifdef __cplusplus
extern "C" {
#endif

int connection_opened(struct gecko_msg_le_connection_opened_evt_t* evt);
int connection_closed(uint8_t conn, uint16_t reason);
void connection_visit(int(*visit_cb)(struct connection_t*, void*), void* args);
struct connection_t* connection_find_by_addr(bd_addr* addr);
struct connection_t* connection_find_unused(void);
struct connection_t* connection_find_by_conn(int conn);
void connection_clear();

#ifdef __cplusplus
}
#endif

#endif
