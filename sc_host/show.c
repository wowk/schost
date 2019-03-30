#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <errno.h>
#include <stdbool.h>
#include <time.h>
#include <unistd.h>
#include <netinet/ether.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>
#include "bg_types.h"
#include "gecko_bglib.h"
#include "host_gecko.h"
#include "app.h"
#include "util.h"


int show_cmd_handler(struct sock_t* sock, struct option_args_t* args)
{
    if(args->show.version){
    }
    
    if (args->show.btaddr) {
    }

    return 0;
}
