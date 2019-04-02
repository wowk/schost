//#include "host_gecko.h"
#include <args.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <limits.h>
#include <error.h>
#include <errno.h>
#include <getopt.h>
#include <netinet/ether.h>
#include "host_gecko.h"

 /** The default serial port to use for BGAPI communication. */
#if ((_WIN32 == 1) || (__CYGWIN__ == 1))
static char *default_uart_port = "COM0";
#elif __APPLE__ == 1
static char *default_uart_port = "/dev/tty.usbmodem14171";
#elif __linux == 1
static char *default_uart_port = "/dev/ttyH0";
#else
static char *default_uart_port = "";
#endif

/** The default baud rate to use. */
static uint32_t default_baud_rate = 115200;

// /** The serial port to use for BGAPI communication. */
// static char *uart_port = NULL;

// /** The baud rate to use. */
// static uint32_t baud_rate = 0;


static struct option options[] = {
    /* general options */
    {"debug",           no_argument,        0, 0},
    {"help",            no_argument,        0, 0},
    
    /* set sub options */
    {"set",             no_argument,        0, 0},
        {"address",     required_argument,  0, 0},
        {"name",        required_argument,  0, 0},
    
    /* show sub options */
    {"show",            no_argument,        0, 0},
        {"version",     no_argument,        0, 0},
        {"btaddr",      no_argument,        0, 0},

    /* dtm sub options */
    {"dtm",             no_argument,        0, 0},
        /* dtm.tx sub options */
        {"tx",          no_argument,        0, 0},
            {"delay",   required_argument,  0, 0},
            {"channel", required_argument,  0, 0},
            {"phy",     required_argument,  0, 0},
            {"showpkt", no_argument,        0, 0},
            {"pwr",     required_argument,  0, 0},
            {"pkttype", required_argument,  0, 0},
            {"pktlen",  required_argument,  0, 0},
        
        /* dtm.rx sub options */
        {"rx",          no_argument,        0, 0},
            {"delay",   required_argument,  0, 0},
            {"channel", required_argument,  0, 0},
            {"phy",     required_argument,  0, 0},
            {"showpkt", no_argument,        0, 0},

    /* enter pair mode */
    {"pair",            no_argument,        0, 0},
        {"mode",        required_argument,  0, 0},
        {"txpwr",       required_argument,  0, 0},
        {"handle",      required_argument,  0, 0},
        {"pphy",        required_argument,  0, 0},
        {"sphy",        required_argument,  0, 0},

    /* scan sub options */
    {"scan",            no_argument,        0, 0},
        {"phy",         required_argument,  0, 0},
        {"mode",        required_argument,  0, 0},
        {"interval",    required_argument,  0, 0},
        {"winsize",     required_argument,  0, 0},
        {"type",        required_argument,  0, 0},
        {"timeout",     required_argument,  0, 0},

    /* connect sub options */
    {"connect",         no_argument,        0, 0},
        {"address",     required_argument,  0, 0},
        {"addrtype",    required_argument,  0, 0},
        {"initphy",     required_argument,  0, 0},

    /* upgrade BT firmware */
    {"upgrade",         no_argument,        0, 0},
        {"firmware",    required_argument,  0, 0},

    /* dev sub options */
    {"dev",             no_argument,        0, 0},
        {"name",        required_argument,  0, 0},
        {"flowctrl",    required_argument,  0, 0},
        {"baudrate",    required_argument,  0, 0},
        {"timeout",     required_argument,  0, 0},
        {"txpwr",        required_argument,  0, 0},
    
    /* end */
    {0, 0, 0, 0},
};

enum sub_option_e {
    OPT_ALL,
        OPT_DTM,
            OPT_TX,
            OPT_RX, 
        OPT_DEV,
        OPT_SCAN,
        OPT_CONNECT,
        OPT_SHOW,
        OPT_SET,
        OPT_PAIR,
        OPT_UPGRADE,
} sub_opt_status;

