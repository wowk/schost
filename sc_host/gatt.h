#ifndef SC_GATT_H_
#define SC_GATT_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C"
#endif

struct uint8array;

struct gatt_attr_t {
    uint16_t uuid;
    uint16_t attr;
};

int gatt_find_local_attribute(uint16_t uuid, uint16_t* attr);
int gatt_find_local_attributes(struct gatt_attr_t* gas, int size);

int gatt_read_local_attribute(uint16_t attr, uint8_t* len, uint8_t* buf);
int gatt_read_local_attribute_by_uuid(uint16_t uuid, uint8_t* size, uint8_t* buf);
int gatt_write_local_attribute(uint16_t attr, uint8_t size, uint8_t* data);
int gatt_write_local_attribute_by_uuid(uint16_t uuid, uint8_t size, uint8_t* data);

#ifdef __cplusplus
}
#endif


#endif
