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
#include "gatt.h"
#include "connection.h"


uint8_t handle = 0;

int pair_bootup_handler(struct sock_t* sock, struct option_args_t* args)
{
    return 0;
}

int pair_cmd_handler(struct sock_t* sock, struct option_args_t* args)
{
    gecko_cmd_system_set_tx_power(args->dev.txpwr);
    //gecko_cmd_sm_set_bondable_mode(0);
    gecko_cmd_sm_delete_bondings();
    gecko_cmd_le_gap_set_advertise_phy(handle, args->pair.primary_phy, args->pair.second_phy);
    gecko_cmd_le_gap_set_advertise_timing(handle, 20, 1000, 100, 0);
    gecko_cmd_le_gap_set_advertise_tx_power(handle, args->pair.txpwr);
    gecko_cmd_le_gap_start_advertising(handle, le_gap_general_discoverable, le_gap_connectable_scannable);
    printf_socket(sock, "enter pairing mode");

    return BLE_EVENT_RETURN;
}

int pair_event_handler(struct sock_t* sock, struct option_args_t* args, struct gecko_cmd_packet *evt)
{
    struct gecko_msg_le_connection_closed_evt_t* closed_evt;
    
    switch(BGLIB_MSG_ID(evt->header)){
    case gecko_evt_le_connection_closed_id:
        closed_evt = &evt->data.evt_le_connection_closed;
        connection_closed(closed_evt->connection, closed_evt->reason);
        pair_cmd_handler(sock, args);
        //gecko_cmd_le_gap_start_advertising(handle++, le_gap_general_discoverable, le_gap_connectable_scannable);
        break;
    default:
        break;
    }

    return BLE_EVENT_CONTINUE;
}

int pair_cleanup(struct sock_t* sock, struct option_args_t* args)
{
    gecko_cmd_le_gap_stop_advertising(handle);

    return 0;
}
