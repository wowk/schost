#include "app.h"
#include "util.h"
#include <event_handler.h>
#include <bg_types.h>
#include <gecko_bglib.h>
#include <host_gecko.h>

void app_handle_events(struct gecko_cmd_packet *evt, struct option_args_t* args)
{
    int ret = 0;
    int msgid = BGLIB_MSG_ID(evt->header);
    
    if(args->show.on){
        ret = show_event_handler(msgid, evt, args);
    }else if(args->set.on){
        ret = set_event_handler(msgid, evt, args);
    }else if(args->dtm.on && (args->dtm.tx.on || args->dtm.rx.on)){
        ret = dtm_event_handler(msgid, evt, args);
    }else if(args->pair.on){
        ret = pair_event_handler(msgid, evt, args);
    }else if(args->scan.on){
        ret = scan_event_handler(msgid, evt, args);
    }else if(args->connect.on){
        ret = connect_event_handler(msgid, evt, args);
    }else if(args->upgrade.on){
        ret = upgrade_event_handler(msgid, evt, args);
    }

    if(ret){
        exit(0);
    }
}
