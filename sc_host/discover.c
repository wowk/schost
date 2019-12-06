#include <app.h>
#include <sock.h>
#include <util.h>
#include <timer.h>
#include <sys/queue.h>
#include <service.h>
#include <discover.h>
#include <connection.h>


#define DISCOVER_TIMER_ID 0x50
#define DISCOVER_INTERVAL 1.0f

struct discover_request_elem_t {
    struct discover_request_t req;
    STAILQ_ENTRY(discover_request_elem_t) entry;
};

struct discover_request_queue_t {
	struct discover_request_elem_t *stqh_first;
	struct discover_request_elem_t **stqh_last;
};

struct discover_timer_arg_t {
    struct sock_t* sock;
    struct option_args_t* arg;
};

static int discover_timer_handler(struct hw_timer_t*);

struct discover_request_queue_t discover_request_queue 
            = STAILQ_HEAD_INITIALIZER(discover_request_queue);

struct discover_timer_arg_t discover_timer_arg; 

struct hw_timer_t discover_timer = {
    .id         = DISCOVER_TIMER_ID,
    .interval   = DISCOVER_INTERVAL,
    .count      = HW_TIMER_INFINITY,
    .arg        = &discover_timer_arg,
    .ret        = BLE_EVENT_IGNORE,
    .callback   = discover_timer_handler,
};

struct discover_request_t* discover_request_get()
{
    struct discover_request_elem_t* elem;
   
    elem = STAILQ_FIRST(&discover_request_queue);
    if(elem){
        return &elem->req;
    }

    return NULL;
}

void discover_request_free_head()
{
    struct discover_request_elem_t* elem;
    
    if(STAILQ_EMPTY(&discover_request_queue)){
        return;
    }

    elem = STAILQ_FIRST(&discover_request_queue);
    STAILQ_REMOVE_HEAD(&discover_request_queue, entry);
    
    if(elem){
        Free(elem);
    }
}

void discover_clear_by_connection(uint8_t connection)
{
    struct discover_request_elem_t* elem;
    struct discover_request_elem_t* deleted = NULL;

    STAILQ_FOREACH(elem, &discover_request_queue, entry) {
        if(deleted){
            info("clear request");
            STAILQ_REMOVE(&discover_request_queue, deleted, discover_request_elem_t, entry);
            free(deleted);
            deleted = NULL;
        }
        if(elem->req.connection == connection){
            deleted = elem;
        }
    }
    
    if(deleted){
        info("clear request");
        STAILQ_REMOVE(&discover_request_queue, deleted, discover_request_elem_t, entry);
        free(deleted);
        deleted = NULL;
    }
}

void discover_request_queue_clear()
{
    struct discover_request_elem_t* elem;
    
    while(1){
        elem = STAILQ_FIRST(&discover_request_queue);
        if(!elem){
            break;
        }
        STAILQ_REMOVE(&discover_request_queue, elem, discover_request_elem_t, entry);
        Free(elem);
    }
}

int discover_services(uint8_t connection)
{
    struct discover_request_t* req;
    struct discover_request_elem_t* elem;

    elem = (struct discover_request_elem_t*)Calloc(1, sizeof(*elem));
    if(!elem){
        return -1;
    }
    
    info("Add Service\n");
    STAILQ_INSERT_TAIL(&discover_request_queue, elem, entry);
    req = &elem->req;
    req->type = DISCOVER_SERVICES;
    req->connection = connection;

    return 0;
}

int discover_characteristics(uint8_t connection, uint32_t service)
{
    struct discover_request_t* req;
    struct discover_request_elem_t* elem;

    elem = (struct discover_request_elem_t*)Calloc(1, sizeof(*elem));
    if(!elem){
        return -1;
    }
    STAILQ_INSERT_TAIL(&discover_request_queue, elem, entry);
    req = &elem->req;
    req->type = DISCOVER_CHARACTERISTIC;
    req->service    = service;
    req->connection = connection;

    return 0;
}

int discover_descriptors(uint8_t connection, uint16_t characteristic)
{
    struct discover_request_t* req;
    struct discover_request_elem_t* elem;

    elem = (struct discover_request_elem_t*)Calloc(1, sizeof(*elem));
    if(!elem){
        return -1;
    }
    memset(elem, 0, sizeof(*elem));
    STAILQ_INSERT_TAIL(&discover_request_queue, elem, entry);
    req = &elem->req;
    req->type = DISCOVER_DESCRIPTORS;
    req->characteristic = characteristic;
    req->connection = connection;
    
    return 0;
}

