#ifndef SCHOST_COMMAND_H_
#define SCHOST_COMMAND_H_

#include <stdbool.h>
#include <args.h>
#include <sock.h>
#include <host_gecko.h>

struct sock_t;
struct gecko_cmd_packet;

typedef int(*cmd_handler_cb)(struct sock_t*, struct option_args_t*);
typedef int(*cmd_exit_cb)(struct sock_t*, struct option_args_t*);
typedef int(*event_handler_cb)(struct sock_t* sock, struct option_args_t*, struct gecko_cmd_packet*);

struct cmd_table_t {
    cmd_handler_cb cmd_handler;
    event_handler_cb event_handler;
    event_handler_cb global_event_handler;
    cmd_exit_cb cleanup;
    bool single_shot;
};

struct ctrl_command_t {
    int type;
    struct option_args_t conf;
};

extern struct cmd_table_t cmd_tab[];

#endif