static int parse_int(char* s, long* value, long min, long max, const char* name)
{
    int base;
    char* endptr = NULL;

    if ( !s || !value ) {
        errno = EINVAL;
        goto return_error;
    }

    if ( 0 == strncasecmp(s, "0x", 2) ) {
        base = 16;
    } else if ( *s == '0' ) {
        base = 8;
    } else {
        base = 10;
    }

    errno = 0;
    *value = strtol(s, &endptr, base);
    if ( *endptr || errno == EINVAL ) {
        errno = EINVAL;
        goto return_error;
    } else if ( errno == ERANGE || (min > *value) || (max < *value) ) {
        errno = ERANGE;
        goto return_error;
    }
    return 0;

return_error:
    error(errno, errno, "failed to parse option <%s>", name);
    return -errno;
}

static int parse_float(char* s, double* value, double min, double max, const char* name)
{
    char* endptr = NULL;

    if ( !s || !value ) {
        errno = EINVAL;
        goto return_error;
    }

    errno = 0;
    *value = strtod(s, &endptr);
    if ( *endptr || errno == EINVAL ) {
        errno = EINVAL;
        goto return_error;
    } else if ( errno == ERANGE || (min > *value) || (max < *value) ) {
        errno = ERANGE;
        goto return_error;
    }

    return 0;

return_error:
    error(errno, errno, "failed to parse option <%s>", name);
    return -EINVAL;
}

static int parse_pkttype(char* s, long* value, const char* name)
{
    if ( !strcasecmp(s, "pkt_prbs9") ) {
        *value = test_pkt_prbs9;
    } else if ( !strcasecmp(s, "pkt_11110000") ) {
        *value = test_pkt_11110000;
    } else if ( !strcasecmp(s, "pkt_10101010") ) {
        *value = test_pkt_10101010;
    } else if ( !strcasecmp(s, "pkt_carrier") ) {
        *value = test_pkt_carrier;
    } else if ( !strcasecmp(s, "pkt_11111111") ) {
        *value = test_pkt_11111111;
    } else if ( !strcasecmp(s, "pkt_00000000") ) {
        *value = test_pkt_00000000;
    } else if ( !strcasecmp(s, "pkt_00001111") ) {
        *value = test_pkt_00001111;
    } else if ( !strcasecmp(s, "pkt_01010101") ) {
        *value = test_pkt_01010101;
    } else if ( !strcasecmp(s, "pkt_pn9") ) {
        *value = test_pkt_pn9;
    } else if ( !strcasecmp(s, "pkt_carrier_deprecated") ) {
        *value = test_pkt_carrier_deprecated;
    } else{
        parse_int(s, value, 0, 0xff, name);
        if(*value < 0 || (*value > 7 && *value != 0xfe && *value != 0xfd)){
            error(errno, errno, "failed to parse option <%s>", name);
        }
    }

    return 0;
}

static int parse_phy(char* s, long* value, const char* name)
{
    if ( !strcasecmp(s, "1m") ) {
        *value = test_phy_1m;
    } else if ( !strcasecmp(s, "2m") ) {
        *value = test_phy_2m;
    } else if ( !strcasecmp(s, "125k") ) {
        *value = test_phy_125k;
    } else if ( !strcasecmp(s, "500k") ) {
        *value = test_phy_500k;
    }else{
        return parse_int(s, value, 1, 4, name);
    }

    return 0;
}

static int parse_scan_phy(char* s, long* value, const char* name)
{
    if(!strcasecmp(s, "1m")){
        *value = le_gap_phy_1m;
    }else if(!strcasecmp(s, "2m")){
        *value = le_gap_phy_2m;
    }else if(!strcasecmp(s, "coded")){
        *value = le_gap_phy_coded;
    }else if(!strcasecmp(s, "1m_coded")){
        *value = 5;
    }else{
        parse_int(s, value, 1, 5, name);
        if(*value == 3){
            error(errno, errno, "failed to parse <%s>", name);
        }
    }

    return 0;
}

static int parse_scan_mode(char* s, long* value, const char* name)
{
    if(!strcasecmp(s, "limited")){
        *value = le_gap_discover_limited;
    }else if(!strcasecmp(s, "generic")){
        *value = le_gap_discover_generic;
    }else if(!strcasecmp(s, "observation")){
        *value = le_gap_discover_observation;
    }else{
        parse_int(s, value, 0, 2, name);
    }

    return 0;
}