static int discover_timer_handler(struct hw_timer_t* t)
{
    int result = 0;
    struct discover_request_t* req;
    
    req = discover_request_get();
    if(!req){
        return BLE_EVENT_CONTINUE;
    }

    switch(req->type){
    case DISCOVER_SERVICES:
        result = gecko_cmd_gatt_discover_primary_services(req->connection)->result;
        //info("Find Service\n");
        if(result){
            info("error: %s(%d)\n", error_summary(result), result);
        }
        break;
    case DISCOVER_DESCRIPTORS:
        //info("Find Service 2\n");
        //we dont want to support this because of no demands
        result = gecko_cmd_gatt_discover_descriptors(req->connection, req->characteristic)->result;
        if(result){
            info("error: %s(%d)\n", error_summary(result), result);
        }
        break;
    case DISCOVER_CHARACTERISTIC:
        //info("Find Service 3\n");
        result = gecko_cmd_gatt_discover_characteristics(req->connection, req->service)->result;
        if(result){
            info("error: %s(%d)\n", error_summary(result), result);
        }
        break;
    default:
        //info("Find Service 4\n");
        break;
    }

    return BLE_EVENT_CONTINUE;
}

int discover_bootup_handler(struct sock_t* sock, struct option_args_t* args)
{
    info("Add Timer\n");
    discover_timer_arg.sock = NULL;
    discover_timer_arg.arg  = args;
    hw_timer_add(&discover_timer);    
    
    return 0;
}

int discover_cmd_handler(struct sock_t* sock, struct option_args_t* args)
{
    return BLE_EVENT_RETURN;
}

int discover_event_handler(struct sock_t* sock, struct option_args_t* args, struct gecko_cmd_packet* evt)
{
    uint16_t vvv = 0x0001;
    struct connection_t* conn;
    struct service_t* service;
    struct characteristic_t* character;
    struct gecko_msg_gatt_service_evt_t* service_evt;
    struct gecko_msg_gatt_characteristic_evt_t* character_evt;
    struct gecko_msg_gatt_descriptor_evt_t* descriptor_evt;

    switch(BGLIB_MSG_ID(evt->header)){
    case gecko_evt_gatt_procedure_completed_id:
        discover_request_free_head();
        break;

    case gecko_evt_gatt_descriptor_id:
        descriptor_evt = &evt->data.evt_gatt_descriptor;
        info("discover descriptor: %.4X", ntohs(to_uuid16(&descriptor_evt->uuid)));
        conn = connection_find_by_conn(descriptor_evt->connection);
        if(!conn){
            break;
        }
        character = characteristic_find_by_handle(&conn->characteristic_list, 
                discover_request_get()->characteristic);
        if(!character){
            break;
        }
        info("Service: %.4X -> Characteristic: %.4X ->  Descriptor: %.4X", 
                ntohs(character->service->uuid),
                ntohs(character->uuid),
                ntohs(to_uuid16(&descriptor_evt->uuid)));
        descriptor_add(character, &conn->descriptor_list, descriptor_evt);
        //gecko_cmd_gatt_read_descriptor_value(conn->connection, descriptor_evt->descriptor);
        break;

    case gecko_evt_gatt_service_id:
        service_evt = &evt->data.evt_gatt_service;
        conn = connection_find_by_conn(service_evt->connection);
        if(!conn){
            break;
        }
        service_add(&conn->service_list, service_evt);
        discover_characteristics(service_evt->connection, service_evt->service);
        info("discover service: %.4X", ntohs(to_uuid16(&service_evt->uuid)));
        break;

    case gecko_evt_gatt_characteristic_id:
        character_evt = &evt->data.evt_gatt_characteristic;
        conn = connection_find_by_conn(character_evt->connection);
        if(!conn){
            break;
        }
        service = service_find_by_handle(&conn->service_list, discover_request_get()->service);
        if(!service){
            break;
        }
        characteristic_add(service, &conn->characteristic_list, character_evt);
        discover_descriptors(character_evt->connection, character_evt->characteristic);
        info("discover characteristic: %.4X", ntohs(to_uuid16(&character_evt->uuid)));
        break;

    default:
        break;
    }

    return (int)discover_timer.ret;
}

int discover_cleanup(struct sock_t* sock, struct option_args_t* args)
{
    info("Del Timer\n");
    discover_request_queue_clear();
    hw_timer_del(&discover_timer);

    return BLE_EVENT_RETURN;
}
