#ifndef BLE_DISCOVER_H_
#define BLE_DISCOVER_H_

struct discover_reuqest_t {
    
};

int discover_request_add(struct discover_reuqest_t* dis);
int discover_request_del(struct discover_reuqest_t* dis);
void discover_request_list_clear();

#endif
