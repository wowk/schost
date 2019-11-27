#ifndef SC_HOST_UTIL_H__
#define SC_HOST_UTIL_H__

#include "debug.h"
#include <stdio.h>
#include <stdlib.h>

#define BLE_INFO       "/tmp/ble/"
#define BLE_VERSION    BLE_INFO"version"
#define BLE_ADDRESS    BLE_INFO"address"
#define BLE_NEWADDR    BLE_INFO"newaddr"
#define BLE_NEIGHBORS  BLE_INFO"neighbors"
#define BLE_CONNECTION BLE_INFO"connection"
#define BLE_DEVNAME    BLE_INFO"devname"

extern char* btaddr2str(void* addr,  char* buf);
extern void* str2btaddr(char* str, void* buf);
extern void echo(int append, const char* file, const char* format, ...);
extern size_t cat(const char* file, char** pdata, size_t len);
extern void show_file(FILE* fp, const char* file, size_t len);
const const char* error_summary(int result);
int process_running(const char* pidfile);

#endif
