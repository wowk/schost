/***********************************************************************************************//**
 * \file   app.h
 * \brief  Application header file
 ***************************************************************************************************
 * <b> (C) Copyright 2015 Silicon Labs, http://www.silabs.com</b>
 ***************************************************************************************************
 * This file is licensed under the Silabs License Agreement. See the file
 * "Silabs_License_Agreement.txt" for details. Before using this software for
 * any purpose, you must agree to the terms of that agreement.
 **************************************************************************************************/

#ifndef APP_H
#define APP_H


#include "args.h"
#include "sock.h"

enum {
    BLE_EVENT_STOP,
    BLE_EVENT_CONTINUE,
    BLE_EVENT_RETURN,
};

struct gecko_cmd_packet;

#ifdef __cplusplus
extern "C" {
#endif

int show_cmd_handler(struct sock_t* sock, struct option_args_t* args);
int set_cmd_handler(struct sock_t* sock, struct option_args_t* args);
int set_cleanup(struct sock_t* sock, struct option_args_t* args);

int dtm_cmd_handler(struct sock_t* sock, struct option_args_t* args);
int dtm_event_handler(struct sock_t* sock, struct option_args_t* args, 
                        struct gecko_cmd_packet *evt);

int scan_cmd_handler(struct sock_t* sock, struct option_args_t* args);
int scan_event_handler(struct sock_t* sock, struct option_args_t* args, 
                        struct gecko_cmd_packet *evt);
int scan_cleanup(struct sock_t* sock, struct option_args_t* args);

int connect_cmd_handler(struct sock_t* sock, struct option_args_t* args);
int connect_event_handler(struct sock_t* sock, struct option_args_t* args, 
                        struct gecko_cmd_packet *evt);

int pair_cmd_handler(struct sock_t* sock, struct option_args_t* args);
int pair_event_handler(struct sock_t* sock, struct option_args_t* args, 
                        struct gecko_cmd_packet *evt);
int pair_cleanup(struct sock_t* sock, struct option_args_t* args);

int upgrade_cmd_handler(struct sock_t* sock, struct option_args_t* args);
int upgrade_event_handler(struct sock_t* sock, struct option_args_t* args, 
                        struct gecko_cmd_packet* evt);
int upgrade_cleanup(struct sock_t* sock, struct option_args_t* args);

#ifdef __cplusplus
};
#endif

#endif /* APP_H */
