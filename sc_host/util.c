#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

#define debug(fmt, ...) do{\
    printf("[%s:%u] ", __FUNCTION__, __LINE__);\
    printf(fmt, ##__VA_ARGS__);\
    if(errno){\
        printf(": %s\n", strerror(errno));\
    }else{\
        printf("\n");\
    }\
    fflush(stdout);\
}while(0)

void echo(int append, const char* file, const char* format, ...)
{
    if (append && access(file, R_OK) < 0) {
        append = 0;
    }
    FILE* fp = fopen(file, append ? "a" : "w");
    if (!fp) {
        debug("failed to open file <%s>", file);
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
        debug("failed to open file <%s>", file);
        goto error;
    }
    if (len == 0) {
        fseek(fp, 0, SEEK_END);
        len = ftell(fp);
        fseek(fp, 0, SEEK_SET);
        if (len < 0) {
            debug("failed to get file <%s> size", file);
            goto error;
        }
    }
    *pdata = (char*)calloc(1, len+1);
    if (!(*pdata)) {
        debug("failed to malloc");
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
