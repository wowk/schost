#if 0
notify_list
struct notify_t {
    int conn;
    int character;
    uint8arry value;
};

read_request_list
struct read_request {
    
};


write_request_list


discover_request_list
#endif

#include "debug.h"
#include "timer.h"
#include "host_gecko.h"
#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/queue.h>


struct hw_timer_elem_t {
    struct hw_timer_t hw_timer;
    LIST_ENTRY(hw_timer_elem_t) entry;
};

struct hw_timer_list_t {
    uint32_t count;
	struct hw_timer_elem_t *lh_first;
};

struct hw_timer_list_t hw_timer_list;

struct hw_timer_t* hw_timer_find(uint8_t timerid);

int hw_timer_add(struct hw_timer_t* t)
{
    struct hw_timer_t* timer;
    struct hw_timer_elem_t* timer_elem;

    timer = hw_timer_find(t->id);
    if(timer){
        return -1;
    }

    timer_elem = (struct hw_timer_elem_t*)calloc(1, sizeof(struct hw_timer_elem_t));
    if(!timer_elem){
        return -1;
    }
    timer_elem->hw_timer = *t;
    timer = &timer_elem->hw_timer;
    timer->interval = t->interval *32768;
    gecko_cmd_hardware_set_soft_timer(timer->interval, timer->id, 0);
    LIST_INSERT_HEAD(&hw_timer_list, timer_elem, entry);

    return 0;
}

int hw_timer_mod(uint8_t timer_id, float interval)
{
    struct hw_timer_t* timer;

    timer = hw_timer_find(timer_id);
    if(!timer){
        return -1;
    }

    timer->interval = (uint32_t)(interval*32767);
    gecko_cmd_hardware_set_soft_timer(0, timer->id, 0);
    gecko_cmd_hardware_set_soft_timer(timer->interval, timer->id, 0);

    return 0;
}

int hw_timer_del(struct hw_timer_t* timer)
{
    struct hw_timer_elem_t* elem;
   
    elem = (struct hw_timer_elem_t*)hw_timer_find(timer->id);

    if(elem){
        LIST_REMOVE(elem, entry);
        return 0;
    }

    return -1;
}

void hw_timer_list_update(struct gecko_msg_hardware_soft_timer_evt_t* evt)
{
    struct hw_timer_elem_t* elem;
    struct hw_timer_elem_t* deleted = NULL;
    
    LIST_FOREACH(elem, &hw_timer_list, entry){
        info("%d========%d", elem->hw_timer.id, evt->handle);
        if(elem->hw_timer.id == evt->handle){
            elem->hw_timer.callback(elem->hw_timer.arg);
            if(elem->hw_timer.count == 1){
                deleted = elem;
            }else if(elem->hw_timer.count > 0){
                elem->hw_timer.count --;
            }
            break;
        }
    }

    if(deleted){
        LIST_REMOVE(deleted, entry);
        gecko_cmd_hardware_set_soft_timer(0, deleted->hw_timer.id, 0);
        free(deleted);
    }
}

struct hw_timer_t* hw_timer_find(uint8_t timerid)
{
    struct hw_timer_elem_t* elem;

    LIST_FOREACH(elem, &hw_timer_list, entry){
        if(elem->hw_timer.id == timerid){
            return &elem->hw_timer;
        }
    }

    return NULL;
}

void hw_timer_list_clear()
{
    struct hw_timer_elem_t* elem;
    struct hw_timer_elem_t* deleted = NULL;

    LIST_FOREACH(elem, &hw_timer_list, entry){
        if(deleted){
            LIST_REMOVE(deleted, entry);
            gecko_cmd_hardware_set_soft_timer(0, deleted->hw_timer.id, 0);
            free(deleted);
        }else{
            deleted = elem;
        }
    }
    
    if(deleted){
        LIST_REMOVE(deleted, entry);
        gecko_cmd_hardware_set_soft_timer(0, deleted->hw_timer.id, 0);
        free(deleted);
    }
}
