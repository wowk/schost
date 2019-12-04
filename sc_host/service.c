#include <util.h>
#include <connection.h>
#include <service.h>
#include <host_gecko.h>
#include <stdlib.h>

int service_add(struct service_list_t* list, struct gecko_msg_gatt_service_evt_t* evt)
{
    uint16_t uuid;
    struct service_t* service;

    uuid = to_uuid16(&evt->uuid);
    service = service_find_by_uuid(list, uuid);
    if(!service){
        service = (struct service_t*)calloc(1, sizeof(struct service_t));
        if(!service){
            return -1;
        }
        service->handle = evt->service;
        service->uuid   = uuid;
        service->head   = list;
        LIST_INSERT_HEAD(list, service, entry);
    }

    return 0;
}

int characteristic_add(struct service_t* service, struct characteristic_list_t* list, 
        struct gecko_msg_gatt_characteristic_evt_t* evt)
{
    uint16_t uuid;
    struct characteristic_t* character;

    uuid = to_uuid16(&evt->uuid);
    character = characteristic_find_by_uuid(list, uuid);
    if(!character){
        character = (struct characteristic_t*)calloc(1, sizeof(struct characteristic_t));
        if(!character){
            return -1;
        }
        character->handle = evt->characteristic;
        character->properties = evt->properties;
        character->uuid   = uuid;
        character->head   = list;
        character->service= service;
        LIST_INSERT_HEAD(list, character, entry);
    }

    return 0;
   
}

int descriptor_set(struct characteristic_t* character, struct gecko_msg_gatt_descriptor_evt_t* evt)
{
    return 0;
}

struct service_t* service_find_by_handle(struct service_list_t* list, uint32_t handle)
{
    struct service_t* service;

    LIST_FOREACH(service, list, entry) {
        if(handle == service->handle){
            return service;
        }
    }

    return NULL;
}


struct service_t* service_find_by_uuid(struct service_list_t* list, uint16_t uuid)
{
    struct service_t* service;

    LIST_FOREACH(service, list, entry) {
        if(uuid == service->uuid){
            return service;
        }
    }

    return NULL;
}

struct characteristic_t* characteristic_find_by_uuid(struct characteristic_list_t* list, uint16_t uuid)
{
    struct characteristic_t* character;

    LIST_FOREACH(character, list, entry) {
        if(uuid == character->uuid){
            return character;
        }
    }

    return NULL;
}

int service_clean(struct service_list_t* list)
{
    struct service_t* service;

    while(!LIST_EMPTY(list)){
        service = LIST_FIRST(list);
        LIST_REMOVE(service, entry);
        if(!service){
            break;
        }
        Free(service);
    }

    return 0;
}

int characteristic_clean(struct characteristic_list_t* list)
{
    struct characteristic_t* character;

    while(!LIST_EMPTY(list)){
        character = LIST_FIRST(list);
        LIST_REMOVE(character, entry);
        if(character->desc.info){
            Free(character->desc.info);
        }
        Free(character);
    }

    return 0;
}


