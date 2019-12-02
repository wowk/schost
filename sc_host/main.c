#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif

#include <stdio.h>
#include <errno.h>
#include <string.h>


extern int schost_main(int argc, char* argv[]);
extern int schostd_main(int argc, char *argv[]);

int main(int argc, char* argv[])
{
    if(!strcmp(program_invocation_short_name, "sc_host")){
        return schost_main(argc, argv);
    }

    return schostd_main(argc, argv);
}
