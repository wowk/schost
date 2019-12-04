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


struct connection_visit_args_t {
    struct sock_t* sock;
    struct option_args_t* args;
    struct connection_t* conn;
};

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
    service_clean(&conn->service_list);
    characteristic_clean(&conn->characteristic_list);

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

static char* property2str(uint8_t property, char* buffer)
{
    const struct {
        uint8_t property_bit;
        const char* property_str;
    } property_db[] = {
        {gatt_char_prop_read,       "r "},
        {gatt_char_prop_write,      "w "},
        {gatt_char_prop_writenoresp,"wn "},
        {gatt_char_prop_writesign,  "ws "},
        {gatt_char_prop_broadcast,  "bc "},
        {gatt_char_prop_extended,   "ex "},
        {gatt_char_prop_indicate,   "ind "},
        {gatt_char_prop_notify,     "nt "},
    };

    for(int i = 0 ; i < sizeof(property_db)/sizeof(property_db[0]) ; i ++){
        if(property_db[i].property_bit & property){
            strcpy(buffer, property_db[i].property_str);
        }
    }

    return buffer;
}

static int connection_characteristic_dump(struct connection_t* conn, void* args)
{
    char address[18] = "";
    char prop_buf[32] = "";
    struct characteristic_t* character;
    struct connection_visit_args_t* cva;

    cva = (struct connection_visit_args_t*)args;
    
    if(!conn->used){
        return 0;
    }else if(cva->conn){
        if(cva->conn != conn){
            return 0;
        }
    }
    
    btaddr2str(&conn->address, address);
    printf_socket(cva->sock, "Connection: %d", conn->connection);
    printf_socket(cva->sock, "       ServiceUUID CharacteristicUUID  Properity", 
            conn->connection, address, conn->addrtype);
    LIST_FOREACH(character, &conn->characteristic_list, entry){
        printf_socket(cva->sock, "       %.4X        %.4X                %s", 
                htons(character->service->uuid), htons(character->uuid), 
                property2str(character->properties, prop_buf));
    }

    return 0;
}

int connection_cmd_handler(struct sock_t* sock, struct option_args_t* args)
{
    struct connection_t* conn;

    if(args->connection.list){ 
        printf_socket(sock, "%-4s%-25s%s", "ID", "Address", "AddrType");
        connection_visit(connection_dump, sock);
    }
    
    if(args->connection.characteristic){
        struct connection_visit_args_t cva = {
            .conn = NULL,
            .sock = sock,
            .args = args,
        };
        if(args->connection.characteristic != 0x100){
            cva.conn = connection_find_by_conn((uint8_t)args->connection.characteristic);
            if(!cva.conn){
                printf_socket(sock, "Connection Not Found");
            }else{
                connection_visit(connection_characteristic_dump, &cva);
            }
        }else{
            connection_visit(connection_characteristic_dump, &cva);
        }
    }
    
    if(args->connection.disconn){
        conn = connection_find_by_conn(args->connect.disconn);
        if(conn){
            gecko_cmd_le_connection_close(conn->connection);
        }else{
            printf_socket(sock, "Connection Not Found");
        }
    }

    return BLE_EVENT_RETURN;
}
