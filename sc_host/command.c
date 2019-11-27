#include "command.h"
#include "app.h"
#include <stdbool.h>

struct cmd_table_t cmd_tab[OPT_ALL] = {
    [OPT_SHOW]    = {
        .single_shot = true,  
        .cmd_handler = show_cmd_handler,    
    },

    [OPT_SET]     = {
        .single_shot = true,  
        .cmd_handler = set_cmd_handler,
        .cleanup     = set_cleanup,
    },
  
    [OPT_PAIR]    = {
        .single_shot = false, 
        .cmd_handler = pair_cmd_handler,    
        .event_handler = pair_event_handler,
        .cleanup     = pair_cleanup,
    },
     
    [OPT_CONNECT] = {
        .single_shot = true, 
        .cmd_handler = connect_cmd_handler, 
    },
    
    [OPT_SCAN]    = {
        .single_shot = false, 
        .cmd_handler = scan_cmd_handler,    
        .event_handler = scan_event_handler,
        .cleanup     = scan_cleanup,
    },

    [OPT_DTM]     = {
        .single_shot = false, 
        .cmd_handler = dtm_cmd_handler,     
        .event_handler = dtm_event_handler,
        .cleanup     = dtm_cleanup,
    },
  
    [OPT_UPGRADE] = {
        .single_shot = false, 
        .cmd_handler = upgrade_cmd_handler, 
        .event_handler = upgrade_event_handler, 
        .cleanup = upgrade_cleanup
    },
}; 



