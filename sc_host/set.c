#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <netinet/ether.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>
#include "bg_types.h"
#include "gecko_bglib.h"
#include "host_gecko.h"
#include "app.h"
#include "util.h"


int set_cmd_handler(struct sock_t* sock, struct option_args_t* args)
{
    if(args->set.address[0] != 0){
        bd_addr addr;
        //memcpy(&addr, ether_aton((char*)args->set.address), 6);
        str2btaddr((char*)args->set.address, &addr);      
        struct gecko_msg_system_set_bt_address_rsp_t* result = gecko_cmd_system_set_bt_address(addr);
        if (result->result) {
            printf_socket(sock, "Failed to assign BT address: %s", error_summary(result->result));
        } else {
            printf_socket(sock, "set BT addr successfully");
        }
    }

    if(args->set.name[0] != 0){
        gecko_cmd_system_set_device_name(0, strlen((char*)args->set.name), args->set.name);
        printf_socket(sock, "Device Name: %s", args->set.name);
    }

    return BLE_EVENT_STOP;
}

int set_cleanup(struct sock_t* sock, struct option_args_t* args)
{
    /* after reset, MAC address will be updated */
    ble_system_reset(0);
    return 0;
}
