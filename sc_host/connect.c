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

static int connection_dump(struct connection_t* conn, void* args)
{
    char address[18] = "";
    struct sock_t* sock = (struct sock_t*)args;

    info("dump: %u", conn->connection);
    if(conn->used){
        info("Found");
        btaddr2str(&conn->address, address);
        printf_socket(sock, "%-4d%-25s%d", conn->connection, address, conn->addrtype);
    }

    return 0;
}

int connect_cmd_handler(struct sock_t* sock, struct option_args_t* args)
{
    struct connection_t* conn;

    info("connect command");
    if(args->connect.show) {
        printf_socket(sock, "%-4s%-25s%s", "ID", "Address", "AddrType");
        connection_visit(connection_dump, sock);        
        return BLE_EVENT_RETURN;
    }else if(args->connect.disconn){
        conn = connection_find_by_conn(args->connect.disconn);
        if(conn){
            gecko_cmd_le_connection_close(conn->connection);
            return BLE_EVENT_CONTINUE;
        }
    }else if(args->connect.address[0]){
        return connect_address(sock, args);
    }

    return BLE_EVENT_RETURN;
}

