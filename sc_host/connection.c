#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <stdbool.h>
#include <sys/queue.h>
#include <netinet/ether.h>
#include "host_gecko.h"
#include "util.h"
#include "app.h"
#include "args.h"
#include "sock.h"
#include "connection.h"
#include "discover.h"



struct connection_elem_t {
    struct connection_t conn;
    LIST_ENTRY(connection_elem_t) entry;
};
LIST_HEAD(connection_list_t, connection_elem_t);

struct connection_list_t connection_list;


int connection_opened(struct gecko_msg_le_connection_opened_evt_t* evt)
{
    struct connection_t* conn;
    struct connection_elem_t* conn_elem;

    conn = connection_find_unused();
    if(!conn){
        conn_elem = (struct connection_elem_t*)calloc(1, sizeof(struct connection_elem_t));
        if(!conn_elem){
            return -ENOMEM;
        }
        conn = &conn_elem->conn;
        LIST_INSERT_HEAD(&connection_list, conn_elem, entry);
    }
    conn->connection = evt->connection;
    conn->master     = evt->master;
    conn->bonding    = evt->bonding;
    conn->addrtype   = evt->address_type;
    conn->advertiser = evt->advertiser;
    conn->address    = evt->address;
    conn->used = true;
    info("connection opened: %u", conn->connection);
   
    discover_services(conn->connection);

    return 0;
}

int connection_closed(uint8_t connid, uint16_t reason)
{
    struct connection_t* conn;

    conn = connection_find_by_conn(connid);
    if(!conn){
        return -1;
    }
    conn->used = false;
    conn->reason = reason;
    info("connection closed");

    return 0;
}

struct find_args_t {
    void* arg;
    struct connection_t* conn;
};

void connection_visit(int(*visit_cb)(struct connection_t*, void*), void* args)
{
    struct connection_elem_t* elem = NULL;

    LIST_FOREACH(elem, &connection_list, entry){
        if(visit_cb(&elem->conn, args)){
            break;
        }
    }
}

static int find_unused(struct connection_t* conn, void* args)
{
    struct find_args_t* fa = (struct find_args_t*)args;

    if(!conn->used){
        fa->conn = conn;
        return 1;
    }

    return 0;
}

static int find_by_bdaddr(struct connection_t* conn, void* args)
{
    struct find_args_t* fa = (struct find_args_t*)args;
    
    if(conn->used && !memcmp(fa->arg, &conn->address, sizeof(conn->address))){
        fa->conn = conn;
        return 1;
    }

    return 0;
}

static int find_by_connid(struct connection_t* conn, void* args)
{
    struct find_args_t* fa = (struct find_args_t*)args;

    if(conn->used && ((uint8_t)(uint32_t)fa->arg) == conn->connection){
        fa->conn = conn;
        return 1;
    }

    return 0;
}

static int clear_conn(struct connection_t* conn, void* args)
{
    conn->used = 0;
    conn->reason = 0;
    return 0;
}

void connection_clear()
{
    connection_visit(clear_conn, NULL);
}

struct connection_t* connection_find_unused()
{
    struct find_args_t fa = {
        .conn   = NULL,
    };

    connection_visit(find_unused, &fa);

    return fa.conn;
}

struct connection_t* connection_find_by_addr(bd_addr* addr)
{
    struct find_args_t fa = {
        .arg    = addr,
        .conn   = NULL,
    };

    connection_visit(find_by_bdaddr, &fa);

    return fa.conn;
}

struct connection_t* connection_find_by_conn(int conn)
{
    struct find_args_t fa = {
        .arg    = (void*)conn,
        .conn   = NULL,
    };

    connection_visit(find_by_connid, &fa);

    return fa.conn;
}
