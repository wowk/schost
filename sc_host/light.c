#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <unistd.h>


size_t makeEffectMsg(uint8_t* buf, size_t len, uint8_t mode)
{
    int i = 2;
    uint8_t value = 1;
    
    memset(buf, 0, len);
    buf[0] = 0xF;
    buf[1] = 0xB;
    buf[2] = 0x12;
    buf[3] = mode;
    buf[4] = mode;
    buf[5] = 0x2;
    buf[13] = 0xFF;
    buf[14] = 0xFF;
    
    while(i < 12){
        value += buf[i++];
    }
    buf[12] = value;

    return 15;
}

size_t makeColorMsg(uint8_t* buf, size_t len, uint8_t r, uint8_t g, uint8_t b, uint8_t br)
{
    int i = 2;
    int value = 1;

    memset(buf, 0, len);
    buf[0] = 0xF;
    buf[1] = 0xD;
    buf[2] = 0x3;
    buf[4] = r;
    buf[5] = g;
    buf[6] = b;
    buf[7] = br;
    buf[15] = 0xFF;
    buf[16] = 0xFF;

    while(i < 14) {
        value += buf[i++];
    }

    buf[14] = value;

    return 17;
}

int main(int argc, char* argv[])
{
    uint8_t offset;
    uint8_t r,g,b,br;
    uint8_t data[17] = "";
    char cmd[256] = "";

    srandom(time(NULL));

    while(1){
        r = (uint8_t)(random()%256);
        g = (uint8_t)(random()%256);
        b = (uint8_t)(random()%256);
        br = (uint8_t)(random()%256);
        makeColorMsg(data, sizeof(data), r, g, b, br);
        //makeEffectMsg(data, sizeof(data), 1+(br%4));
        memset(cmd, 0, sizeof(cmd));
        offset = sprintf(cmd, "/tmp/sc_host --gatt --uuid %s --connid 1 --write ", argv[1]);
        for(int i = 0 ; i < 17 ; i ++){
            unsigned d = data[i];
            if((d>>4) > 9){
                cmd[offset] = (d>>4) - 10 + 'a';
            }else{
                cmd[offset] = (d>>4) + '0';

            }
            offset += 1;
            if((d&0xf) > 9){
                cmd[offset] = (d&0xf) - 10 + 'a';

            }else{
                cmd[offset] = (d&0xf) + '0';

            }
            offset += 1;
        }
        system(cmd);
    }

    return 0;
}
