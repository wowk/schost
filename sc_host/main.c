#include "debug.h"
#include "sock.h"
#include "timer.h"
#include "options.h"
#include "headers.h"
#include "connection.h"


BGLIB_DEFINE();

static void on_message_send(uint32_t msg_len, uint8_t *msg_data);
static int app_serial_port_init(const struct option_args_t* args, int32_t timeout);


int schost_main(int argc, char* argv[])
{
    char buffer[512];
    struct sock_t* sock;
    struct option_args_t command;

    if(0 > parse_args(argc, argv, &command)){
        error(1, EINVAL, "failed to parse args");
    }
 
    unlink(SCHOST_CLIENT_USOCK);

    sock = create_socket(AF_LOCAL, SOCK_DGRAM, 0);
    if(!sock){
        error(1, errno, "failed to create socket for sc_host");
    }

    if(0 > bind_socket(sock, SCHOST_CLIENT_USOCK, 0)){
        error(1, errno, "failed bind to sc_host");
    } 

    socket_addr(&sock->daddr, AF_LOCAL, SCHOST_SERVER_USOCK, 0, &sock->socklen);

    if(0 > send_socket(sock, 0, 1, &command, sizeof(command))){
        error(1, errno, "failed to send command to schostd");
    }

    while(1){
        memset(buffer, 0, sizeof(buffer));
        int retlen = recv_socket(sock, 0, 1, buffer, sizeof(buffer));
        if(retlen < 0){
            error(1, errno, "failed to receive response from schostd");
        }else if(retlen == 0){
            break;
        }

        if(buffer[0] == '\r'){
            printf("%s", buffer);
        }else{
            printf("%s\n", buffer);
        }
    }

    close_socket(sock);
    
    return 0;
}

