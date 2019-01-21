#ifndef SC_HOST_UTIL_H__
#define SC_HOST_UTIL_H__

#include <stdlib.h>

#define BLE_INFO       "/tmp/ble/"
#define BLE_VERSION    BLE_INFO"version"
#define BLE_ADDRESS    BLE_INFO"address"
#define BLE_NEWADDR    BLE_INFO"newaddr"
#define BLE_NEIGHBORS  BLE_INFO"neighbors"
#define BLE_CONNECTION BLE_INFO"connection"

extern void echo(int append, const char* file, const char* format, ...);
extern size_t cat(const char* file, char** pdata, size_t len);
extern void show_file(FILE* fp, const char* file, size_t len);

#endif