static int parse_init_phy(char* s, long* value, const char* name)
{
    if(!strcasecmp(s, "1m")){
        *value = le_gap_phy_1m;
    }else if(!strcasecmp(s, "2m")){
        *value = le_gap_phy_2m;
    }else if(!strcasecmp(s, "coded")){
        *value = le_gap_phy_coded;
    }else{
        parse_int(s, value, 1, 4, name);
        if(*value == 3){
            error(errno, errno, "failed to parse <%s>", name);
        }
    }

    return 0;
}

static int parse_primary_phy(char* s, long* value, const char* name)
{
    if(!strcasecmp(s, "1m")){
        *value = le_gap_phy_1m;
    }else if(!strcasecmp(s, "coded")){
        *value = le_gap_phy_coded;
    }else{
        parse_int(s, value, 1, 4, name);
        if(*value == 3 || *value == 2){
            error(errno, errno, "failed to parse <%s>", name);
        }
    }

    return 0;
}

static int parse_second_phy(char* s, long* value, const char* name)
{
    if(!strcasecmp(s, "1m")){
        *value = le_gap_phy_1m;
    }else if(!strcasecmp(s, "2m")){
        *value = le_gap_phy_2m;
    }else if(!strcasecmp(s, "coded")){
        *value = le_gap_phy_coded;
    }else{
        parse_int(s, value, 1, 4, name);
        if(*value == 3){
            error(errno, errno, "failed to parse <%s>", name);
        }
    }

    return 0;
}

static int parse_macaddr(char* s, char* buf, size_t buflen, const char* name)
{
    if(buflen < 18){
        error(ENOBUFS, ENOBUFS, "failed to parse <%s>", name);
    }else if(!ether_aton(s)){
        error(EINVAL, EINVAL, "failed to parse <%s>", name);
    }else{
        snprintf(buf, buflen, "%s", s);
    }
    
    return 0;
}

void print_args(const struct option_args_t* args)
{
    if(args->dev.on){
        printf(">>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\ndev:\n");
        printf("\t%-16s: %s\n",     "name",     args->dev.name);
        printf("\t%-16s: %.8x\n",   "flowctrl", args->dev.flowctrl);
        printf("\t%-16s: %u\n",     "baudrate", args->dev.baudrate);
        printf("\t%-16s: %u\n",     "timeout",  args->dev.timeout);
    }

    if(args->show.on){
        printf("\n>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\nshow:\n");
        printf("\t%-16s: %u\n",     "version",  args->show.version);
        printf("\t%-16s: %u\n",     "address",  args->show.btaddr);

    }

    if(args->set.on){
        printf("\n>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\nset:\n");
        printf("\t%-16s: %s\n",     "address",  args->set.address);
        printf("\t%-16s: %s\n",     "name",     args->set.name);
    }

    if(args->scan.on){
        printf("\n>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\nscan:\n");
        printf("\t%-16s: %u\n",     "mode",     args->scan.mode);
        printf("\t%-16s: %u\n",     "type",     args->scan.type);
        printf("\t%-16s: %u\n",     "timeout",  args->scan.timeout);
        printf("\t%-16s: %u\n",     "winsize",  args->scan.winsize);
        printf("\t%-16s: %u\n",     "interval", args->scan.interval);
    }

    if(args->connect.on){
        printf("\n>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\nconnect:\n");
        printf("\t%-16s: %s\n",     "address",  args->connect.address);
        printf("\t%-16s: %u\n",     "addrtype", args->connect.addrtype);
        printf("\t%-16s: %u\n",     "initphy",  args->connect.initphy);
        printf("\t%-16s: %u\n",     "min_interval", args->connect.min_interval);
        printf("\t%-16s: %u\n",     "max_interval", args->connect.max_interval);
    }

    if(args->pair.on){
        printf("\n>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\npair:\n");
        printf("\t%-16s: %u\n",     "mode", args->pair.mode);
    }

    if(args->upgrade.on){
        printf("\n>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\nupgrade:\n");
        printf("\t%-16s: %s\n",     "firmware", args->upgrade.firmware);
    }

    if(args->dtm.on){
        printf("\n>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>\ndtm:\n");
        if(args->dtm.tx.on){
            printf("\ttx:\n");
            printf("\t\t%-16s: %u\n",   "delay",    args->dtm.tx.delay);
            printf("\t\t%-16s: %u\n",   "channel",  args->dtm.tx.channel);
            printf("\t\t%-16s: %u\n",   "phy",      args->dtm.tx.phy);
            printf("\t\t%-16s: %u\n",   "showpkt",  args->dtm.tx.showpkt);
            printf("\t\t%-16s: %.8x\n", "pkttype",  args->dtm.tx.pkttype);
            printf("\t\t%-16s: %u\n",   "pktlen",   args->dtm.tx.pktlen);
            printf("\t\t%-16s: %f\n",   "pwr",      args->dtm.tx.pwr);
        }

        if(args->dtm.rx.on){
            printf("\n\trx:\n");
            printf("\t\t%-16s: %u\n",   "delay",    args->dtm.rx.delay);
            printf("\t\t%-16s: %u\n",   "channel",  args->dtm.rx.channel);
            printf("\t\t%-16s: %u\n",   "phy",      args->dtm.rx.phy);
            printf("\t\t%-16s: %u\n",   "showpkt",  args->dtm.rx.showpkt);
        }
    }
}

