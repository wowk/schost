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

static int scan_time = 0;
static int scan_monotic_id = 0;


static inline const char* scan_mac_to_str(struct ether_addr* ethaddr, char* buf)
{
    uint8_t* p = (uint8_t*)ethaddr;
    sprintf(buf, "%.2x:%.2x:%.2x:%.2x:%.2x:%.2x", p[5], p[4], p[3], p[2], p[1], p[0]);
    return buf;
}

int  scan_event_handler(int msgid, struct gecko_cmd_packet *evt, const struct option_args_t* args)
{
    char mac[18] = "";
    struct gecko_msg_le_gap_scan_response_evt_t* scanrsptr = NULL;


    switch (msgid) {
    case gecko_evt_system_boot_id:
        remove(BLE_NEIGHBORS);
        gecko_cmd_le_gap_set_discovery_timing(1, args->scan.interval, args->scan.winsize);
        gecko_cmd_le_gap_set_discovery_type(1, args->scan.type);
        gecko_cmd_le_gap_start_discovery(1, args->scan.mode);
        gecko_cmd_hardware_set_soft_timer(32768,0,0);
        break;

    case gecko_evt_le_gap_scan_response_id:
        scanrsptr = &evt->data.evt_le_gap_scan_response;

        if (scan_time < (args->scan.timeout/2)) {
            echo(1, BLE_NEIGHBORS, "%-10d\t%-4d\t%-18s\t%-u\n", scan_monotic_id++, 1,
                 scan_mac_to_str((struct ether_addr*)&scanrsptr->address, mac), scanrsptr->address_type);
            //printf("%-10d\t%-4d\t%-18s\t%-u\n", scan_monotic_id-1, 1,
            //       scan_mac_to_str((struct ether_addr*)&scanrsptr->address, mac), scanrsptr->address_type);
        } else {
            echo(1, BLE_NEIGHBORS, "%-10d\t%-4d\t%-18s\t%-u\n", scan_monotic_id++, 4,
                 scan_mac_to_str((struct ether_addr*)&scanrsptr->address, mac), scanrsptr->address_type);
            //printf("%-10d\t%-4d\t%-18s\t%-u\n", scan_monotic_id-1, 4,
            //       scan_mac_to_str((struct ether_addr*)&scanrsptr->address, mac), scanrsptr->address_type);
        }
        break;

    case gecko_evt_hardware_soft_timer_id:
        if (scan_time > args->scan.timeout) {
            exit(0);
        } else if (scan_time > (args->scan.timeout/2)) {
            gecko_cmd_le_gap_set_discovery_timing(4, args->scan.interval, args->scan.winsize);
            gecko_cmd_le_gap_set_discovery_type(4, args->scan.type);
            gecko_cmd_le_gap_start_discovery(4, args->scan.mode);
        }
        scan_time ++;
        break;

    default:
        break;
    }

    return 0;
}
