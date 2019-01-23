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

extern const char* error_summary(int result);

void show_pkt(const char* data, size_t len)
{

}

void show_result(int result, const char* fmt, ...)
{

}

static void dtm_tx_init(const struct option_args_t* args)
{
    uint8_t  pkttype = args->dtm.tx.pkttype;
    uint32_t pktlen  = args->dtm.tx.pktlen;
    uint8_t  channel = args->dtm.tx.channel;
    uint8_t  phy = args->dtm.tx.phy;
 
    printf(">>>>>>>>> start tx test <<<<<<<<<\n\tsend tx command\n");
    gecko_cmd_system_set_tx_power(args->dev.txpwr);
    
    txrsptr = gecko_cmd_test_dtm_tx(pkttype, pktlen, channel, phy);
    
    delay_time = args->dtm.tx.delay / 1000;

    gecko_cmd_hardware_set_soft_timer(0, 0, 0);
    gecko_cmd_hardware_set_soft_timer(32768,0,0);
}

static void dtm_rx_init(const struct option_args_t* args)
{
    uint8_t channel = args->dtm.rx.channel;
    uint8_t phy = args->dtm.rx.phy;

    printf(">>>>>>>>> start rx test <<<<<<<<<\n\tsend rx command\n");
    gecko_cmd_system_set_tx_power(0);
    
    rxrsptr = gecko_cmd_test_dtm_rx(channel, phy);
    
    delay_time = args->dtm.rx.delay / 1000;

    gecko_cmd_hardware_set_soft_timer(0, 0, 0);
    gecko_cmd_hardware_set_soft_timer(32768, 0, 0);
}

static void dtm_do_action(const struct option_args_t* args, struct gecko_cmd_packet* evt)
{
    if(dtm_state == DTM_TX_INIT){
        dtm_tx_init(args);
        dtm_state = DTM_TX_START;
    }else if(dtm_state == DTM_RX_INIT){
        dtm_rx_init(args);
        dtm_state = DTM_RX_START;
    }else if(dtm_state == DTM_TX_START){
        completed_evt = &evt->data.evt_test_dtm_completed;
        endrsptr = gecko_cmd_test_dtm_end();
        dtm_state = DTM_TX_END;
    }else if(dtm_state == DTM_RX_START){
        completed_evt = &evt->data.evt_test_dtm_completed;
        endrsptr = gecko_cmd_test_dtm_end();
        dtm_state = DTM_RX_END;
    }else if(dtm_state == DTM_TX_END){
        completed_evt = &evt->data.evt_test_dtm_completed;
        ;//show result
        if(args->dtm.tx.on){
            dtm_state = DTM_RX_INIT;
        }
    }else if(dtm_state == DTM_RX_END){
        completed_evt = &evt->data.evt_test_dtm_completed;
        ;//show result
        exit(0);
    }
}

int dtm_event_handler(struct gecko_cmd_packet *evt, const struct option_args_t* args)
{
    int msg_id = BGLIB_MSG_ID(evt->header);

    switch(msg_id){
    case gecko_evt_system_boot_id:
        if(args->dtm.tx.on)
            dtm_state = DTM_TX_INIT;
        else
            dtm_state = DTM_RX_INIT;
        dtm_do_action(args, evt);
        break;

    case gecko_evt_hardware_soft_timer_id:
        if((--delay_time) == 0){
            dtm_do_action(args, evt);
        }
        break;

    case gecko_evt_test_dtm_completed_id:
        dtm_do_action(args, evt);
        break;

    default:
        break;
    }

    return 0;
}

