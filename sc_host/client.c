

int schost_main(int argc, char* argv[])
{
    char buffer[512] = "";
    struct option_args_t args;
    
    if(0 > parse_args(argc, argv, &args)){
        error(1, EINVAL, "failed to parse args");
    }

    unlink(SCHOST_CLIENT_USOCK);
    
    struct sock_t* sock = create_socket(AF_LOCAL, SOCK_DGRAM, 0);
    if(!sock){
        error(1, errno, "failed to create socket for sc_host");
    }

    if(0 > bind_socket(sock, SCHOST_CLIENT_USOCK, 0)){
        error(1, errno, "failed bind to sc_host");
    } 

    socket_addr(&sock->daddr, AF_LOCAL, SCHOST_CLIENT_USOCK, 0, &sock->socklen);

    if(0 > send_socket(sock, 0, 1, &args, sizeof(args))){
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


