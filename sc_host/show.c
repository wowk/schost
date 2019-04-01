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
#include "sock.h"

int show_cmd_handler(struct sock_t* sock, struct option_args_t* args)
{
    struct gecko_msg_flash_ps_load_rsp_t* ps_load_rsp;
    struct gecko_msg_system_get_bt_address_rsp_t* get_bt_rsp;

    if(args->show.version){
        ps_load_rsp = gecko_cmd_flash_ps_load(0x4001);
        printf_socket(sock, "Version: %.2x", (uint8_t)ps_load_rsp->value.data[0]);
    }
    
    if (args->show.btaddr) {
        get_bt_rsp = gecko_cmd_system_get_bt_address();
        printf_socket(sock, "BT address: %s", ether_ntoa((struct ether_addr*)get_bt_rsp));
    }

    return 0;
}
