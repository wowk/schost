#ifndef SC_HOST_OPTIONS_H_
#define SC_HOST_OPTIONS_H_

#include <args.h>

extern void usage(const char* app_name);
extern int parse_args(int argc, char** argv, struct option_args_t* args);
extern void print_args(const struct option_args_t* args);

#endif //SC_HOST_OPTIONS_H_
