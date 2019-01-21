#ifndef SC_HOST_EVENT_HANDLER_H__
#define SC_HOST_EVENT_HANDLER_H__

struct gecko_cmd_packet;
struct option_args_t;

typedef void (*event_handler_t)(struct gecko_cmd_packet*, const struct option_args_t*);

#endif //SC_HOST_EVENT_HANDLER_H__
