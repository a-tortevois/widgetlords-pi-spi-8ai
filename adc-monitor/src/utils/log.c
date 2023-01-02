//
// Created by Alexandre Tortevois

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <sys/time.h>

#include "config.h"
#include "utils/log.h"

#define MAX_BUFFER_SIZE (2*1024) // 2ko

static const char *label_level[] = {"OFF", "FATAL", "ERROR", "WARN", "INFO", "DEBUG", "TRACE"};
static level_id LOG_LEVEL;

// -- Private functions ---------------------------------------------------------------------

static int get_syslog_priority(level_id level) {
    switch (level) {
        case LEVEL_FATAL:
        case LEVEL_ERROR:
        case LEVEL_WARN:
            return LOG_WARNING;
        case LEVEL_INFO:
            return LOG_INFO;
        case LEVEL_DEBUG:
        case LEVEL_TRACE:
            return LOG_DEBUG;
        default :
            return LOG_DEBUG;
    }
}

static int log_write_to_file(const char *filename, const char *format, ...) {
    int rc = 0;
    FILE *fp = NULL;
    if ((fp = fopen(filename, "a")) == NULL) {
        rc = -1;
        goto __error;
    }

    va_list args;
    va_start (args, format);
    rc = vfprintf(fp, format, args);
    va_end (args);

    __error:
    if (fp != NULL) {
        if (fclose(fp) != 0) {
            printf("Unable to close file %s: %s\n", filename, strerror(errno));
        }
    }
    return rc;
}

// ------------------------------------------------------------------------------------------

void set_log_level(level_id level) {
    LOG_LEVEL = level;
}

void log_add(level_id level, const char *file, const char *function, int line, const char *msg, ...) {
    if (level > LOG_LEVEL)
        return;

    int rc = 0;
    char buffer[MAX_BUFFER_SIZE];
    va_list args;
    va_start (args, msg);
    rc = vsnprintf(buffer, sizeof(buffer), msg, args);
    va_end (args);

    char str_time[26];
    struct tm *tm_info;
    struct timeval tval;
    gettimeofday(&tval, NULL);
    tm_info = localtime(&tval.tv_sec);
    strftime(str_time, sizeof(str_time), "%Y-%m-%d %H:%M:%S", tm_info);

    if (rc < 0) {
        printf("%s.%06ld %-5s %s %s\n", str_time, (long int) tval.tv_usec, label_level[LEVEL_FATAL], "log_add vsnprintf error: ", strerror(errno));
        return;
    } else if (sizeof(buffer) < rc) {
        printf("%s.%06ld %-5s %s\n", str_time, (long int) tval.tv_usec, label_level[LEVEL_FATAL], "log_add vsnprintf error: buffer overflow");
        return;
    }

#if DEBUG_CONSOLE
    // Console output
    printf("%s.%06ld %-5s %s [%s:%s:%d]\n", str_time, (long int) tval.tv_usec, label_level[level], buffer, file, function, line);
#endif

    // Add into syslog `cat /var/log/syslog | grep nebul_rpi`
    if (level <= LEVEL_WARN) {
        // Log only LEVEL_WARN, LEVEL_ERROR, LEVEL_FATAL
        syslog(get_syslog_priority(level), "%s (%s:%s:%d) \n", buffer, file, function, line);
    }

    // Log into file
    char filename[32];
    char date[9];
    memset(filename, 0, sizeof(filename));
    memset(date, 0, sizeof(date));
    strftime(date, sizeof(date), "%Y%m%d", tm_info);
    snprintf(filename, sizeof(filename) - 1, "%s%s.log", LOGS_PATH, date);
    if (log_write_to_file(filename, "%s:%s:%d;%s.%06ld;%-5s;%s\n", file, function, line, str_time, (long int) tval.tv_usec, label_level[level], buffer) < 0) {
        printf("%s.%06ld %-5s Unable to write into the log file '%s': %s\n", str_time, (long int) tval.tv_usec, label_level[LEVEL_FATAL], filename, strerror(errno));
    }
}