#ifndef BLE_NEIGH_H_
#define BLE_NEIGH_H_

#include <sock.h>
#include <host_gecko.h>
#include <stdint.h>
#include <sys/queue.h>

struct neigh_t {
    bd_addr address;
    uint8_t phy_1;
    uint8_t phy_4;
    uint8_t addrtype;
};

#ifdef __cplusplus
extern "C" {
#endif

void neigh_list_visit(int(*callback)(struct neigh_t*, void*), void* arg);
struct neigh_t* neigh_list_find(bd_addr addr);
void neigh_list_add(int phy, struct gecko_msg_le_gap_scan_response_evt_t* rsp);
void neigh_list_clear(void);

#ifdef __cplusplus
}
#endif

#endif
