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
#include "timer.h"
#include "neigh.h"


#define SCAN_TIMER_ID       0x10
#define SCAN_TIMER_INTERVAL 1.0f


static int scan_timer_handler(struct hw_timer_t* arg);

struct hw_timer_t scan_timer = {
    .id         = SCAN_TIMER_ID,
    .interval   = SCAN_TIMER_INTERVAL,
    .arg        = NULL,
    .ret        = BLE_EVENT_IGNORE,
    .callback   = scan_timer_handler,
};

struct dump_neigh_arg_t {
    int id;
    struct sock_t* sock;
};

int dump_neigh(struct neigh_t* neigh, void* arg)
{
    char address[18] = "";
    struct dump_neigh_arg_t* dna = (struct dump_neigh_arg_t*)arg;

    printf_socket(dna->sock, "%-5d%-24s%-16d%-6s", dna->id++, 
            btaddr2str((void*)&neigh->address, address),
            neigh->addrtype, (neigh->phy_1 && neigh->phy_4) ? "1/4" : (neigh->phy_1 ? "1" : "4"));

    return 0;
}

static int scan_timer_handler(struct hw_timer_t* t)
{
    int ret = BLE_EVENT_CONTINUE;
    struct sock_t* sock = (struct sock_t*)t->arg;
    struct dump_neigh_arg_t dna = {
        .id    = 0,
        .sock  = sock,
    };

    if(t->count == 0){
        printf_socket(sock, "%-5s%-24s%-16s%-6s", "ID", "BLE Address", "Address Type", "PHY");
        neigh_list_visit(dump_neigh, &dna);
        neigh_list_clear();
        ret = BLE_EVENT_STOP;
    }
    
    return ret;
}

int  scan_cmd_handler(struct sock_t* sock, struct option_args_t* args)
{
    debug(args->debug, "starting scan neighbor devices");

    gecko_cmd_system_set_tx_power(args->dev.txpwr);
    gecko_cmd_le_gap_set_discovery_timing(args->scan.phy, args->scan.interval, args->scan.winsize);
    gecko_cmd_le_gap_set_discovery_type(args->scan.phy, args->scan.type);
    gecko_cmd_le_gap_start_discovery(args->scan.phy, args->scan.mode);
   
    scan_timer.ret   = BLE_EVENT_IGNORE;
    scan_timer.arg   = sock;
    scan_timer.count = args->scan.timeout;
    hw_timer_add(&scan_timer);

    return BLE_EVENT_CONTINUE;
}

int  scan_event_handler(struct sock_t* sock, struct option_args_t* args, struct gecko_cmd_packet* evt)
{
    struct gecko_msg_le_gap_scan_response_evt_t* scanrsptr = NULL;

    switch (BGLIB_MSG_ID(evt->header)) {
    case gecko_evt_le_gap_scan_response_id:
        scanrsptr = &evt->data.evt_le_gap_scan_response;
        neigh_list_add(args->scan.phy, scanrsptr); 
        break;

    //case gecko_evt_hardware_soft_timer_id:
    //    info("ScanTimer: %d\n", (int)scan_timer.ret);
    //    break;

    default:
        break;
    }
    

    return (int)scan_timer.ret;
}

int scan_cleanup(struct sock_t* sock, struct option_args_t* args)
{
    gecko_cmd_le_gap_end_procedure();
    neigh_list_clear();
    hw_timer_del(&scan_timer);
    scan_timer.ret = BLE_EVENT_IGNORE;
    
    return 0;
}
