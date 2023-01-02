//
// Created by Alexandre Tortevois

#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "config.h"
#include "tasks/api.h"
#include "tasks/mainloop.h"
#include "utils/log.h"
#include "utils/stdfun.h"

static int tasks_start(void) {
    int rc = 0;

    if ((rc = mainloop_start()) < 0) {
        goto __error;
    }
    if ((rc = api_server_start()) < 0) {
        goto __error;
    }

    __error:
    return rc;
}

static void tasks_stop(void) {
    mainloop_stop();
    api_server_stop();
}

static void free_all(void) {
    log_fatal("Process termination callback: ending of the program with a safe exit");
    tasks_stop();
    log_fatal("End of program...");
}

static void sig_handler(int sig) {
    switch (sig) {
        case SIGSEGV: //
            log_fatal("Catch SIGSEGV");
            break;

        case SIGTERM:
            log_fatal("Catch SIGTERM");
            break;

        case SIGFPE : // float error
            log_fatal("Catch SIGFPE");
            break;

        default:
            log_fatal("Catch SIG %s", strsignal(sig));
            break;
    }
    exit(EXIT_FAILURE);
}

int main(void) {
    // Callback before exit
    atexit(free_all);

    // Define log level
#if DEBUG_CONSOLE
    set_log_level(LEVEL_TRACE);
#else
    set_log_level(LEVEL_INFO);
#endif

    // Check if directory exists, else create it
    dir_exists(LOGS_PATH);

    // Print version
    log_info("Start ADC_MONITOR v%s-%d compiled at %s %s", ADC_MONITOR_VERSION, ADC_MONITOR_VERSION_BUILD, ADC_MONITOR_COMPILE_DATE, ADC_MONITOR_COMPILE_TIME);

    // Define the callback for signals interception
    struct sigaction sig_action;
    memset(&sig_action, 0, sizeof(sig_action));
    sig_action.sa_handler = sig_handler;

    if (sigaction(SIGSEGV, &sig_action, NULL) < 0) {
        log_fatal("Error: could not define sigaction SIGSEGV");
        goto __error;
    }

    if (sigaction(SIGTERM, &sig_action, NULL) < 0) {
        log_fatal("Error: could not define sigaction SIGTERM");
        goto __error;
    }

    if (sigaction(SIGFPE, &sig_action, NULL) < 0) {
        log_fatal("Error: could not define sigaction SIGFPE");
        goto __error;
    }

    // Start all tasks
    if (tasks_start() < 0) {
        goto __error;
    }

    // Execute forever the loop
    for (;;) {
        usleep(1 * SEC_TO_US);
    }

    __error:
    log_fatal("Error: Main exit");
    return EXIT_FAILURE;
}