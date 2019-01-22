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


int show_event_handler(int msgid, struct gecko_cmd_packet *evt, const struct option_args_t* args)
{
    switch(msgid){
    case gecko_evt_system_boot_id:
        if(args->show.version){
            show_file(stdout, BLE_VERSION, 0);
        }
        if (args->show.btaddr) {
            show_file(stdout, BLE_ADDRESS, 0);
        }
        break;

    default:
        break;
    }

    return 0;
}
