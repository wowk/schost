#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <netinet/ether.h>
#include "host_gecko.h"
#include "util.h"
#include "args.h"
#include "sock.h"


struct connection_t {
    bool    used;
    uint8_t handle;
};

static struct connection_t conn_tab[256];
static struct gecko_msg_le_gap_connect_rsp_t* connrsptr = NULL;


int connect_cmd_handler(struct sock_t* sock, struct option_args_t* args)
{
    gecko_cmd_system_reset(0);
    return 0;
}

int connect_event_handler(struct sock_t* sock, struct option_args_t* args, struct gecko_cmd_packet *evt)
{
    int done = 0;

    bd_addr addr;
    struct gecko_msg_le_connection_opened_evt_t* opened_evt;
    struct gecko_msg_le_connection_closed_evt_t* closed_evt;
    //struct gecko_msg_le_connection_parameters_evt_t* params_evt;

    switch (BGLIB_MSG_ID(evt->header)){
    case gecko_evt_system_boot_id:
        ether_aton_r((char*)args->connect.address, (struct ether_addr*)&addr);
        //gecko_cmd_le_gap_set_conn_parameters(100, 1000, 1000, 0x0c80);
        connrsptr = gecko_cmd_le_gap_connect(addr, args->connect.addrtype, args->connect.initphy);
        printf_socket(sock, "connect device: %s", (char*)args->connect.address);
        break;

    case gecko_evt_le_connection_opened_id:
        opened_evt = &evt->data.evt_le_connection_opened; 
        conn_tab[opened_evt->connection].used = true;
        conn_tab[opened_evt->connection].handle = opened_evt->connection;
        printf_socket(sock, "connected: %u", connrsptr->connection);
        done = 1;
        break;

    case gecko_evt_le_connection_parameters_id:
        //params_evt = &evt->data.evt_le_connection_parameters;
        info("connect parameters");
        break;

    case gecko_evt_le_connection_rssi_id:
        info("connection rssi");
        break;

    case gecko_evt_le_connection_phy_status_id:
        info("connection phy status");
        break;

    case gecko_evt_le_connection_closed_id:
        closed_evt = &evt->data.evt_le_connection_closed;
        conn_tab[closed_evt->connection].used = false;
        info("connection closed");
        gecko_cmd_system_reset(0);
        /* Restart general advertising and re-enable connections after disconnection. */
        //gecko_cmd_le_gap_start_advertising(handle, le_gap_general_discoverable, le_gap_undirected_connectable);
        break;

    default:
        break;
    }

    return done;
}
