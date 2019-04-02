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


int pair_cmd_handler(struct sock_t* sock, struct option_args_t* args)
{
    gecko_cmd_system_reset(0);
    return BLE_EVENT_CONTINUE;
}

int pair_event_handler(struct sock_t* sock, struct option_args_t* args, struct gecko_cmd_packet *evt)
{
    int handle = args->pair.handle;
    int ret = BLE_EVENT_CONTINUE;

    switch (BGLIB_MSG_ID(evt->header)) {
    case gecko_evt_system_boot_id:
        gecko_cmd_system_set_tx_power(args->dev.txpwr);
        gecko_cmd_le_gap_set_advertise_phy(handle, args->pair.primary_phy, args->pair.second_phy);
        gecko_cmd_le_gap_set_advertise_timing(handle, 20, 1000, 100, 0);
        gecko_cmd_le_gap_set_advertise_tx_power(handle, args->pair.txpwr);
        gecko_cmd_le_gap_start_advertising(handle, le_gap_general_discoverable, le_gap_connectable_scannable);
        struct gecko_msg_system_get_bt_address_rsp_t* btaddr = gecko_cmd_system_get_bt_address();
        debug(args->debug, "BT address: %s", ether_ntoa((struct ether_addr*)btaddr));
        printf_socket(sock, "enter pairing mode");
        ret = BLE_EVENT_RETURN;
        break;

    case gecko_evt_le_connection_opened_id:
        debug(args->debug, "connection opened");
        break;

    case gecko_evt_le_connection_parameters_id:
        debug(args->debug, "need connect parameters");
        break;

    case gecko_evt_le_connection_closed_id:
        debug(args->debug, "connection closed");
        gecko_cmd_le_gap_start_advertising(handle, le_gap_general_discoverable, le_gap_connectable_scannable);
        break;

    default:
        break;
    }

    return ret;
}

int pair_cleanup(struct sock_t* sock, struct option_args_t* args)
{
    gecko_cmd_le_gap_stop_advertising(0);

    return 0;
}
