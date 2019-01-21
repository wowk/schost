/***********************************************************************************************//**
 * \file   app.c
 * \brief  Event handling and application code for Empty NCP Host application example
 ***************************************************************************************************
 * <b> (C) Copyright 2016 Silicon Labs, http://www.silabs.com</b>
 ***************************************************************************************************
 * This file is licensed under the Silabs License Agreement. See the file
 * "Silabs_License_Agreement.txt" for details. Before using this software for
 * any purpose, you must agree to the terms of that agreement.
 **************************************************************************************************/

/* standard library headers */
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
/* BG stack headers */
#include "bg_types.h"
#include "gecko_bglib.h"
#include "host_gecko.h"
/* Own header */
#include "app.h"
#include "util.h"
// App booted flag
static bool appBooted = false;
static int scan_time = 0;
static int scan_monotic_id = 0;

const char* error_summary(int result)
{
    const char* errptr = "";
    uint16_t type = (result >> 8);

    switch ( type ) {
    case 0x0:
        errptr = "success";
        break;

    case 0x01:
        errptr = "errors related to BGAPI protocol";
        break;

    case 0x02:
        errptr = "bluetooth errors";
        break;

    case 0x03:
        errptr = "errors from security protocol";
        break;

    case 0x04:
        errptr = "errors from attribute protocol";
        break;

    case 0x05:
        errptr = "errors related hardware";
        break;

    case 0x9:
        errptr = "filesystem errors";
        break;

    case 0xa:
        errptr = "application errors";
        break;

    case 0xb:
        errptr = "security errors";
        break;

    case 0xc:
        errptr = "bluetooth mesh errors";
        break;

    default:
        errptr = "unknown error";
        break;
    }

    return errptr;
}

void show_pkt(const char* data, size_t len)
{}

void delay(uint32_t seconds)
{
    time_t start_time = time(NULL);

    while ( time(NULL) < start_time + seconds );
}

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

static inline const char* scan_mac_to_str(struct ether_addr* ethaddr, char* buf)
{
    uint8_t* p = (uint8_t*)ethaddr;
    sprintf(buf, "%.2x:%.2x:%.2x:%.2x:%.2x:%.2x", p[5], p[4], p[3], p[2], p[1], p[0]);
    return buf;
}

/***********************************************************************************************//**
 *  \brief  Event handler function.
 *  \param[in] evt Event pointer.
 **************************************************************************************************/
