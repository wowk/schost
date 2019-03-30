/**
 * This an example application that demonstrates Bluetooth connectivity
 * using BGLIB C function definitions. The example enables Bluetooth
 * advertisements and connections.
 *
 * Most of the functionality in BGAPI uses a request-response-event pattern
 * where the module responds to a command with a command response indicating
 * it has processed the request and then later sending an event indicating
 * the requested operation has been completed. */
#define _GNU_SOURCE

#include <errno.h>
#include <getopt.h>
#include <stdint.h>
#include <limits.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <libgen.h>
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
#include "debug.h"
#include "sock.h"
#include "app.h"
#include "options.h"
#include "util.h"


/***************************************************************************************************
 * Local Macros and Definitions
 **************************************************************************************************/


typedef int(*cmd_handler_cb)(struct sock_t* sock, struct option_args_t* args);
typedef int(*cmd_exit_cb)(struct sock_t* sock, struct option_args_t* args);
typedef int(*event_handler_cb)(struct sock_t* sock, 
        struct option_args_t* args, struct gecko_cmd_packet*);

struct cmd_table_t {
        cmd_handler_cb cmd_handler;
        event_handler_cb event_handler;
        cmd_exit_cb cleanup;
}; 

struct cmd_table_t cmd_tab[] = {
    [0] = {.cmd_handler = show_cmd_handler,    .event_handler = NULL},
    [1] = {.cmd_handler = set_cmd_handler,     .event_handler = NULL},
    [2] = {.cmd_handler = scan_cmd_handler,    .event_handler = scan_event_handler},
    [3] = {.cmd_handler = dtm_cmd_handler,     .event_handler = dtm_event_handler},
    [4] = {.cmd_handler = pair_cmd_handler,    .event_handler = pair_event_handler},
    [5] = {.cmd_handler = connect_cmd_handler, .event_handler = connect_event_handler},
    [6] = {.cmd_handler = upgrade_cmd_handler, .event_handler = upgrade_event_handler},
}; 

BGLIB_DEFINE();


static int app_serial_port_init(const struct option_args_t* args, int32_t timeout);
static void on_message_send(uint32_t msg_len, uint8_t *msg_data);

int schost_main(int argc, char* argv[])
{
    struct option_args_t args;
    if(0 > parse_args(argc, argv, &args)){
        error(1, EINVAL, "failed to parse args");
    }

    unlink("/tmp/.schost");
    
    struct sock_t* sock = create_socket(AF_LOCAL, SOCK_DGRAM, 0);
    if(!sock){
        error(1, errno, "failed to create socket for sc_host");
    }

    if(0 > bind_socket(sock, "/tmp/.schost", 0)){
        error(1, errno, "failed bind to sc_host");
    } 

#if 0
    if(0 > connect_socket(sock, "/tmp/.schostd", 0)){
        error(1, errno, "failed connect to sc_hostd");
    }
#else
    socket_addr(&sock->daddr, AF_LOCAL, "/tmp/.schostd", 0, &sock->socklen);
#endif

    if(0 > send_socket(sock, 0, 1, &args, sizeof(args))){
        error(1, errno, "failed to send command to schostd");
    }

    char buffer[512] = "";
    while(1){
        int retlen = recv_socket(sock, 0, 1, buffer, sizeof(buffer));
        if(retlen < 0){
            error(1, errno, "failed to receive response from schostd");
        }else if(retlen == 0){
            info("all response received");
            break;
        }

        /* output the result received here*/
        info("received data: %s", buffer);
    }

    close_socket(sock);

    return 0;
}

int schostd_main(int argc, char *argv[])
{
    struct option_args_t args;
    struct gecko_cmd_packet *evt = NULL;


    info("Build Time: Date: %s, Time: %s\n", __DATE__, __TIME__);
    memset(&args, 0, sizeof(args));    

    /* Initialize BGLIB with our output function for sending messages. */
    BGLIB_INITIALIZE_NONBLOCK(on_message_send, uartRx, uartRxPeek);

    /* Initialise serial communication as non-blocking. */
    args.dev.baudrate = 115200;
    if (app_serial_port_init(&args, 100) < 0) {
        error(1, errno, "Non-blocking serial port init failure");
    }

    const char* pidfile = "/var/run/sc_host.pid";
    const char* check_process_cmd = "test -f /proc/`cat /var/run/sc_host.pid`/cmdline";
    if (access(pidfile, R_OK) == 0 && system(check_process_cmd) == 0) {
        error(1, errno, "schostd stop exist sc_host process first");
    }
    /* no other sc_host process */
    echo(0, pidfile, "%u", (unsigned)getpid());

    unlink("/tmp/.schostd");
    struct sock_t* sock = create_socket(AF_LOCAL, SOCK_DGRAM, 0);
    if(!sock){
        error(1, errno, "failed to create control socket");
    }
    if(0 > bind_socket(sock, "/tmp/.schostd", 0)){
        error(1, errno, "failed to bind control socket to /tmp/.schostd");
    }

    int ctl_fd = sock->fd;
    int uart_fd = uartHandle();

    fd_set rfdset, rfdset_save;
    FD_ZERO(&rfdset_save);
    FD_SET(ctl_fd, &rfdset_save);
    FD_SET(uart_fd, &rfdset_save);
    int maxfd = 1 + ((ctl_fd > uart_fd) ? ctl_fd : uart_fd);
   
    /* Reset NCP to ensure it gets into a defined state.
     * Once the chip successfully boots, gecko_evt_system_boot_id event should be
     * received. */
    gecko_cmd_system_reset(0);
    
    info("Starting up..., Resetting NCP target...");

    while (1) {
        rfdset = rfdset_save;

        int ret = select(maxfd, &rfdset, NULL, NULL, NULL);
        if(ret < 0){
            error(1, errno, "failed to select socket");
        }

        if(FD_ISSET(uart_fd, &rfdset)){
            /* Check for stack event. */
            evt = gecko_peek_event();
            if ( evt == NULL){
                continue;
            }
                
        }
        
        if(FD_ISSET(ctl_fd, &rfdset)){
            if(0 < recv_socket(sock, 0, 1, &args, sizeof(args))){
                info("command received");
            }
            const char* msg = "i get it";
            for(int i = 0 ; i< 5 ; i ++){
                if(0 > send_socket(sock, 0, 1, msg, strlen(msg))){
                    error(0, errno, "failed to send data to client");
                }
                sleep(1);
            }
            send_socket(sock, 0, 1, "", 0);
        }
    }

    return -1;
}

int main(int argc, char* argv[])
{
    if(!strcmp(program_invocation_short_name, "sc_host")){
        return schost_main(argc, argv);
    }

    return schostd_main(argc, argv);
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

#if 0
static void system_init_action(void)
{
    /* BT advertising may affect other functions,
     * we should stop it after system boot up and
     * start it when we need it */
    gecko_cmd_le_gap_stop_advertising(0);
    sleep(1);
}

static void load_bt_dev_info(void)
{
    struct gecko_msg_flash_ps_load_rsp_t* rsp = gecko_cmd_flash_ps_load(0x4001);
    echo(0, BLE_VERSION, "Version: %.2X", (uint8_t)rsp->value.data[0]);

    struct gecko_msg_system_get_bt_address_rsp_t* btaddr = gecko_cmd_system_get_bt_address();
    echo(0, BLE_ADDRESS, "BT address: %s", ether_ntoa((struct ether_addr*)btaddr));
}
#endif

