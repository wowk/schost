#ifndef SC_HOST_OPTIONS_H_
#define SC_HOST_OPTIONS_H_

#include <args.h>

#define SCHOST_PID_FILE         "/var/run/schost.pid"
#define SCHOST_CLIENT_USOCK     "/tmp/.schost.usock"
#define SCHOST_SERVER_USOCK     "/tmp/.schostd.usock"
#define SCHOST_UART_BAUDRATE    115200
#define SCHOST_UART_DEV         "/dev/ttyH0"
#define SCHOST_UART_DEF_PWR     80

extern struct option_args_t conf;

extern void usage(const char* app_name);
extern int parse_args(int argc, char** argv, struct option_args_t* args);
extern void print_args(const struct option_args_t* args);

#endif //SC_HOST_OPTIONS_H_
