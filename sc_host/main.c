/**
 * This an example application that demonstrates Bluetooth connectivity
 * using BGLIB C function definitions. The example enables Bluetooth
 * advertisements and connections.
 *
 * Most of the functionality in BGAPI uses a request-response-event pattern
 * where the module responds to a command with a command response indicating
 * it has processed the request and then later sending an event indicating
 * the requested operation has been completed. */

#include <errno.h>
#include <getopt.h>
#include <stdint.h>
#include <limits.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <netinet/ether.h>
#include "infrastructure.h"
/* BG stack headers */
#include "gecko_bglib.h"
#include "host_gecko.h"
/* hardware specific headers */
#include "uart.h"

/* application specific files */
#include "app.h"
#include "options.h"
#include "util.h"
#include "event_handler.h"


/***************************************************************************************************
 * Local Macros and Definitions
 **************************************************************************************************/

BGLIB_DEFINE();


static int app_serial_port_init(const struct option_args_t* args, int32_t timeout);
static void on_message_send(uint32_t msg_len, uint8_t *msg_data);
static void app_handle_events(struct gecko_cmd_packet *evt, struct option_args_t* args);
static void system_init_action(void);
static void load_bt_dev_info(void);

int main(int argc, char *argv[])
{
    struct option_args_t args;
    struct gecko_cmd_packet *evt = NULL;


    printf("Build Time: Date: %s, Time: %s\n", __DATE__, __TIME__);

    if ( 0 > parse_args(argc, argv, &args) ) {
        exit(EXIT_FAILURE);
    }

    /* Initialize BGLIB with our output function for sending messages. */
    BGLIB_INITIALIZE_NONBLOCK(on_message_send, uartRx, uartRxPeek);

    /* Initialise serial communication as non-blocking. */
    if (app_serial_port_init(&args, 100) < 0) {
        printf("Non-blocking serial port init failure\n");
        exit(EXIT_FAILURE);
    }

    if (access(BLE_INFO, R_OK) < 0) {
        mkdir(BLE_INFO, 0666);
    }
    
    const char* pidfile = "/var/run/sc_host.pid";
    const char* check_process_cmd = "test -f /proc/`cat /var/run/sc_host.pid`/cmdline";
    if (access(pidfile, R_OK) == 0 && system(check_process_cmd) == 0) {
        /* find another sc_host process */
        if(args.show.on){
            if (args.show.version) {
                show_file(stdout, BLE_VERSION, 0);
            }
            if (args.show.btaddr) {
                show_file(stdout, BLE_ADDRESS, 0);
            }
        }else if (args.set.on) {
            printf("shoud stop exist sc_host process first\n");
        }
        exit(0);
    } else {
        /* no other sc_host process */
        echo(0, pidfile, "%u", (unsigned)getpid());
    }

    /* Flush std output */
    fflush(stdout);
    if (args.debug) {
        printf("Starting up...\nResetting NCP target...\n");
    }

    /* Reset NCP to ensure it gets into a defined state.
     * Once the chip successfully boots, gecko_evt_system_boot_id event should be
     * received. */
    gecko_cmd_system_reset(0);
    
    int msg = 0;
    bool booted = false;

    while (1) {
        /* Check for stack event. */
        evt = gecko_peek_event();
        if ( evt == NULL){
            continue;
        }

        msg = BGLIB_MSG_ID(evt->header);
        if(!booted){
            if(msg == gecko_evt_system_boot_id){
                booted = true;
                system_init_action();
                load_bt_dev_info();
            }else if(msg == gecko_evt_dfu_boot_id){
                booted = true;
            }else{
                if(args.debug){
                    printf("Event: 0x%04x\n", msg);
                }
                usleep(50000);
                continue;
            }
        }

        app_handle_events(evt, &args);
    }

    return -1;
}

/***********************************************************************************************/ /**
*  \brief  Function called when a message needs to be written to the serial port.
*  \param[in] msg_len Length of the message.
*  \param[in] msg_data Message data, including the header.
**************************************************************************************************/
static void on_message_send(uint32_t msg_len, uint8_t *msg_data)
{
    /** Variable for storing function return values. */
    int32_t ret;

    ret = uartTx(msg_len, msg_data);
    if (ret < 0) {
        printf("Failed to write to serial port, ret: %d, errno: %s\n", ret, strerror(errno));
        exit(EXIT_FAILURE);
    }
}

/***********************************************************************************************/ /**
*  \brief  Serial Port initialisation routine.
*  \param[in] argc Argument count.
*  \param[in] argv Buffer contaning Serial Port data.
*  \return  0 on success, -1 on failure.
**************************************************************************************************/
static int app_serial_port_init(const struct option_args_t* args, int32_t timeout)
{
    /* Initialise the serial port with RTS/CTS enabled. */
    return uartOpen((int8_t*)args->dev.name, args->dev.baudrate, args->dev.flowctrl, args->dev.timeout);
}


static void app_handle_events(struct gecko_cmd_packet *evt, struct option_args_t* args)
{
    int msgid = BGLIB_MSG_ID(evt->header);
    if(args->show.on){
        show_event_handler(msgid, evt, args);
        args->show.on = 0;
    }else if(args->set.on){
        set_event_handler(msgid, evt, args);
        args->set.on = 0;
    }else if(args->pair.on){
        pair_event_handler(msgid, evt, args);
    }else if(args->scan.on){
        scan_event_handler(msgid, evt, args);
    }else if(args->connect.on){
        connect_event_handler(msgid, evt, args);
    }else if(args->upgrade.on){
        upgrade_event_handler(msgid, evt, args);
    }else{
        exit(0);
    }
}

static void system_init_action(void)
{
    /* BT advertising may affect other functions,
     * we should stop it after system boot up and
     * start it when we need it */
    printf("Stop advertising\n");
    gecko_cmd_le_gap_stop_advertising(0);
    sleep(1);
}

static void load_bt_dev_info(void)
{
    struct gecko_msg_flash_ps_load_rsp_t* rsp = gecko_cmd_flash_ps_load(0x4001);
    echo(0, BLE_VERSION, "Version: %.2X\n", (uint8_t)rsp->value.data[0]);

    struct gecko_msg_system_get_bt_address_rsp_t* btaddr = gecko_cmd_system_get_bt_address();
    echo(0, BLE_ADDRESS, "BT address: %s\n", ether_ntoa((struct ether_addr*)btaddr));
}
