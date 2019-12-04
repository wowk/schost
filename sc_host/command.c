#include "command.h"
#include "app.h"
#include <stdbool.h>

struct cmd_table_t cmd_tab[OPT_ALL] = {
    [OPT_SHOW]    = {
        .name = "SHOW",
        .single_shot = true,  
        .cmd_handler = show_cmd_handler,    
    },

    [OPT_SET]     = {
        .name = "SET",
        .single_shot = true,  
        .cmd_handler = set_cmd_handler,
        .cleanup     = set_cleanup,
    },
  
    [OPT_PAIR]    = {
        .name = "PAIR",
        .single_shot = false,
        .bootup_handler = pair_bootup_handler,
        .cmd_handler = pair_cmd_handler,    
        .event_handler = pair_event_handler,
        .cleanup     = pair_cleanup,
    },
     
    [OPT_CONNECT] = {
        .name = "CONNECT",
        .single_shot = true, 
        .cmd_handler = connect_cmd_handler, 
    },
    
    [OPT_CONNECTION] = {
        .name = "CONNECTION",
        .single_shot = true, 
        .cmd_handler = connection_cmd_handler, 
    },
    
    [OPT_SCAN]    = {
        .name = "SCAN",
        .single_shot = false, 
        .cmd_handler = scan_cmd_handler,    
        .event_handler = scan_event_handler,
        .cleanup     = scan_cleanup,
    },

    [OPT_DTM]     = {
        .name = "DTM",
        .single_shot = false, 
        .bootup_handler = dtm_bootup_handler,
        .cmd_handler = dtm_cmd_handler,     
        .event_handler = dtm_event_handler,
        .cleanup     = dtm_cleanup,
    },
  
    [OPT_UPGRADE] = {
        .name = "UPGRADE",
        .single_shot = false, 
        .cmd_handler = upgrade_cmd_handler, 
        .event_handler = upgrade_event_handler, 
        .cleanup = upgrade_cleanup
    },

    [OPT_GATT]    = {
        .name = "GATT",
        .single_shot = false,
        .bootup_handler = gatt_bootup_handler,
        .cmd_handler = gatt_cmd_handler,
        .event_handler = gatt_event_handler,
        .cleanup = gatt_cleanup,
    },

    [OPT_DISCOVER] = {
        .name = "DISCOVER",
        .single_shot = false,
        .bootup_handler = discover_bootup_handler,
        .cmd_handler    = discover_cmd_handler,
        .event_handler  = discover_event_handler,
        .cleanup = discover_cleanup,
    },
}; 



