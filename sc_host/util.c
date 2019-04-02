#include "debug.h"
#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>


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
    *pdata = (char*)calloc(1, len+1);
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

    error(0, 0, "error code: 0x%.4x,  %s", result, errptr);

    return errptr;
}