void usage(const char* app_name)
{
    printf("usage: %s\n" 
           "\t[ --help    ]                                                : show help info\n\n"
           "\t[ --debug   ]                                                : show debug info\n\n"
           "\t[ --show    ]                                                : show system info\n"
           "\t       [ --version  ]                                        : show version\n"
           "\t       [ --btaddr   ]                                        : show BT address\n\n"
           "\t[ --set     ]                                                : configure system\n"
           "\t       [ --name     ] <name>                                 : set BT SSID\n"
           "\t       [ --address  ] <BT address>                           : set BT address\n\n"
           "\t[ --scan    ]                                                : scan BT SSID\n"
           "\t       [ --interval ]                                        : set scan interval\n"
           "\t       [ --winsize  ]                                        : set scan window size\n"
           "\t       [ --type     ]                                        : set scan type\n"
           "\t       [ --mode     ]                                        : set scan mode\n"
           "\t       [ --timeout  ]                                        : set scan timeout\n\n"
           "\t[ --connect ]                                                : connect device\n"
           "\t       [ --address  ]                                        : set remote device's address\n"
           "\t       [ --addrtype ]                                        : set address type\n"
           "\t       [ --initphy  ]                                        : set init phy\n\n"
           "\t[ --upgrade ]                                                : upgrade BT firmware\n"
           "\t       [ --firmware ] <firmware>                             : firmware file\n\n"
           "\t[ --dtm   ]                                                  : dtm test\n"
           "\t       [ --tx       ]                                        : config tx\n"
           "\t                [ --channel | -c ]   <0-39>                  : working channel\n"
           "\t                [ --phy     | -p ]   <1M | 2M | 125K | 500K> : phy type\n"
           "\t                [ --pkttype | -t ]   <pkt_pn9 | ...>         : packet type\n"
           "\t                [ --pktlen  | -l ]                           : packet length\n"
           "\t                [ --pwr     | -P ]                           : set dtm tx power\n"
           "\t                [ --delay   | -d ]                           : set tx test time\n"
           "\t                [ --showpkt | -s ]                           : show packet in hex format\n"
           "\t       [ --rx       ]                                        : Config rx\n"
           "\t                [ --channel | -c ]   <0-39>                  : working channel\n"
           "\t                [ --phy     | -p ]   <1M | 2M | 125K | 500K> : phy type\n"
           "\t                [ --delay   | -d ]                           : rx test time\n"
           "\t                [ --showpkt | -s ]                           : show packet in hex format\n\n"
           "\t[ --pair  ]                                                  : pair new device\n"
           "\t       [ --mode     ]                                        : pairing mode\n\n"
           "\t[ --dev   ]                                                  : Config uart\n"
           "\t       [ --baudrate ]                                        : set baudrate\n"
           "\t       [ --flowctrl ]                                        : set flowctrl bits\n"
           "\t       [ --timeout  ]                                        : set timeout\n"
           "\t       [ --name     ]                                        : set uart device\n"
           "\t       [ --txpwr    ]                                        : set system tx pwr\n",
           app_name);
}

