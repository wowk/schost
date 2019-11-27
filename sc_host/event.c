#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
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
#include <sys/queue.h>
#include <sys/select.h>
#include "bg_types.h"
#include "gecko_bglib.h"
#include "host_gecko.h"
#include "app.h"
#include "util.h"
#include "sock.h"


static int scan_time = 0;

struct neigh_entry_t {
    LIST_ENTRY(neigh_entry_t) entry;
    bd_addr address;
    uint8_t phy_1;
    uint8_t phy_4;
    uint8_t addrtype;
};
LIST_HEAD(neigh_list_t, neigh_entry_t) neigh_list ;

static void add_neigh_to_list(int phy, struct neigh_list_t* list, struct gecko_msg_le_gap_scan_response_evt_t* rsp)
{
    struct neigh_entry_t* neigh = NULL;

    if(LIST_EMPTY(list) == 0){
        LIST_FOREACH(neigh, list, entry){
            if(!memcmp(&neigh->address, &rsp->address, sizeof(bd_addr))){
                break;
            }
        }
    }
    
    if(!neigh) {
        neigh = (struct neigh_entry_t*)calloc(1, sizeof(struct neigh_entry_t));
        if(!neigh){
            return;
        }
        LIST_INSERT_HEAD(list, neigh, entry);
        memcpy(&neigh->address, &rsp->address, sizeof(bd_addr));
        neigh->addrtype = rsp->address_type;
    }
    
    if(phy == 1){
        neigh->phy_1 = 1;
    }else if(phy == 4){
        neigh->phy_4 = 1;
    }
}

static void dump_neigh_list(struct sock_t* sock, struct neigh_list_t* list)
{
    int id = 0;
    char address[18] = "";
    struct neigh_entry_t* neigh = NULL;

    printf_socket(sock, "%-5s%-24s%-16s%-6s", "ID", "BLE Address", "Address Type", "PHY");

    LIST_FOREACH(neigh, list, entry) {
        printf_socket(sock, "%-5d%-24s%-16d%-6s", id++, 
                btaddr2str((void*)&neigh->address, address),
                neigh->addrtype, (neigh->phy_1 && neigh->phy_4) ? "1/4" : (neigh->phy_1 ? "1" : "4"));
    }
}

static void free_neigh_list(struct neigh_list_t* list)
{
    struct neigh_entry_t* p;
    while((p = LIST_FIRST(list))){
        LIST_REMOVE(p, entry);
        free(p);
    }
}

int  scan_cmd_handler(struct sock_t* sock, struct option_args_t* args)
{
    debug(args->debug, "starting scan neighbor devices");
    scan_time = args->scan.timeout;
    LIST_INIT(&neigh_list);
    gecko_cmd_system_set_tx_power(args->dev.txpwr);
    gecko_cmd_le_gap_set_discovery_timing(args->scan.phy, args->scan.interval, args->scan.winsize);
    gecko_cmd_le_gap_set_discovery_type(args->scan.phy, args->scan.type);
    gecko_cmd_le_gap_start_discovery(args->scan.phy, args->scan.mode);
    gecko_cmd_hardware_set_soft_timer(32768,0,0);

    return BLE_EVENT_CONTINUE;
}

int  scan_event_handler(struct sock_t* sock, struct option_args_t* args, struct gecko_cmd_packet* evt)
{
    int ret = BLE_EVENT_CONTINUE;
    struct gecko_msg_le_gap_scan_response_evt_t* scanrsptr = NULL;


    switch (BGLIB_MSG_ID(evt->header)) {
    case gecko_evt_system_boot_id:
    case gecko_evt_dfu_boot_id:
    case gecko_evt_dfu_boot_failure_id:
    case gecko_evt_le_connection_opened_id:
    case gecko_evt_le_connection_parameters_id:
    case gecko_evt_le_connection_rssi_id:
    case gecko_evt_le_connection_phy_status_id:
    case gecko_evt_le_connection_closed_id:
    case gecko_evt_hardware_soft_timer_id:
    case gecko_evt_test_dtm_completed_id:
    case gecko_evt_le_gap_scan_response_id:
    case gecko_evt_le_gap_scan_request_id:
    case gecko_evt_hardware_soft_timer_id:
        break;
    case gecko_evt_system_boot_id:
        scan_cmd_handler(sock, args);
        break;

    case gecko_evt_le_gap_scan_response_id:
        scanrsptr = &evt->data.evt_le_gap_scan_response;
        add_neigh_to_list(args->scan.phy, &neigh_list, scanrsptr); 
        break;

    case gecko_evt_le_gap_scan_request_id:
        break;

    case gecko_evt_hardware_soft_timer_id:
        if (scan_time == 0) {
            debug(args->debug, "timeout<1>: %d", scan_time);
            dump_neigh_list(sock, &neigh_list);
            free_neigh_list(&neigh_list);
            ret = BLE_EVENT_STOP;
        }else{
            debug(args->debug, "timeout<4>: %d", scan_time);
        }
        scan_time --;
        break;

    default:
        break;
    }

    return ret;
}

int scan_cleanup(struct sock_t* sock, struct option_args_t* args)
{
    gecko_cmd_le_gap_end_procedure();
    return 0;
}