int schostd_main(int argc, char *argv[])
{
    int ctl_fd;
    int uart_fd;
    int maxfd;
    fd_set rfdset;
    fd_set rfdset_save;

    enum option_e cmd;
    enum option_e new_cmd;

    struct gecko_cmd_packet *evt = NULL;
    struct option_args_t conf;
    struct option_args_t command;
    
    info("Build Time: Date: %s, Time: %s\n", __DATE__, __TIME__);
    daemon(0,1);
    memset(&conf, 0, sizeof(conf));    

    /* Initialize BGLIB with our output function for sending messages. */
    BGLIB_INITIALIZE_NONBLOCK(on_message_send, uartRx, uartRxPeek);

    /* Initialise serial communication as non-blocking. */
    conf.dev.baudrate   = SCHOST_UART_BAUDRATE;
    conf.dev.txpwr      = SCHOST_UART_DEF_PWR;
    strcpy(conf.dev.name, SCHOST_UART_DEV);
    if (app_serial_port_init(&conf, 100) < 0) {
        error(0, errno, "Non-blocking serial port init failure");
    }

    if (process_running(SCHOST_PID_FILE)) {
        error(1, errno, "schostd stop exist sc_host process first");
    }
    echo(0, SCHOST_PID_FILE, "%d", (int)getpid());

    unlink(SCHOST_SERVER_USOCK);
    struct sock_t* sock = create_socket(AF_LOCAL, SOCK_DGRAM, 0);
    if(!sock){
        error(1, errno, "failed to create control socket");
    }
    if(0 > bind_socket(sock, SCHOST_SERVER_USOCK, 0)){
        error(1, errno, "failed to bind control socket to /tmp/.schostd");
    }

    ctl_fd = sock->fd;
    uart_fd = uartHandle();
    if(ctl_fd > uart_fd)
        maxfd = ctl_fd;
    else
        maxfd = uart_fd;
    
    FD_ZERO(&rfdset_save);
    FD_SET(ctl_fd, &rfdset_save);
	maxfd = ctl_fd;
    FD_SET(uart_fd, &rfdset_save);
   
    /* ************************************************************
     * Reset NCP to ensure it gets into a defined state.
     * Once the chip successfully boots, gecko_evt_system_boot_id 
     * event should be received. 
     * ************************************************************/
    gecko_cmd_system_reset(0);
     
    debug(conf.debug, "Starting up...");
    debug(conf.debug, "Resetting NCP target...");

    cmd = OPT_IDLE;

    while (1) {
        rfdset = rfdset_save;

        int ret = select(maxfd+1, &rfdset, NULL, NULL, NULL);
        if(ret < 0){
            error(1, errno, "failed to select socket");
        }

        if(FD_ISSET(uart_fd, &rfdset)){
            /* Check for stack event. */
            evt = gecko_peek_event();
            if(!evt){
                continue;
            }

            switch (BGLIB_MSG_ID(evt->header)) {
            case gecko_evt_dfu_boot_id:
            case gecko_evt_system_boot_id:
                hw_timer_list_clear();
                //notify_list_clear();
                //read_request_list_clear();
                //write_request_list_clear();
                //discover_request_list_clear();

                info("Bootup done");
                for(int i = 0 ; i < OPT_ALL ; i ++){
                    if(!cmd_tab[i].bootup_handler){
                        continue;
                    }
                    cmd_tab[i].bootup_handler(&conf);
                }
                break;
            }
            
            for(int i = 0 ; i < OPT_ALL ; i ++){
                if(!cmd_tab[i].event_handler){
                    continue;
                }else if(cmd != OPT_IDLE && cmd == i){
                    ret = cmd_tab[i].event_handler(sock, &conf, evt);
                    if(ret == BLE_EVENT_RETURN || ret == BLE_EVENT_STOP){
                        send_socket(sock, 0, 1, "", 0);
                        cmd = OPT_IDLE;
                    }
                }else{
                    cmd_tab[i].event_handler(sock, &conf, evt);
                }
            }
        }
        
        if(FD_ISSET(ctl_fd, &rfdset)){
            int retlen = recv_socket(sock, 0, 1, &command, sizeof(command));
            if(retlen != sizeof(command)){
                continue;
            }
            
            new_cmd = command.option;
            info("Do %s Cmd Handling", cmd_tab[new_cmd].name);
            if(cmd_tab[new_cmd].single_shot){
                cmd_tab[new_cmd].cmd_handler(sock, &command);
                if(cmd_tab[new_cmd].cleanup){
                    cmd_tab[new_cmd].cleanup(sock, &command);
                    info("Do %s Cleanup", cmd_tab[new_cmd].name);
                }
                send_socket(sock, 0, 1, "", 0);
                cmd = OPT_IDLE;
                continue;
            }

            switch(new_cmd){
            case OPT_SCAN:
                conf.scan = command.scan;
                break;
            case OPT_PAIR:
                conf.pair = command.pair;
                break;
            case OPT_UPGRADE:
                conf.upgrade = command.upgrade;
                break;
            case OPT_DTM:
                conf.dtm = command.dtm;
                break;
            case OPT_DEV:
                conf.dev = command.dev;
                break;
            case OPT_SHOW:
                conf.show = command.show;
                break;
            case OPT_SET:
                conf.set  = command.set;
            default:
                break;
            }
            
            if(cmd_tab[cmd].cleanup){
                cmd_tab[cmd].cleanup(sock, &conf);
            }
            ret = cmd_tab[new_cmd].cmd_handler(sock, &conf);
            if(ret == BLE_EVENT_STOP){
                if(cmd_tab[new_cmd].cleanup){
                    cmd_tab[new_cmd].cleanup(sock, &conf);
                    info("Do %s Cleanup", cmd_tab[new_cmd].name);
                }
                info("Back to %s command", cmd_tab[OPT_PAIR].name);
                cmd_tab[OPT_PAIR].cmd_handler(sock, &conf);
                cmd = OPT_IDLE;
            }else if(ret == BLE_EVENT_RETURN){
                send_socket(sock, 0, 1, "", 0);
                cmd = OPT_IDLE;
            }else{
                cmd = new_cmd;
            }
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

static void on_message_send(uint32_t msg_len, uint8_t *msg_data)
{
    int32_t ret;

    ret = uartTx(msg_len, msg_data);
    if (ret < 0) {
        printf("Failed to write to serial port, ret: %d, errno: %s\n", ret, strerror(errno));
        exit(EXIT_FAILURE);
    }
}

static int app_serial_port_init(const struct option_args_t* args, int32_t timeout)
{
    return uartOpen((int8_t*)args->dev.name, args->dev.baudrate, args->dev.flowctrl, args->dev.timeout);
}

