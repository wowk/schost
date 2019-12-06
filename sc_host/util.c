#include "debug.h"
#include <util.h>
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <netinet/ether.h>
#include <host_gecko.h>


#define swap(a, b) do{\
    typeof(a) tmp = b;\
    b = a;\
    a = tmp;\
}while(0)

uint16_t to_uuid16(uint8array* uuid)
{
	uint16_t val;
    if(uuid->len == 16){
	    val = (uuid->data[2] << 8) + uuid->data[3];
    }else{
        val = (uuid->data[0]<<8) + uuid->data[1];
    }

	return val;
}

size_t hex2str(uint8_t* data, size_t len, char* buffer)
{
    //strcpy(buffer, "0x");
    //buffer += 2;
    for(int i = 0 ; i < len ; i ++){
        sprintf(buffer, "%.2X ", data[i]);
        buffer += 2;
    }

    return len;
}

char* btaddr2str(void* addr,  char* buf)
{
    uint8_t* p =(uint8_t*)addr;
    const char* fmt = "%.2x:%.2x:%.2x:%.2x:%.2x:%.2x";

    sprintf(buf, fmt, p[5], p[4], p[3], p[2], p[1], p[0]);

    return buf;
}

void* str2btaddr(char* str, void* buf)
{
    uint8_t* p = (uint8_t*)buf;

    ether_aton_r(str, (struct ether_addr*)buf);
    swap(p[5], p[0]);
    swap(p[4], p[1]);
    swap(p[3], p[2]);
    
    return buf;
}

void echo(int append, const char* file, const char* format, ...)
{
    if (append && access(file, R_OK) < 0) {
        append = 0;
    }
    FILE* fp = fopen(file, append ? "a" : "w");
    if (!fp) {
        error(0, errno, "failed to open file <%s>", file);
        return;
    }
    va_list val;
    va_start(val, format);
    vfprintf(fp, format, val);
    va_end(val);
    fclose(fp);
}

size_t cat(const char* file, char** pdata, size_t len)
{
    *pdata = NULL;
    FILE* fp = fopen(file, "r");
    if (!fp) {
        error(0, errno, "failed to open file <%s>", file);
        goto error;
    }
    if (len == 0) {
        fseek(fp, 0, SEEK_END);
        len = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        if (len < 0) {
            error(0, errno, "failed to get file <%s> size", file);
            goto error;
        }
    }
    *pdata = (char*)Calloc(1, len+1);
    if (!(*pdata)) {
        error(0, errno, "failed to malloc");
        goto error;
    }
    len = fread(*pdata, 1, len, fp);
    fclose(fp);

    return len;
error:
    if (fp)
        fclose(fp);
    return -1;
}

void show_file(FILE* fp, const char* file, size_t len)
{
    char* pdata;
    cat(file, &pdata, len);
    if (pdata) {
        fprintf(fp, "%s\n", pdata);
        free(pdata);
    }
}

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

int process_running(const char* pidfile)
{
    if(access(pidfile, F_OK) < 0){
        return 0;
    }
    
    char* pid = NULL;
    cat(pidfile, &pid, sizeof(pid));
    if(!pid){
        return 1;
    }

    char cmdline[64] = "";
    sprintf(cmdline, "/proc/%s/cmdline", pid);
    free(pid);

    if(access(cmdline, F_OK) < 0){
        echo(0, pidfile, "%d", (int)getpid());
        return 0;
    }
    
    return 1;
}
