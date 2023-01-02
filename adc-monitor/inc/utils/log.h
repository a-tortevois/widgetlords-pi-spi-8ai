//
// Created by Alexandre Tortevois

/**
 * Level of logging to be recorded. Options are:
 *   off - turn off all logging (doesn't affect metrics or audit)
 *   fatal - only those errors which make the application unusable should be recorded
 *   error - record errors which are deemed fatal for a particular request + fatal errors
 *   warn - record problems which are non fatal + errors + fatal errors
 *   info - record information about the general running of the application + warn + error + fatal errors
 *   debug - record information which is more verbose than info + info + warn + error + fatal errors
 *   trace - record very detailed logging + debug + info + warn + error + fatal errors
 */

#ifndef H_LOG_H
#define H_LOG_H

typedef enum {
    LEVEL_OFF,
    LEVEL_FATAL,
    LEVEL_ERROR,
    LEVEL_WARN,
    LEVEL_INFO,
    LEVEL_DEBUG,
    LEVEL_TRACE,
} level_id;

void set_log_level(level_id level);
void log_add(level_id level, const char *file, const char *function, int line, const char *msg, ...);

#define log_trace(...) log_add(LEVEL_TRACE, __FILENAME__, __FUNCTION__, __LINE__, __VA_ARGS__) // for SM status, API
#define log_debug(...) log_add(LEVEL_DEBUG, __FILENAME__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define log_info(...)  log_add(LEVEL_INFO,  __FILENAME__, __FUNCTION__, __LINE__, __VA_ARGS__) // for GLOBAL, GPIO, ADC, PWM value change
#define log_warn(...)  log_add(LEVEL_WARN,  __FILENAME__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define log_error(...) log_add(LEVEL_ERROR, __FILENAME__, __FUNCTION__, __LINE__, __VA_ARGS__)
#define log_fatal(...) log_add(LEVEL_FATAL, __FILENAME__, __FUNCTION__, __LINE__, __VA_ARGS__) // fatal error !

#endif //H_LOG_H
