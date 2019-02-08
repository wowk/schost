#ifndef SC_HOST_EVENT_HANDLER_H__
#define SC_HOST_EVENT_HANDLER_H__

#include <args.h>
#include <bg_types.h>
#include <gecko_bglib.h>
#include <host_gecko.h>


typedef void (*event_handler_t)(int, struct gecko_cmd_packet*, struct option_args_t*);

extern int show_event_handler(int msgid, struct gecko_cmd_packet *evt, struct option_args_t* args);
extern int set_event_handler(int msgid, struct gecko_cmd_packet *evt, struct option_args_t* args);
extern int scan_event_handler(int msgid, struct gecko_cmd_packet *evt, struct option_args_t* args);
extern int connect_event_handler(int msgid, struct gecko_cmd_packet *evt, struct option_args_t* args);
extern int dtm_event_handler(int msgid, struct gecko_cmd_packet *evt, struct option_args_t* args);
extern int pair_event_handler(int msgid, struct gecko_cmd_packet *evt, struct option_args_t* args);
extern int upgrade_event_handler(int msgid, struct gecko_cmd_packet *evt, struct option_args_t* args);

#endif //SC_HOST_EVENT_HANDLER_H__
