#ifndef DEBUG_H__
#define DEBUG_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef error
#undef error
#endif

#ifdef debug
#undef debug
#endif

#ifdef info
#undef info
#endif

#define error(need_exit, errcode, fmt, args...)  do{\
    flockfile(stderr);\
    fprintf(stderr, "\033[1;31m");\
    fprintf(stderr, "[ %-.8s ] [ error ] [ %-.16s : %-5d] ", \
            program_invocation_short_name,\
            __FUNCTION__, __LINE__-1);\
    fprintf(stderr, fmt, ##args);\
    if(errcode){\
        fprintf(stderr, "  : %s", strerror(errcode));\
    }\
    fprintf(stderr, "\n\033[1;0m");\
    fflush(stderr);\
    if( need_exit )\
        exit(errcode);\
    funlockfile(stderr);\
}while(0)

#define debug(onoff, fmt, args...)do{\
    if(onoff){\
        flockfile(stdout);\
        fprintf(stdout, "\033[1;32m");\
        fprintf(stdout, "[ %-.8s ] [ debug ] [ %-.16s : %-5d] ", \
            program_invocation_short_name,\
            __FUNCTION__, __LINE__);\
        fprintf(stdout, fmt, ##args);\
        fprintf(stdout, "\033[1;0m");\
        fprintf(stdout, "\n");\
        fflush(stdout);\
        funlockfile(stdout);\
    }\
    }while(0)

#define info(fmt, args...)do{\
    flockfile(stdout);\
    fprintf(stderr, "\033[1;34m");\
    fprintf(stdout, "[ %-.8s ] [ info  ] [ %-.16s : %-5d] ", \
            program_invocation_short_name,\
            __FUNCTION__, __LINE__);\
    fprintf(stdout, fmt, ##args);\
    fprintf(stdout, "\033[1;0m");\
    fprintf(stdout, "\n");\
    fflush(stdout);\
    funlockfile(stdout);\
}while(0)

#endif
