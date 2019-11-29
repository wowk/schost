int schostd_main(int argc, char *argv[])
{
    int ctl_fd;
    int uart_fd;
    int maxfd;
    fd_set rfdset;
    fd_set rfdset_save;

    enum cmd_type_e cmd;
    enum cmd_type_e new_cmd;
    enum cmd_type_e default_cmd = CMD_PAIR;

    struct cmd_table_t* cmd_ops = NULL;
    struct option_args_t conf;
    struct gecko_cmd_packet *evt = NULL;
    
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
        error(1, errno, "Non-blocking serial port init failure");
    }

    if (process_running(SCHOST_PID_FILE)) {
        error(1, errno, "schostd stop exist sc_host process first");
    }

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
    FD_SET(uart_fd, &rfdset_save);
   
    /* ************************************************************
     * Reset NCP to ensure it gets into a defined state.
     * Once the chip successfully boots, gecko_evt_system_boot_id 
     * event should be received. 
     * ************************************************************/
    ble_system_reset(0);
    
    debug(conf.debug, "Starting up...\n");
    debug(conf.debug, "Resetting NCP target...");

    while (1) {
        rfdset = rfdset_save;

        int ret = select(maxfd+1, &rfdset, NULL, NULL, NULL);
        if(ret < 0){
            error(1, errno, "failed to select socket");
        }

        if(FD_ISSET(uart_fd, &rfdset)){
            /* Check for stack event. */
            evt = gecko_peek_event();
            if (evt && cmd_ops){
                int ret = cmd_ops->event_handler(sock, &conf, evt);
                switch(ret){
                case BLE_EVENT_RETURN:
                    send_socket(sock, 0, 1, "", 0);
                    break;

                case BLE_EVENT_STOP:
                    send_socket(sock, 0, 1, "", 0);
                    if(cmd_ops->cleanup){
                        cmd_ops->cleanup(sock, &conf);
                    }
                    cmd_ops = &cmd_tab[default_cmd];
                    cmd = default_cmd;
                    break;
                case BLE_EVENT_CONTINUE:
                    break;
                default:
                    break;
                }
            }
        }
        
        if(FD_ISSET(ctl_fd, &rfdset)){
            int retlen = recv_socket(sock, 0, 1, &conf, sizeof(conf));
            if(retlen != sizeof(struct option_args_t)){
                continue;
            }
            debug(conf.debug, "get command from client");

            if(conf.show.on){
                new_cmd = CMD_SHOW;                    
            }else if(conf.set.on){
                new_cmd = CMD_SET;
            }else if(conf.pair.on){
                new_cmd = CMD_PAIR; 
            }else if(conf.connect.on){
                new_cmd = CMD_CONNECT;
            }else if(conf.upgrade.on){
                new_cmd = CMD_UPGRADE;
            }else if(conf.dtm.on){
                new_cmd = CMD_DTM;
            }else if(conf.scan.on){
                new_cmd = CMD_SCAN;
            }else{
                send_socket(sock, 0, 1, "", 0);
                continue;
            }
             
            if(cmd_tab[new_cmd].single_shot){
                cmd_tab[new_cmd].cmd_handler(sock, &conf);
                if(cmd_tab[new_cmd].cleanup)
                    cmd_tab[new_cmd].cleanup(sock, &conf);
                send_socket(sock, 0, 1, "", 0);
            }else{
                if(new_cmd != cmd){
                    if(cmd_ops && cmd_ops->cleanup){
                        cmd_ops->cleanup(sock, &conf);
                    }
                    cmd = new_cmd;
                    cmd_ops = &cmd_tab[cmd];
                }
                ret = cmd_ops->cmd_handler(sock, &conf);
                if(ret == BLE_EVENT_STOP){
                    if(cmd_ops->cleanup){
                        cmd_ops->cleanup(sock, &conf);
                    }
                    cmd_ops = NULL;
                    cmd = CMD_NONE;
                }else if(ret == BLE_EVENT_RETURN){
                    send_socket(sock, 0, 1, "", 0);
                }
            }
        }
    }

    return -1;
}


