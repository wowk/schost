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
/* BG stack headers */
#include "bg_types.h"
#include "gecko_bglib.h"
#include "host_gecko.h"
/* Own header */
#include "timer.h"
#include "app.h"
#include "util.h"
#include <stdarg.h>

#define DTM_TIMER_ID        0x20
#define DTM_TIMER_INTERVAL  1.0f

enum dtm_state_e {
    DTM_NONE, DTM_INIT,
    DTM_TX_INIT, DTM_TX_START, DTM_TX_END,
    DTM_RX_INIT, DTM_RX_START, DTM_RX_END,
};

enum dtm_state_e dtm_state = DTM_NONE;
struct sock_t* client_sock = NULL;
struct gecko_msg_test_dtm_tx_rsp_t* txrsptr = NULL;
struct gecko_msg_test_dtm_rx_rsp_t* rxrsptr = NULL;
struct gecko_msg_test_dtm_completed_evt_t* completed_evt = NULL;

static int dtm_timer_handler(struct hw_timer_t*);

struct dtm_timer_arg_t {
    int tx_on, rx_on;
    int delay;
    struct sock_t* sock;
    struct option_args_t* args;
};
struct dtm_timer_arg_t dtm_timer_arg;

struct hw_timer_t dtm_timer = {
    .id         = DTM_TIMER_ID,
    .interval   = DTM_TIMER_INTERVAL,
    .arg        = &dtm_timer_arg,
    .ret        = BLE_EVENT_IGNORE,
    .count      = HW_TIMER_INFINITY,
    .callback   = dtm_timer_handler,
};

static void dtm_tx_init(struct sock_t* sock, const struct option_args_t* args)
{
    uint8_t  pkttype = args->dtm.tx.pkttype;
    uint32_t pktlen  = args->dtm.tx.pktlen;
    uint8_t  channel = args->dtm.tx.channel;
    uint8_t  phy = args->dtm.tx.phy;
 
    printf_socket(sock, ">>>>>>>>> start tx test <<<<<<<<<");
    printf_socket(sock, "\tsend tx command");
 
    txrsptr = gecko_cmd_test_dtm_tx(pkttype, pktlen, channel, phy);
    
    dtm_timer_arg.delay = args->dtm.tx.delay / 1000;
    hw_timer_add(&dtm_timer);
}

static void dtm_rx_init(struct sock_t* sock, const struct option_args_t* args)
{
    uint8_t channel = args->dtm.rx.channel;
    uint8_t phy = args->dtm.rx.phy;

    printf_socket(sock, ">>>>>>>>> start rx test <<<<<<<<<");
    printf_socket(sock, "\tsend rx command");

    gecko_cmd_system_set_tx_power(0);
    
    rxrsptr = gecko_cmd_test_dtm_rx(channel, phy);
    
    dtm_timer_arg.delay = args->dtm.rx.delay / 1000;
    hw_timer_add(&dtm_timer);
}

static void dtm_do_action(struct sock_t* sock, struct option_args_t* args, struct gecko_cmd_packet* evt)
{
    if(dtm_state == DTM_TX_INIT){
        dtm_tx_init(sock, args);
        dtm_state = DTM_TX_START;
    }else if(dtm_state == DTM_RX_INIT){
        dtm_rx_init(sock, args);
        dtm_state = DTM_RX_START;
    }else if(dtm_state == DTM_TX_START){
        dtm_state = DTM_TX_END;
    }else if(dtm_state == DTM_RX_START){
        dtm_state = DTM_RX_END;
    }else if(dtm_state == DTM_TX_END){
        completed_evt = &evt->data.evt_test_dtm_completed;
        printf_socket(sock, "total %u pkts were sent", completed_evt->number_of_packets);
    }else if(dtm_state == DTM_RX_END){
        completed_evt = &evt->data.evt_test_dtm_completed;
        printf_socket(sock, "total %u pkts were received", completed_evt->number_of_packets);
    }
}

static int dtm_timer_handler(struct hw_timer_t* t)
{
    int ret = BLE_EVENT_CONTINUE;
    struct dtm_timer_arg_t* da = (struct dtm_timer_arg_t*)t->arg;
    
    da->delay --;

    if(da->tx_on == 0 && da->rx_on == 0){
        dtm_state = DTM_NONE;
        ret = BLE_EVENT_STOP;
    }else if(da->delay > 0){
        printf_socket(da->sock, "\ttimer's up: %d", da->delay);
    }else if(da->delay == 0){
        if(da->tx_on){
            da->tx_on = 0;
            dtm_state = DTM_TX_END;
        }else if(da->rx_on){
            da->rx_on = 0;
            dtm_state = DTM_RX_END;
        }
        gecko_cmd_test_dtm_end();
    }else {
        if(da->rx_on){
            dtm_state = DTM_RX_INIT;
            dtm_do_action(da->sock, da->args, NULL);
        }else{
            dtm_state = DTM_NONE;
            ret = BLE_EVENT_STOP;
        }
        // waiting gecko_evt_test_dtm_completed_id:
    }

    return ret;
}

int dtm_bootup_handler(struct sock_t* sock, struct option_args_t* args)
{
    if(dtm_state == DTM_INIT){
        gecko_cmd_system_set_tx_power(args->dev.txpwr);
        if(args->dtm.tx.on)
            dtm_state = DTM_TX_INIT;
        else
            dtm_state = DTM_RX_INIT;
        dtm_do_action(sock, args, NULL);
    }

    return 0;
}

int dtm_cmd_handler(struct sock_t* sock, struct option_args_t* args)
{
    dtm_timer_arg.tx_on = args->dtm.tx.on;
    dtm_timer_arg.rx_on = args->dtm.rx.on;
    dtm_timer_arg.sock = sock;
    dtm_timer_arg.args = args;
    dtm_timer.ret = BLE_EVENT_IGNORE;
    dtm_state = DTM_INIT;
    ble_system_reset(0);

    return BLE_EVENT_CONTINUE;
}

int dtm_event_handler(struct sock_t* sock, struct option_args_t* args, struct gecko_cmd_packet *evt)
{
    switch(BGLIB_MSG_ID(evt->header)){
    case gecko_evt_test_dtm_completed_id:
        dtm_do_action(sock, args, evt);
        info("=======dtm end=====");
        break;

    default:
        break;
    }
    
    return (int)dtm_timer.ret;
}

int dtm_cleanup(struct sock_t* sock, struct option_args_t* args)
{
    info("=======DTM Cleanup=====");

    dtm_state = DTM_NONE;
    dtm_timer.ret = BLE_EVENT_IGNORE;
    hw_timer_del(&dtm_timer);
    
    ble_system_reset(0);

    return 0;
}
