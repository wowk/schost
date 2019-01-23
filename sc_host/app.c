#include "app.h"
#include "util.h"
#include <event_handler.h>
#include <bg_types.h>
#include <gecko_bglib.h>
#include <host_gecko.h>

void app_handle_events(struct gecko_cmd_packet *evt, struct option_args_t* args)
{
    int msgid = BGLIB_MSG_ID(evt->header);
    if(args->show.on){
        show_event_handler(msgid, evt, args);
        args->show.on = 0;
    }else if(args->set.on){
        set_event_handler(msgid, evt, args);
        args->set.on = 0;
    }else if(args->pair.on){
        pair_event_handler(msgid, evt, args);
    }else if(args->scan.on){
        scan_event_handler(msgid, evt, args);
    }else if(args->connect.on){
        connect_event_handler(msgid, evt, args);
    }else if(args->upgrade.on){
        upgrade_event_handler(msgid, evt, args);
    }else{
        exit(0);
    }
}
