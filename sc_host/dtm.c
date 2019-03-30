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
    return 0;
}

static void dtm_tx_init(struct sock_t* sock, const struct option_args_t* args)
{
    const char* msg;
    uint8_t  pkttype = args->dtm.tx.pkttype;
    uint32_t pktlen  = args->dtm.tx.pktlen;
    uint8_t  channel = args->dtm.tx.channel;
    uint8_t  phy = args->dtm.tx.phy;
 
    msg = ">>>>>>>>> start tx test <<<<<<<<<\n\tsend tx command\n";
    send_socket(sock, 0, 1, msg, strlen(msg));

    gecko_cmd_system_set_tx_power(args->dev.txpwr);
    
    txrsptr = gecko_cmd_test_dtm_tx(pkttype, pktlen, channel, phy);
    
    delay_time = args->dtm.tx.delay / 1000;

    gecko_cmd_hardware_set_soft_timer(0, 0, 0);
    gecko_cmd_hardware_set_soft_timer(32768,0,0);
}

static void dtm_rx_init(struct sock_t* sock, const struct option_args_t* args)
{
    const char* msg;

    uint8_t channel = args->dtm.rx.channel;
    uint8_t phy = args->dtm.rx.phy;

    msg = ">>>>>>>>> start rx test <<<<<<<<<\n\tsend rx command\n";
    send_socket(sock, 0, 1, msg, strlen(msg));

    gecko_cmd_system_set_tx_power(0);
    
    rxrsptr = gecko_cmd_test_dtm_rx(channel, phy);
    
    delay_time = args->dtm.rx.delay / 1000;
    gecko_cmd_hardware_set_soft_timer(0, 0, 0);
    gecko_cmd_hardware_set_soft_timer(32768, 0, 0);
}

static void dtm_do_action(struct sock_t* sock, struct option_args_t* args, struct gecko_cmd_packet* evt)
{
    char msg[128] = "";

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
        snprintf(msg, sizeof(msg), "total %u pkts were sent\n", completed_evt->number_of_packets);
        send_socket(sock, 0, 1, msg, sizeof(msg));
        args->dtm.tx.on = 0;
        if(args->dtm.rx.on){
            dtm_state = DTM_RX_INIT;
            dtm_do_action(sock, args, NULL);
        }
    }else if(dtm_state == DTM_RX_END){
        completed_evt = &evt->data.evt_test_dtm_completed;
        snprintf(msg, sizeof(msg), "total %u pkts were received\n", completed_evt->number_of_packets);
        send_socket(sock, 0, 1, msg, sizeof(msg));
        args->dtm.rx.on = 0;
    }
}

int dtm_event_handler(struct sock_t* sock, struct option_args_t* args, struct gecko_cmd_packet *evt)
{
    switch(BGLIB_MSG_ID(evt->header)){
    case gecko_evt_system_boot_id:
        if(args->dtm.tx.on)
            dtm_state = DTM_TX_INIT;
        else
            dtm_state = DTM_RX_INIT;
        dtm_do_action(sock, args, evt);
        break;

    case gecko_evt_hardware_soft_timer_id:
        if((--delay_time) == 0){
            //dtm_do_action(args, evt);
            endrsptr = gecko_cmd_test_dtm_end();
        }
        printf("\ttimer's up: %d\n", delay_time);
        break;

    case gecko_evt_test_dtm_completed_id:
        dtm_do_action(sock, args, evt);
        break;

    default:
        break;
    }
    
    return (args->dtm.tx.on == 0 && args->dtm.rx.on == 0);
}
