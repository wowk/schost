#ifndef BLE_TIMER_H_
#define BLE_TIMER_H_

#include <stdint.h>


struct gecko_msg_hardware_soft_timer_evt_t;

struct hw_timer_t {
    uint8_t id;
    uint32_t interval;
    uint8_t count;
    int(*callback)(void* arg);
    void* arg;
};

#ifdef __cplusplus
extern "C" {
#endif

int hw_timer_add(struct hw_timer_t* timer);
int hw_timer_mod(uint8_t timerid, float interval);
int hw_timer_del(struct hw_timer_t* timer);
struct hw_timer_t* hw_timer_find(uint8_t timerid);
void hw_timer_list_clear(void);
void hw_timer_list_update(struct gecko_msg_hardware_soft_timer_evt_t* evt);

#ifdef __cplusplus
}
#endif

#endif
