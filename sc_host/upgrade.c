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

static int upgrade_bt_fw(const char* firmware_file)
{
    struct stat st;
    if (stat(firmware_file, &st) < 0) {
        printf("failed to get firmware size: %s\n", strerror(errno));
        return -errno;
    }
    int fw_size = (int)st.st_size;

    gecko_cmd_dfu_flash_set_address(0);
    printf("DFU packet size: %d\n", fw_size);

    int fd = open(firmware_file, O_RDONLY);
    if (fd < 0) {
        printf("failed to open bt firmware: %s\n", strerror(errno));
        return -errno;
    }

    int left_size = fw_size;
    int readlen = 0;
    unsigned char pkt[48] = "";

    while (left_size > 0) {
        int pkt_size = left_size > sizeof(pkt) ? sizeof(pkt) : left_size;
        left_size -= sizeof(pkt);
        while (0 > (readlen = read(fd, pkt, pkt_size)) && errno == EINTR);
        if (readlen < pkt_size) {
            printf("failed to read firmware file: %s\n", strerror(errno));
            close(fd);
        }
        int reason = gecko_cmd_dfu_flash_upload(pkt_size, pkt)->result;
        if (reason) {
            printf("failed to upload dfu packet\n");
            close(fd);
            return reason;
        } else {
            printf("\r%u%%   ", (unsigned)((fw_size - left_size)*100 / fw_size));
        }
    }
    close(fd);

    return gecko_cmd_dfu_flash_upload_finish()->result;
}

int upgrade_event_handler(int msgid, struct gecko_cmd_packet *evt, struct option_args_t* args)
{
    int result = 0;
    struct gecko_msg_dfu_boot_evt_t* dfu_boot_evt;

    /* Handle events */
    switch (msgid){
    case gecko_evt_dfu_boot_id:
        dfu_boot_evt = &evt->data.evt_dfu_boot;
        printf("DFU OK\nBootloader version: %u (0x%x)\n", dfu_boot_evt->version, dfu_boot_evt->version);
        result = upgrade_bt_fw(args->upgrade.firmware);
        if (result) {
            printf("failed to upgrade bt chip fw: %s\n", error_summary(result));
        } else {
            printf("upgrade bt chip fw successfully\n");
        }
        printf("Switch to normal mode\n");
        gecko_cmd_dfu_reset(0);
        exit(0);
        break;

    case gecko_evt_system_boot_id:
        if (access(args->upgrade.firmware, R_OK) < 0) {
            printf("failed to access bt chip's firmware: %s\n", strerror(errno));
            exit(-errno);
        }
        printf("Switch to DFU mode\n");
        gecko_cmd_dfu_reset(1);
        break;

    default:
        break;
    }

    return 0;
}
