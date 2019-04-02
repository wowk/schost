#ifndef ARGS_H__
#define ARGS_H__

#include <stdint.h>

struct option_args_t {
    struct {
        uint8_t on;
	    char name[32];
	    uint32_t flowctrl;
	    uint32_t baudrate;
	    uint32_t timeout;
	    float txpwr;
    } dev;

    struct {
        uint8_t on;
        uint8_t mode;
        uint8_t type;
        uint32_t timeout;
        uint32_t winsize;
        uint32_t interval;
    } scan;

    struct {
        uint8_t on;
        uint8_t address[18];
        uint8_t addrtype;
        uint8_t initphy;
        uint32_t max_interval;
        uint32_t min_interval;
    } connect;

    struct {
        uint8_t handle;
        uint8_t primary_phy;
        uint8_t second_phy;
        uint8_t on;
        uint8_t mode;
	    float txpwr;
    } pair;
   
    struct {
        uint8_t on;
	    char firmware[512];
    } upgrade;
    
    struct {
        uint8_t on;
        struct {
            uint8_t on;
	        uint32_t delay;
	        uint8_t pkttype;
	        uint32_t pktlen;
	        uint8_t channel;
	        uint8_t phy;
	        uint8_t showpkt;
	        float pwr;
        } tx;

        struct {
            uint8_t on;
	        uint32_t delay;
	        uint8_t channel;
	        uint8_t phy;
	        uint8_t showpkt;
        } rx;
    } dtm;
    
    struct {
        uint8_t on;
        uint8_t version;
        uint8_t btaddr;
    } show;

    struct {
        uint8_t on;
        uint8_t address[18];
        uint8_t name[64];
    } set;

	uint8_t debug;
};

#endif