void appHandleEvents(struct gecko_cmd_packet *evt, const struct option_args_t* args)
{
    char mac[18] = "";
    static int handle = 0;
    static int delay_time;
    static struct gecko_msg_test_dtm_tx_rsp_t* txrsptr = NULL;
    static struct gecko_msg_test_dtm_rx_rsp_t* rxrsptr = NULL;
    static struct gecko_msg_test_dtm_end_rsp_t* endrsptr = NULL;
    static struct gecko_msg_le_gap_connect_rsp_t* connrsptr = NULL;
    struct gecko_msg_le_gap_scan_response_evt_t* scanrsptr = NULL;
    struct gecko_msg_dfu_boot_evt_t* dfu_boot_evt;
    struct gecko_msg_test_dtm_completed_evt_t* completed_evt;
    static enum {
        RX_START_CMD, TX_START_CMD, RX_END_CMD, TX_END_CMD,
    } last_cmd;



    /* Handle events */
    switch (BGLIB_MSG_ID(evt->header)) {
    case gecko_evt_dfu_boot_id:
        dfu_boot_evt = &evt->data.evt_dfu_boot;
        printf("DFU OK\nBootloader version: %u (0x%x)\n", dfu_boot_evt->version, dfu_boot_evt->version);
        int result = upgrade_bt_fw(args->bt_firmware);
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
        appBooted = true;

        /* show version */
        struct gecko_msg_flash_ps_load_rsp_t* rsp = gecko_cmd_flash_ps_load(0x4001);
        echo(0, BLE_VERSION, "Version: %.2X\n", (uint8_t)rsp->value.data[0]);
        if (args->show_ver) {
            show_file(stdout, BLE_VERSION, 0);
            exit(0);
        }

        /* show BT address */
        struct gecko_msg_system_get_bt_address_rsp_t* btaddr = gecko_cmd_system_get_bt_address();
        echo(0, BLE_ADDRESS, "BT address: %s\n", ether_ntoa((struct ether_addr*)btaddr));
        if (args->show_addr) {
            show_file(stdout, BLE_ADDRESS, 0);
            exit(0);
        }

        /* set BT address */
        if (args->set_bt_addr) {
            bd_addr addr;
            memcpy(&addr, ether_aton((char*)args->bt_addr), 6);
            struct gecko_msg_system_set_bt_address_rsp_t* result = gecko_cmd_system_set_bt_address(addr);
            if (result->result) {
                printf("Failed to assign BT address: %s\n", error_summary(result->result));
            } else {
                echo(0, BLE_ADDRESS, "BT address: %s\n", args->bt_addr);
            }
            exit(0);
        }

        /* BT advertising may affect other functions,
         * we should stop it after system boot up and
         * start it when we need it */
        printf("Stop advertising\n");
        gecko_cmd_le_gap_stop_advertising(handle);
        sleep(1);

        if (strlen(args->bt_firmware)) {
            if (access(args->bt_firmware, R_OK) < 0) {
                printf("failed to access bt chip's firmware: %s\n", strerror(errno));
                exit(-errno);
            }
            printf("Switch to DFU mode\n");
            gecko_cmd_dfu_reset(1);
        } else if (args->scan_mode) {
            remove(BLE_NEIGHBORS);
            gecko_cmd_le_gap_set_discovery_timing(1, args->scan_interval, args->scan_winsize);
            gecko_cmd_le_gap_set_discovery_type(1, 1);
            gecko_cmd_le_gap_start_discovery(1, args->scan_type);
            gecko_cmd_hardware_set_soft_timer(32768,0,0); //set 1 second
        } else if (args->connect_mode) {
            remove(BLE_CONNECTION);
            bd_addr addr;
            bd_addr tmp;
            //uint8_t*p1 = (uint8_t*)&addr, *p2 = (uint8_t*)&tmp;
            ether_aton_r((char*)args->bt_addr, (struct ether_addr*)&tmp);
            gecko_cmd_le_gap_set_conn_parameters(100, 1000, 1000, 0x0c80);
            connrsptr = gecko_cmd_le_gap_connect(addr, args->conn_addr_type, args->conn_init_phy);
        } else if (args->pair_mode == 0) {
            printf("System booted. Starting advertising... \n");
            printf("Device is being advertised.\n");

            if ( args->tx_delay > 0 ) {
                gecko_cmd_system_set_tx_power(args->tx_pwr);
                printf("set tx power => %.2lf\n", args->tx_pwr);
            } else {
                gecko_cmd_system_set_tx_power(0);
                printf("set tx power => 0\n");
            }

            gecko_cmd_hardware_set_soft_timer(32768,0,0); //set 1 second
            if ( 0 < args->tx_delay ) {
                printf(">>>>>>>>> start tx test <<<<<<<<<\n");
                txrsptr = gecko_cmd_test_dtm_tx(args->tx_pkttype, args->tx_pktlen, args->tx_channel, args->tx_phy);
                printf("\tsend tx command\n");
                last_cmd = TX_START_CMD;
                delay_time = args->tx_delay / 1000;
            } else if ( 0 < args->rx_delay ) {
                printf(">>>>>>>>> start rx test <<<<<<<<<\n");
                rxrsptr = gecko_cmd_test_dtm_rx(args->rx_channel, args->rx_phy);
                printf("\tsend rx command\n");
                last_cmd = RX_START_CMD;
                delay_time = args->rx_delay / 1000;
            } else {
                printf("\tquit\n");
                exit(0);
            }
        } else {
            gecko_cmd_system_set_tx_power(args->tx_pwr);
            gecko_cmd_le_gap_set_advertise_phy(handle, test_phy_1m, test_phy_2m);
            gecko_cmd_le_gap_set_advertise_timing(handle, 20, 1000, 100, 0);
            gecko_cmd_le_gap_set_advertise_tx_power(handle, args->tx_pwr);
            gecko_cmd_le_gap_start_advertising(handle, le_gap_general_discoverable, le_gap_undirected_connectable);
            struct gecko_msg_system_get_bt_address_rsp_t* btaddr = gecko_cmd_system_get_bt_address();
            printf("BT address: %s\n", ether_ntoa((struct ether_addr*)btaddr));
        }
        break;

    case gecko_evt_le_gap_scan_response_id:
        scanrsptr = &evt->data.evt_le_gap_scan_response;

        if (scan_time < (args->scan_timeout/2)) {
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
        if (args->tx_delay > 0 || args->rx_delay > 0) {
            printf("\tdelay time left: %d\n", delay_time--);
            if ( delay_time == 0 ) {
                printf("\ttime's up");
                if ( last_cmd == TX_START_CMD ) {
                    endrsptr = gecko_cmd_test_dtm_end();
                    last_cmd = TX_END_CMD;
                } else if ( last_cmd == RX_START_CMD ) {
                    endrsptr = gecko_cmd_test_dtm_end();
                    last_cmd = RX_END_CMD;
                } else {
                    break;
                }
                printf("\tsend end command\n");
            }
        } else if (args->scan_mode) {
            if (scan_time > args->scan_timeout) {
                exit(0);
            } else if (scan_time > (args->scan_timeout/2)) {
                gecko_cmd_le_gap_set_discovery_timing(4, args->scan_interval, args->scan_winsize);
                gecko_cmd_le_gap_set_discovery_type(4, 1);
                gecko_cmd_le_gap_start_discovery(4, args->scan_type);
            }
            scan_time ++;
        }
        break;
    case gecko_evt_le_connection_opened_id:
        printf("connected: %u\n", connrsptr->connection);
        break;

    case gecko_evt_le_connection_parameters_id:
        printf("need connect parameters\n");
        break;

    case gecko_evt_test_dtm_completed_id: {
        if ( last_cmd == TX_START_CMD ) {
            printf("\tgot tx test completed event\n");
            if ( txrsptr->result ) {
                printf("\tresult: %.2x, summary: %s\n", txrsptr->result, error_summary(txrsptr->result));
            }
        } else if ( last_cmd == TX_END_CMD ) {
            printf("\tgot end test completed event\n");
            if ( endrsptr->result ) {
                printf("\tresult: %.2x, summary: %s\n", endrsptr->result, error_summary(endrsptr->result));
            }
            completed_evt = &evt->data.evt_test_dtm_completed;
            printf("\ttotal %u pkts were sent\n", completed_evt->number_of_packets);
            printf("<<<<<<<<< finish tx test >>>>>>>>>\n\n\n");
            if ( args->rx_delay > 0 ) {
                gecko_cmd_hardware_set_soft_timer(0,0,0); //set 1 second
                gecko_cmd_system_set_tx_power(0);
                printf("set tx power => 0\n");
                printf("wait 5s\n");
                delay(5);
                gecko_cmd_hardware_set_soft_timer(32768,0,0); //set 1 second
                printf(">>>>>>>>> start rx test <<<<<<<<<\n");
                rxrsptr = gecko_cmd_test_dtm_rx(args->rx_channel, args->rx_phy);
                printf("\tsend rx command\n");
                last_cmd = RX_START_CMD;
                delay_time = args->rx_delay / 1000;
            } else {
                printf("\tquit\n");
                exit(0);
            }
        } else if ( last_cmd == RX_START_CMD ) {
            printf("\tgot rx test completed event\n");
            if ( rxrsptr->result ) {
                printf("\tresult: %.2x, summary: %s\n", rxrsptr->result, error_summary(rxrsptr->result));
            }
        } else if ( last_cmd == RX_END_CMD ) {
            printf("\tgot end test completed event\n");
            if ( endrsptr->result ) {
                printf("\tresult: %.2x, summary: %s\n", endrsptr->result, error_summary(endrsptr->result));
            }
            completed_evt = &evt->data.evt_test_dtm_completed;
            printf("\ttotal %u pkts were received\n", completed_evt->number_of_packets);
            printf("<<<<<<<<< finish rx test >>>>>>>>>\n\n\n");
            exit(0);
        }
        break;
    }

    case gecko_evt_le_connection_closed_id:
        printf("closed event\n");
        appBooted = false;
        gecko_cmd_system_reset(0);
        /* Restart general advertising and re-enable connections after disconnection. */
        //gecko_cmd_le_gap_start_advertising(handle, le_gap_general_discoverable, le_gap_undirected_connectable);

        break;

    default:
        break;
    }
}
