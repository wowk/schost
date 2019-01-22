#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <netinet/ether.h>
#include "host_gecko.h"
#include "util.h"
#include "event_handler.h"


struct gecko_msg_le_gap_connect_rsp_t* connrsptr = NULL;

int connect_event_handler(int msgid, struct gecko_cmd_packet *evt, const struct option_args_t* args)
{
    switch (msgid) {
    case gecko_evt_system_boot_id:
        remove(BLE_CONNECTION);
        bd_addr addr;
        bd_addr tmp;
        //uint8_t*p1 = (uint8_t*)&addr, *p2 = (uint8_t*)&tmp;
        ether_aton_r((char*)args->connect.address, (struct ether_addr*)&tmp);
        gecko_cmd_le_gap_set_conn_parameters(100, 1000, 1000, 0x0c80);
        connrsptr = gecko_cmd_le_gap_connect(addr, args->connect.addrtype, args->connect.initphy);
        break;

    case gecko_evt_le_connection_opened_id:
        printf("connected: %u\n", connrsptr->connection);
        break;

    case gecko_evt_le_connection_parameters_id:
        printf("need connect parameters\n");
        break;

    case gecko_evt_le_connection_closed_id:
        printf("closed event\n");
        gecko_cmd_system_reset(0);
        /* Restart general advertising and re-enable connections after disconnection. */
        //gecko_cmd_le_gap_start_advertising(handle, le_gap_general_discoverable, le_gap_undirected_connectable);
        break;

    default:
        break;
    }

    return 0;
}
