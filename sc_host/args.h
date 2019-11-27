#ifndef ARGS_H__
#define ARGS_H__

#include <stdint.h>

enum option_e {
    OPT_DEV,
    OPT_SCAN,
    OPT_CONNECT,
    OPT_SHOW,
    OPT_SET,
    OPT_PAIR,
    OPT_UPGRADE,
    OPT_DTM,
        OPT_TX,
        OPT_RX, 
    OPT_ALL,
};


struct dev_arg_t {
    char name[32];
    uint32_t flowctrl;
    uint32_t baudrate;
    uint32_t timeout;
    float txpwr;
};

struct scan_arg_t {
    uint8_t mode;
    uint8_t phy;
    uint8_t type;
    uint32_t timeout;
    uint32_t winsize;
    uint32_t interval;
};

struct conn_arg_t {
    uint8_t on;
    uint8_t address[18];
    uint8_t addrtype;
    uint8_t initphy;
    uint32_t max_interval;
    uint32_t min_interval;
    uint8_t show;
    uint8_t disconn;
};

struct pair_arg_t {
    uint8_t handle;
    uint8_t primary_phy;
    uint8_t second_phy;
    uint8_t on;
    uint8_t mode;
    float txpwr;
    uint8_t off;
};

struct upgrade_arg_t {
    uint8_t on;
    char firmware[512];
};

struct dtm_tx_arg_t {
    uint8_t on;
    uint32_t delay;
    uint8_t pkttype;
    uint32_t pktlen;
    uint8_t channel;
    uint8_t phy;
    uint8_t showpkt;
    float pwr;
};

struct dtm_rx_arg_t {
    uint8_t on;
    uint32_t delay;
    uint8_t channel;
    uint8_t phy;
    uint8_t showpkt;
};

struct dtm_arg_t {
    struct dtm_rx_arg_t rx;
    struct dtm_tx_arg_t tx;
};

struct show_arg_t {
    uint8_t version;
    uint8_t btaddr;
};

struct set_arg_t {
    uint8_t on;
    uint8_t address[18];
    uint8_t name[64];
};

struct option_args_t {
    struct dev_arg_t dev;
    struct scan_arg_t scan;
    struct pair_arg_t pair;
    struct conn_arg_t connect;
    struct show_arg_t show;
    struct upgrade_arg_t upgrade;
    struct set_arg_t set;
    struct dtm_arg_t dtm;

    uint8_t option;
	uint8_t debug;
};



#endif
