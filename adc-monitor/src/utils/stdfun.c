//
// Created by Alexandre Tortevois

#include <ctype.h> // isspace, tolower, isxdigit
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#include "config.h"
#include "utils/stdfun.h"
#include "utils/log.h"

// Check if the directory exists, else try to create it
int dir_exists(char *path) {
    struct stat file_stat;
    if (stat(path, &file_stat) < 0) {
        if (mkdir(path, 0777) < 0) {
            log_fatal("Unable to create the log directory '%s': %s", path, strerror(errno));
            exit(EXIT_FAILURE);
        }
    }
    return 0;
}


// Push one element into the position of the array
int fifo_push(int arr[], size_t size, int *pos, int value) {
    if (size > 0 && *pos >= 0) {
        arr[*pos] = value;
        (*pos)++;
        if (*pos >= size) {
            *pos = 0;
        }
        return 0;
    }
    log_fatal("Unable to push an element into the fifo");
    return -1;
}

// libjson -->

int strtoint(const char *str, int *value) {
    int rc = 0;
    char *endptr = NULL;
    errno = 0; // to distinguish success/failure after call
    long l_value = strtol(str, &endptr, 10);
    if (endptr != NULL && ((errno == ERANGE && (*value == LONG_MAX || *value == LONG_MIN)) || (errno != 0 && *value == 0))) {
        rc = -1;
        log_fatal("strtol conversion error in `%s`: %s", str, strerror(errno));
        goto __error;
    }
    if (str == endptr) {
        rc = -1;
        log_fatal("No digits were found in `%s`", str);
        goto __error;
    }
    *value = (int) l_value;
    __error:
    return rc;
}

#ifndef isxdigit
int isxdigit(int c) {
    return ((c >= 0x30 && c <= 0x39) ||   // 0-9
            (c >= 0x41 && c <= 0x46) ||   // A-F
            (c >= 0x61 && c <= 0x66));    // a-f
}
#endif

//
int atoh(char c, int *i) {
    if (c >= 0x30 && c <= 0x39) {
        *i = c - 0x30;
        return 0;
    } else if (c >= 0x41 && c <= 0x46) {
        *i = c - 0x41 + 0x0A;
        return 0;
    } else if (c >= 0x61 && c <= 0x66) {
        *i = c - 0x61 + 0x0A;
        return 0;
    }
    return -1;
}

// Unicode to UTF-8 char encoder
int uc_to_utf8(int uc, unsigned char *dest) {
    if (uc < 0x00) {
        return 0;
    }
    if (uc < 0x80) {
        dest[0] = (unsigned char) uc;
        return 1;
    }
    if (uc < 0x800) {
        dest[0] = (unsigned char) (0xC0 + (uc >> 6));
        dest[1] = (unsigned char) (0x80 + (uc & 0x3F));
        return 2;
    }
    // Note: we allow encoding 0xd800-0xdfff here, so as not to change the API, however, these are actually invalid in UTF-8
    if (uc < 0x10000) {
        dest[0] = (unsigned char) (0xE0 + (uc >> 12));
        dest[1] = (unsigned char) (0x80 + ((uc >> 6) & 0x3F));
        dest[2] = (unsigned char) (0x80 + (uc & 0x3F));
        return 3;
    }
    if (uc < 0x110000) {
        dest[0] = (unsigned char) (0xF0 + (uc >> 18));
        dest[1] = (unsigned char) (0x80 + ((uc >> 12) & 0x3F));
        dest[2] = (unsigned char) (0x80 + ((uc >> 6) & 0x3F));
        dest[3] = (unsigned char) (0x80 + (uc & 0x3F));
        return 4;
    }
    return 0;
}

// <-- libjson