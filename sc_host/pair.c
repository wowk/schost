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
    return 0;
}

int pair_event_handler(struct sock_t* sock, struct option_args_t* args, struct gecko_cmd_packet *evt)
{
    bool done = false;

    switch (BGLIB_MSG_ID(evt->header)) {
    case gecko_evt_system_boot_id:
        gecko_cmd_system_set_tx_power(args->dev.txpwr);
        gecko_cmd_le_gap_set_advertise_phy(0, test_phy_1m, test_phy_2m);
        gecko_cmd_le_gap_set_advertise_timing(0, 20, 1000, 100, 0);
        gecko_cmd_le_gap_set_advertise_tx_power(0, args->pair.txpwr);
        gecko_cmd_le_gap_start_advertising(0, le_gap_general_discoverable, le_gap_connectable_scannable);
        struct gecko_msg_system_get_bt_address_rsp_t* btaddr = gecko_cmd_system_get_bt_address();
        printf("BT address: %s\n", ether_ntoa((struct ether_addr*)btaddr));
        break;

    case gecko_evt_le_connection_opened_id:
        done = true;
        printf("connection opened\n");
        break;

    case gecko_evt_le_connection_parameters_id:
        printf("need connect parameters\n");
        break;

    case gecko_evt_le_connection_closed_id:
        printf("closed event\n");
        gecko_cmd_system_reset(0);
        /* Restart general advertising and re-enable connections after disconnection. */
        //gecko_cmd_le_gap_start_advertising(0, le_gap_general_discoverable, le_gap_undirected_connectable);
        break;

    default:
        break;
    }

    return done;
}
