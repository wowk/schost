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
#include "app.h"
#include "util.h"
#include <stdarg.h>

#define DTM_TIMER_ID        0x20
#define DTM_TIMER_INTERVAL  (32768*1)

enum dtm_state_e {
    DTM_TX_INIT, DTM_TX_START, DTM_TX_END,
    DTM_RX_INIT, DTM_RX_START, DTM_RX_END,
};
 
int delay_time = 0;
enum dtm_state_e dtm_state;
struct gecko_msg_test_dtm_tx_rsp_t* txrsptr = NULL;
struct gecko_msg_test_dtm_rx_rsp_t* rxrsptr = NULL;
struct gecko_msg_test_dtm_end_rsp_t* endrsptr = NULL;
struct gecko_msg_test_dtm_completed_evt_t* completed_evt = NULL;


int dtm_cmd_handler(struct sock_t* sock, struct option_args_t* args)
{
    gecko_cmd_system_reset(0);

    return BLE_EVENT_CONTINUE;
}

static void dtm_tx_init(struct sock_t* sock, const struct option_args_t* args)
{
    uint8_t  pkttype = args->dtm.tx.pkttype;
    uint32_t pktlen  = args->dtm.tx.pktlen;
    uint8_t  channel = args->dtm.tx.channel;
    uint8_t  phy = args->dtm.tx.phy;
 
    printf_socket(sock, ">>>>>>>>> start tx test <<<<<<<<<");
    printf_socket(sock, "\tsend tx command");
 
    txrsptr = gecko_cmd_test_dtm_tx(pkttype, pktlen, channel, phy);
    
    delay_time = args->dtm.tx.delay / 1000;

    gecko_cmd_hardware_set_soft_timer(DTM_TIMER_INTERVAL, DTM_TIMER_ID, 0);
}

static void dtm_rx_init(struct sock_t* sock, const struct option_args_t* args)
{
    uint8_t channel = args->dtm.rx.channel;
    uint8_t phy = args->dtm.rx.phy;

    printf_socket(sock, ">>>>>>>>> start rx test <<<<<<<<<");
    printf_socket(sock, "\tsend rx command");

    gecko_cmd_system_set_tx_power(0);
    
    rxrsptr = gecko_cmd_test_dtm_rx(channel, phy);
    
    delay_time = args->dtm.rx.delay / 1000;
    gecko_cmd_hardware_set_soft_timer(0, 0, 0);
    gecko_cmd_hardware_set_soft_timer(32768, 0, 0);
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
        args->dtm.tx.on = 0;
        if(args->dtm.rx.on){
            dtm_state = DTM_RX_INIT;
            dtm_do_action(sock, args, NULL);
        }
    }else if(dtm_state == DTM_RX_END){
        completed_evt = &evt->data.evt_test_dtm_completed;
        printf_socket(sock, "total %u pkts were received", completed_evt->number_of_packets);
        args->dtm.rx.on = 0;
    }
}

int dtm_event_handler(struct sock_t* sock, struct option_args_t* args, struct gecko_cmd_packet *evt)
{
    int ret = BLE_EVENT_CONTINUE;
    struct gecko_msg_hardware_soft_timer_evt_t* timer_evt;

    switch(BGLIB_MSG_ID(evt->header)){
    case gecko_evt_system_boot_id:
        gecko_cmd_system_set_tx_power(args->dev.txpwr);
        delay_time = 0;
        if(args->dtm.tx.on)
            dtm_state = DTM_TX_INIT;
        else
            dtm_state = DTM_RX_INIT;
        dtm_do_action(sock, args, evt);
        break;

    case gecko_evt_hardware_soft_timer_id:
        timer_evt = &evt->data.evt_hardware_soft_timer;
        if(timer_evt->handle != DTM_TIMER_ID){
            break;
        }
        if((--delay_time) == 0){
            //dtm_do_action(args, evt);
            endrsptr = gecko_cmd_test_dtm_end();
        }
        printf_socket(sock, "\ttimer's up: %d", delay_time);
        break;

    case gecko_evt_test_dtm_completed_id:
        dtm_do_action(sock, args, evt);
        break;

    default:
        break;
    }
    
    if(args->dtm.tx.on == 0 && args->dtm.rx.on == 0){
        ret = BLE_EVENT_STOP;
    }

    return ret;
}

int dtm_cleanup(struct sock_t* sock, struct option_args_t* args)
{
    gecko_cmd_system_reset(0);
    return 0;
}
