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
#include "connection.h"


static int connect_address(struct sock_t* sock, struct option_args_t* args)
{
    bd_addr addr;
    struct connection_t* conn;

    str2btaddr((char*)args->connect.address, &addr);
    conn = connection_find_by_addr(&addr);
    if(conn){
        printf_socket(sock, "Already Connected");
        return BLE_EVENT_RETURN;
    }

    gecko_cmd_le_gap_connect(addr, args->connect.addrtype, args->connect.initphy);
    printf_socket(sock, "Connecting %s", (char*)args->connect.address);

    return BLE_EVENT_RETURN;
}

int connect_cmd_handler(struct sock_t* sock, struct option_args_t* args)
{
    if(args->connect.show || args->connect.disconn) {
        args->connection.list = args->connect.show;
        args->connection.disconn = args->connect.disconn;
        return connection_cmd_handler(sock, args);
    }else if(args->connect.address[0]){
        return connect_address(sock, args);
    }

    return BLE_EVENT_RETURN;
}

