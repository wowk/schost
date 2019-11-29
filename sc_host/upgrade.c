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

enum {
    UPGRADE_INIT,
    UPGRADE_NONE,
} upgrade_state = UPGRADE_NONE;


static int upgrade_bt_fw(struct sock_t* sock, const char* firmware_file)
{
    struct stat st;
    if (stat(firmware_file, &st) < 0) {
        printf_socket(sock, "failed to get firmware size: %s", strerror(errno));
        return -errno;
    }
    int fw_size = (int)st.st_size;

    gecko_cmd_dfu_flash_set_address(0);
    printf_socket(sock, "DFU packet size: %d", fw_size);

    int fd = open(firmware_file, O_RDONLY);
    if (fd < 0) {
        printf_socket(sock, "failed to open bt firmware: %s", strerror(errno));
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
            printf_socket(sock, "failed to read firmware file: %s", strerror(errno));
            close(fd);
        }
        int reason = gecko_cmd_dfu_flash_upload(pkt_size, pkt)->result;
        if (reason) {
            printf_socket(sock, "failed to upload dfu packet");
            close(fd);
            return reason;
        } else {
            printf_socket(sock, "\rupgrading:   %.2u%%", (unsigned)((fw_size - left_size)*100 / fw_size));
        }
    }
    close(fd);

    return gecko_cmd_dfu_flash_upload_finish()->result;
}

int upgrade_bootup_handler(struct sock_t* sock, struct option_args_t* args)
{
    if(upgrade_state == UPGRADE_INIT){
        gecko_cmd_le_gap_stop_advertising(0);
        if (access(args->upgrade.firmware, R_OK) < 0) {
            printf_socket(sock, "failed to access bt chip's firmware: %s", strerror(errno));
            send_socket(sock, 0, 1, "", 0);
            upgrade_state = UPGRADE_NONE;
            gecko_cmd_dfu_reset(0);
        }else{
            printf_socket(sock, "Switch to DFU mode");
            gecko_cmd_dfu_reset(1);
        }
    }

    return 0;
}

int upgrade_cmd_handler(struct sock_t* sock, struct option_args_t* args)
{
    ble_system_reset(0);
    upgrade_state = UPGRADE_INIT;
    printf_socket(sock, "reset system to 1");
    return BLE_EVENT_CONTINUE;
}

int upgrade_event_handler(struct sock_t* sock, struct option_args_t* args, struct gecko_cmd_packet* evt)
{
    int ret = BLE_EVENT_CONTINUE;
    int result = 0;
    struct gecko_msg_dfu_boot_evt_t* dfu_boot_evt;
    struct gecko_msg_dfu_boot_failure_evt_t* dfu_boot_failure_evt;

    /* Handle events */
    switch (BGLIB_MSG_ID(evt->header)){
    case gecko_evt_dfu_boot_id:
        dfu_boot_evt = &evt->data.evt_dfu_boot;
        printf_socket(sock, "DFU OK");
        printf_socket(sock, "Bootloader version: %u (0x%x)", dfu_boot_evt->version, dfu_boot_evt->version);
        result = upgrade_bt_fw(sock, args->upgrade.firmware);
        if (result) {
            printf_socket(sock, "failed to upgrade bt chip fw: %s", error_summary(result));
        } else {
            printf_socket(sock, "upgrade bt chip fw successfully");
        }
        printf_socket(sock, "Switch to normal mode");
        ret = BLE_EVENT_STOP;
        break;

    case gecko_evt_dfu_boot_failure_id:
        dfu_boot_failure_evt = &evt->data.evt_dfu_boot_failure;
        printf_socket(sock, "failed to upgrade bt chip fw: %s", error_summary(dfu_boot_failure_evt->reason));
        ret = BLE_EVENT_STOP;
        break;
    default:
        break;
    }

    return ret;
}

int upgrade_cleanup(struct sock_t* sock, struct option_args_t* args)
{
    printf_socket(sock, "reset system to 0");
    upgrade_state = UPGRADE_NONE;
    gecko_cmd_dfu_reset(0);

    return 0;
}