int parse_args(int argc, char** argv, struct option_args_t* args)
{
    int op;
    int option_index;
    long value;
    double dvalue;

    if ( argc < 2 ) {
        usage(argv[0]);
        return -EINVAL;
    }
    
    memset(args, 0, sizeof(struct option_args_t));

    /* set default args */
    strncpy(args->dev.name, default_uart_port, sizeof(args->dev.name));
    args->dev.timeout   = 1000;
    args->dev.baudrate  = default_baud_rate;
    args->dev.txpwr     = 80.f;
    args->dtm.tx.phy    = test_phy_1m;
    args->dtm.tx.pwr    = 80.0f;
    args->dtm.rx.phy    = test_phy_1m;
    args->dtm.tx.pkttype = test_pkt_prbs9;
    
    args->pair.handle   = 0;
    args->pair.primary_phy = le_gap_phy_1m;
    args->pair.second_phy  = le_gap_phy_1m;
    args->pair.txpwr       = 80.f;
    args->pair.mode        = 0;

    args->scan.type        = 1;
    args->scan.mode        = le_gap_discover_observation;
    args->scan.interval    = 160;
    args->scan.winsize     = 160;
    args->scan.timeout     = 10;
    args->scan.phy         = le_gap_phy_1m;

    int sub_opt_index = 0;
    enum sub_option_e sub_opt_status[16];
    sub_opt_status[0] = OPT_ALL;

    while(-1 != (op=getopt_long_only(argc, argv, "", options, &option_index))){
        //printf("get next option\n");
        while(1){
            if(sub_opt_status[sub_opt_index] == OPT_ALL) {
                if(op == 0 && !strcmp("dtm", options[option_index].name)) {
                    args->dtm.on = 1;
                    sub_opt_status[++sub_opt_index] = OPT_DTM;
                }else if(op == 0 && !strcmp("pair", options[option_index].name)){
                    args->pair.on = 1;
                    sub_opt_status[++sub_opt_index] = OPT_PAIR;
                }else if(op == 0 && !strcmp("upgrade", options[option_index].name)){
                    args->upgrade.on = 1;
                    sub_opt_status[++sub_opt_index] = OPT_UPGRADE;
                }else if(op == 0 && !strcmp("dev", options[option_index].name)){
                    args->dev.on = 1;
                    sub_opt_status[++sub_opt_index] = OPT_DEV;
                }else if(op == 0 && !strcmp("show", options[option_index].name)){
                    args->show.on = 1;
                    sub_opt_status[++sub_opt_index] = OPT_SHOW;
                }else if(op == 0 && !strcmp("set", options[option_index].name)){
                    args->set.on = 1;
                    sub_opt_status[++sub_opt_index] = OPT_SET;
                }else if(op == 0 && !strcmp("scan", options[option_index].name)){
                    args->scan.on = 1;
                    sub_opt_status[++sub_opt_index] = OPT_SCAN;
                }else if(op == 0 && !strcmp("connect", options[option_index].name)){
                    args->connect.on = 1;
                    sub_opt_status[++sub_opt_index] = OPT_CONNECT;
                }else if(op == 0 && !strcmp("debug", options[option_index].name)){
                    args->debug = 1;
                }else if(op == 0 && !strcmp("help", options[option_index].name)){
                    usage(argv[0]);
                    exit(0);
                }else if(op == '?'){
                    exit(-EINVAL);
                }else{
                    /* never get here */
                    exit(-EINVAL);
                }
            }

            /* parse connect's sub option */
            else if(sub_opt_status[sub_opt_index] == OPT_CONNECT){
                if(op == 0 && !strcmp("address", options[option_index].name)){
                    //printf("address: %s\n", optarg);
                    parse_macaddr(optarg, (char*)args->connect.address, sizeof(args->connect.address), "connect.address");
                }else if(op == 0 && !strcmp("addrtype", options[option_index].name)){
                    parse_int(optarg, &value, 0, CHAR_MAX, "connect.addrtype");
                    args->connect.addrtype = (uint8_t)value;
                }else if(op == 0 && !strcmp("initphy", options[option_index].name)){
                    parse_init_phy(optarg, &value, "connect.addrtype");
                    args->connect.initphy = (uint8_t)value;
                }else{
                    sub_opt_index --;
                    continue;
                }
            }

            /* parse pair's sub option */
            else if(sub_opt_status[sub_opt_index] == OPT_PAIR){
                if(op == 0 && !strcmp("mode", options[option_index].name)){
                    parse_int(optarg, &value, 0, CHAR_MAX, "pair.mode");
                    args->pair.mode = (uint8_t)value;
                }else if(op == 0 && !strcmp("txpwr", options[option_index].name)){
                    parse_float(optarg, &dvalue, 0, 256.0f, "pair.txpwr");
                    args->pair.txpwr = (float)value;
                }else if(op == 0 && !strcmp("handle", options[option_index].name)){
                    parse_int(optarg, &value, 0, CHAR_MAX, "pair.handle");
                    args->pair.mode = (uint8_t)value;
                }else if(op == 0 && !strcmp("sphy", options[option_index].name)){
                    parse_second_phy(optarg, &value, "pair.sphy");
                    args->pair.second_phy = (uint8_t)value;
                }else if(op == 0 && !strcmp("pphy", options[option_index].name)){
                    parse_primary_phy(optarg, &value, "pair.pphy");
                    args->pair.primary_phy = (uint8_t)value;
                }else{
                    sub_opt_index --;
                    continue;
                }
            }

            /* parse upgrade's sub options */
            else if(sub_opt_status[sub_opt_index] == OPT_UPGRADE){
                if(op == 0 && !strcmp("firmware", options[option_index].name)){
                    snprintf(args->upgrade.firmware, sizeof(args->upgrade.firmware), "%s", optarg);
                }else{
                    sub_opt_index --;
                    continue;
                }
            }
            
            /* parse set's sub option */
            else if(sub_opt_status[sub_opt_index] == OPT_SET){
                if(op == 0 && !strcmp("address", options[option_index].name)){
                    parse_macaddr(optarg, (char*)args->set.address, sizeof(args->set.address), "set.address");
                }else if(op == 0 && !strcmp("name", options[option_index].name)){
                    strncpy((char*)args->set.name, optarg, strlen((char*)args->set.name));
                }else{
                    sub_opt_index --;
                    continue;
                }
            }

            /* parse show's sub option */
            else if(sub_opt_status[sub_opt_index] == OPT_SHOW){
                if(op == 0 && !strcmp("btaddr", options[option_index].name)){
                    args->show.btaddr = 1;
                }else if(op == 0 && !strcmp("version", options[option_index].name)){
                    args->show.version = 1;
                }else{
                    sub_opt_index --;
                    continue;
                }
            }

            /* parse dtm's sub option */
            else if(sub_opt_status[sub_opt_index] == OPT_DTM){
                //printf("dtm.%s\n", options[option_index].name);
                args->dtm.on = 1;
                if(op == 0 && !strcmp("rx", options[option_index].name)){
                    args->dtm.rx.on = 1;
                    sub_opt_status[++sub_opt_index] = OPT_RX;
                }else if(op == 0 && !strcmp("tx", options[option_index].name)){
                    args->dtm.tx.on = 1;
                    sub_opt_status[++sub_opt_index] = OPT_TX;
                }else{
                    sub_opt_index --;
                    continue;
                }
            }

            /* parse dtm.rx's sub option */
            else if(sub_opt_status[sub_opt_index] == OPT_RX){
                //printf("rx.%s\n", options[option_index].name);
                if(op == 0 && !strcmp("delay", options[option_index].name)){
                    //printf("found delay\n");
                    parse_int(optarg, &value, 0, INT_MAX, "dtm.rx.delay");
                    args->dtm.rx.delay = (uint32_t)value;
                }else if(op == 0 && !strcmp("channel", options[option_index].name)){
                    parse_int(optarg, &value, 0, 39, "dtm.rx.channel");
                    args->dtm.rx.channel = (uint8_t)value;
                }else if(op == 0 && !strcmp("phy", options[option_index].name)){
                    parse_phy(optarg, &value, "dtm.rx.phy");
                    args->dtm.rx.phy = (uint32_t)value;
                }else if(op == 0 && !strcmp("showpkt", options[option_index].name)){
                    args->dtm.rx.showpkt = 1;
                }else{
                    sub_opt_index --;
                    continue;
                }
            }
        
            /* parse dtm.tx's sub option */
            else if(sub_opt_status[sub_opt_index] == OPT_TX){
                //printf("tx\n");
                if(op == 0 && !strcmp("delay", options[option_index].name)){
                    parse_int(optarg, &value, 0, INT_MAX, "dtm.tx.delay");
                    args->dtm.tx.delay = (uint32_t)value;
                }else if(op == 0 && !strcmp("channel", options[option_index].name)){
                    parse_int(optarg, &value, 0, 39, "dtm.tx.channel");
                    args->dtm.tx.channel = (uint8_t)value;
                }else if(op == 0 && !strcmp("phy", options[option_index].name)){
                    parse_phy(optarg, &value, "dtm.tx.phy");
                    args->dtm.tx.phy = (uint32_t)value;
                }else if(op == 0 && !strcmp("showpkt", options[option_index].name)){
                    args->dtm.tx.showpkt = 1;
                }else if(op == 0 && !strcmp("pwr", options[option_index].name)){
                    parse_float(optarg, &dvalue, -100000.0, 100000.0, "dtm.tx.pwr");
                    args->dtm.tx.pwr = (float)dvalue;
                }else if(op == 0 && !strcmp("pkttype", options[option_index].name)){
                    parse_pkttype (optarg, &value, "dtm.tx.pkttype");
                    args->dtm.tx.pkttype = (uint32_t)value;
                }else if(op == 0 && !strcmp("pktlen", options[option_index].name)){
                    parse_int(optarg, &value, 0, 0xffff, "dtm.tx.pktlen");
                    args->dtm.tx.pktlen = (uint32_t)value;
                }else{
                    sub_opt_index --;
                    continue;
                }
            }
            
            /* parse scan's sub option */
            else if ( sub_opt_status[sub_opt_index] == OPT_SCAN ) {
                if ( op == 0 && !strcmp("interval", options[option_index].name) ) {
                    parse_int(optarg, &value, 0, INT_MAX, "scan.interval");
                    args->scan.interval = (uint32_t)value;
                } else if ( op == 0 && !strcmp("winsize", options[option_index].name) ) {
                    parse_int(optarg, &value, 0, INT_MAX, "scan.winsize");
                    args->scan.winsize = (uint32_t)value;
                } else if ( op == 0 && !strcmp("phy", options[option_index].name) ) {
                    parse_scan_phy(optarg, &value, "scan.phy");
                    args->scan.phy = (uint8_t)value;
                } else if ( op == 0 && !strcmp("type", options[option_index].name) ) {
                    parse_int(optarg, &value, 0, 1, "scan.type");
                    args->scan.type = (uint32_t)value;
                } else if ( op == 0 && !strcmp("mode", options[option_index].name) ) {
                    parse_scan_mode(optarg, &value, "scan.mode");
                    args->scan.mode = (uint8_t)value;
                } else if ( op == 0 && !strcmp("timeout", options[option_index].name) ) {
                    parse_int(optarg, &value, 0, INT_MAX, "scan.timeout");
                    args->scan.timeout = (uint32_t)value;
                } else {
                    sub_opt_index --;
                    continue;
                }
            }
            
            /* parse dev's sub option */
            else if ( sub_opt_status[sub_opt_index] == OPT_DEV ) {
                if ( op == 0 && !strcmp("name", options[option_index].name) ) {
                    snprintf(args->dev.name, sizeof(args->dev.name), "%s", optarg);
                } else if ( op == 0 && !strcmp("baudrate", options[option_index].name) ) {
                    parse_int(optarg, &value, 0, INT_MAX, "baudrate");
                    args->dev.baudrate = (uint32_t)value;
                } else if ( op == 0 && !strcmp("flowctrl", options[option_index].name) ) {
                    parse_int(optarg, &value, 0, INT_MAX, "flowctrl");
                    args->dev.flowctrl = (uint32_t)value;
                } else if ( op == 0 && !strcmp("timeout", options[option_index].name) ) {
                    parse_int(optarg, &value, 0, INT_MAX, "timeout");
                    args->dev.timeout = (uint32_t)value;
                } else if ( op == 0 && !strcmp("txpwr", options[option_index].name) ) {
                    parse_float(optarg, &dvalue, -100000.0, 100000.0, "dev.txpwr");
                    args->dev.txpwr = (float)dvalue;
                } else {
                    sub_opt_index --;
                    continue;
                }
            }

            break;
        }
    }

    if ( argv[optind] ) {
        error(EINVAL, EINVAL, "garbage options <%s>", argv[optind]);
        return -EINVAL;
    }

    if ( args->debug ) {
        print_args(args);
    }

    return 0;
}

