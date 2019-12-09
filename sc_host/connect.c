#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <netinet/ether.h>
#include "host_gecko.h"
#include "util.h"
#include "app.h"
#include "args.h"
#include "sock.h"
#include "timer.h"
#include "connection.h"

#define CONNECT_TIMER_ID        0x70
#define CONNECT_TIMER_INTERVAL  1.0f
#define CONNECT_TIMER_TIMEOUT   10

static int connect_timer_handler(struct hw_timer_t*);

enum {
    CONNECT_STOPPED,
    CONNECT_RUNNING,
};

struct connect_timer_arg_t {
    struct sock_t* sock;
    uint8_t connection;
    uint8_t state;
} connect_timer_arg;

struct hw_timer_t connect_timer = {
    .id         = CONNECT_TIMER_ID,
    .interval   = CONNECT_TIMER_INTERVAL,
    .arg        = &connect_timer_arg,
    .ret        = (void*)BLE_EVENT_CONTINUE,
    .callback   = connect_timer_handler,
};

static int connect_timer_handler(struct hw_timer_t* t)
{
    struct connect_timer_arg_t* cta = (struct connect_timer_arg_t*)t->arg;

    info("Connect Timer: %d", t->count);
    if(t->count == 0) {
        cta->state = CONNECT_STOPPED; 
        gecko_cmd_le_connection_close(cta->connection);
        printf_socket(cta->sock, "Connecting Timeout");
        return BLE_EVENT_RETURN;
    }

    return BLE_EVENT_CONTINUE;
}

static int connect_address(struct sock_t* sock, struct option_args_t* args, uint8_t* connection)
{
    bd_addr addr;
    struct connection_t* conn;
    struct gecko_msg_le_gap_connect_rsp_t* rsp;

    str2btaddr((char*)args->connect.address, &addr);
    conn = connection_find_by_addr(&addr);
    if(conn){
        printf_socket(sock, "Already Connected");
        return BLE_EVENT_RETURN;
    }

    rsp = gecko_cmd_le_gap_connect(addr, args->connect.addrtype, args->connect.initphy);
    if(rsp->result){
        printf_socket(sock, "Failed to Connecting %s: %s", (char*)args->connect.address, error_summary(rsp->result));
        return BLE_EVENT_RETURN;
    }
    *connection = rsp->connection;
    printf_socket(sock, "Connecting %s", (char*)args->connect.address);

    return BLE_EVENT_CONTINUE;
}

int connect_bootup_handler(struct sock_t* sock, struct option_args_t* args)
{
#if 0
    int result = gecko_cmd_le_gap_set_conn_parameters(0x10, 0x28, 0, 1000)->result;
    if(result){
        info("connect param: %s", error_summary(result));
    }else{
        info("connect param success");
    }
#endif
    return 0;
}

int connect_cmd_handler(struct sock_t* sock, struct option_args_t* args)
{
    int ret = BLE_EVENT_RETURN;

    if(args->connect.show || args->connect.disconn) {
        args->connection.list = args->connect.show;
        args->connection.disconn = args->connect.disconn;
        ret = connection_cmd_handler(sock, args);
    }else if(args->connect.address[0]){
        connect_timer.ret   = (void*)BLE_EVENT_CONTINUE;
        connect_timer.count = CONNECT_TIMER_TIMEOUT;
        connect_timer_arg.state = CONNECT_RUNNING;
        connect_timer_arg.sock = sock;
        ret = connect_address(sock, args, &connect_timer_arg.connection);
        if(ret != BLE_EVENT_RETURN){
            hw_timer_add(&connect_timer);
        }
    }else{
        printf_socket(sock, "connect ???");
    }

    return ret;
}

int connect_event_handler(struct sock_t* sock, struct option_args_t* args, struct gecko_cmd_packet *evt)
{
    struct gecko_msg_le_connection_opened_evt_t* opened_evt;
    struct gecko_msg_le_connection_closed_evt_t* closed_evt;

    switch (BGLIB_MSG_ID(evt->header)) {
    case gecko_evt_le_connection_opened_id:
        opened_evt = &evt->data.evt_le_connection_opened;
        connection_opened(opened_evt);
        if(connect_timer_arg.state == CONNECT_RUNNING && opened_evt->connection == connect_timer_arg.connection){
            connect_timer_arg.state = CONNECT_STOPPED;
            hw_timer_del(&connect_timer);
            return BLE_EVENT_RETURN;
        }
        break;

    case gecko_evt_le_connection_parameters_id:
        debug(args->debug, "need connect parameters");
        break;

    case gecko_evt_le_connection_closed_id:
        closed_evt = &evt->data.evt_le_connection_closed;
        connection_closed(closed_evt->connection, closed_evt->reason);
        gecko_cmd_le_gap_start_advertising(0, le_gap_general_discoverable, le_gap_connectable_scannable);
        break;

    default:
        break;
    }

    return (int)connect_timer.ret;
}
