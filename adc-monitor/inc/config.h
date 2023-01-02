//
// Created by Alexandre Tortevois

#ifndef H_CONFIG_H
#define H_CONFIG_H

#include <build_number.h>
#include <stdbool.h>

#ifndef SEC_TO_MS
#define SEC_TO_MS                                                     1000
#endif
#ifndef SEC_TO_US
#define SEC_TO_US                                                  1000000
#endif
#ifndef SEC_TO_NS
#define SEC_TO_NS                                               1000000000
#endif
#ifndef MS_TO_US
#define MS_TO_US                                                      1000
#endif
#ifndef MS_TO_NS
#define MS_TO_NS                                                   1000000
#endif
#ifndef US_TO_NS
#define US_TO_NS                                                      1000
#endif
#ifndef US_TO_MS
#define US_TO_MS                                                      1000
#endif
#ifndef ONE_DAY_TO_SEC
#define ONE_DAY_TO_SEC                                               86400 // (24*60*60)
#endif

#ifndef TICK_BASE
#define TICK_BASE                                                      100 // ms
#endif
#ifndef TICKS_PER_SECOND
#define TICKS_PER_SECOND                           (SEC_TO_MS / TICK_BASE)
#endif

// #include <limits.h>
#ifndef LONG_MAX
#define LONG_MAX                                               2147483647L
#endif
#ifndef LONG_MIN
#define LONG_MIN                                          (-LONG_MAX - 1L)
#endif

#ifndef MIN
#define MIN(a, b) (((a)<(b))?(a):(b))
#endif
#ifndef MAX
#define MAX(a, b) (((a)>(b))?(a):(b))
#endif
#ifndef ARRAY_SIZEOF
#define ARRAY_SIZEOF(x)  (sizeof(x)/sizeof((x)[0]))
#endif

#ifndef LOGS_PATH
#define LOGS_PATH                                            "/root/logs/"
#endif

#endif //H_CONFIG_H
