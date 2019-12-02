#ifndef SC_GATT_H_
#define SC_GATT_H_

#include <stdint.h>

#ifdef __cplusplus
extern "C"
#endif

struct gatt_attr_t {
    uint16_t uuid;
    uint16_t attr;
};

int gatt_find_attribute(const uint8_t* uuid, uint16_t* attr);
int gatt_find_attributes(struct gatt_attr_t* gas, int size);
void gatt_write_attribute(uint16_t attr, uint16_t offset, uint8_t value_len, uint8_t* value);

#ifdef __cplusplus
}
#endif


#endif
