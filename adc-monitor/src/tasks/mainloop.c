//
// Created by Alexandre Tortevois

#include <sched.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>

#include "config.h"
#include "tasks/mainloop.h"
#include "drivers/adc_mcp3008.h"
#include "drivers/gpio.h"
#include "utils/log.h"
#include "utils/sched_attr.h"

#define MAINLOOP_RUNTIME                                   (50 * MS_TO_NS)
#define MAINLOOP_PERIOD                             (TICK_BASE * MS_TO_NS)
#define MAINLOOP_DEADLINE                                  (51 * MS_TO_NS)

static pthread_t thread_id = -1;

// -- Private functions ---------------------------------------------------------------------

static int mainloop_init() {
    int rc = 0;
    rc = gpio_init();
    rc = adc_mcp3008_init();
    return rc;
}

static void mainloop_exec() {
    // Read all inputs
    for (adc_mcp3008_input_id id = 0; id < ADC_INPUTS_COUNT; id++) {
        adc_mcp3008_read(id);
    }
}

static int mainloop_clean() {
    int rc = 0;
    rc = gpio_clean();
    rc = adc_mcp3008_clean();
    return rc;
}

static void *mainloop_thread(void __attribute__((unused)) *p) {

    // Allow the thread to be canceled
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

    struct sched_attr sched_attr;
    unsigned int flags = 0;

    sched_attr.size = sizeof(struct sched_attr);
    sched_attr.sched_flags = 0;
    sched_attr.sched_nice = 0;
    sched_attr.sched_priority = 0;
    sched_attr.sched_policy = SCHED_DEADLINE;
    sched_attr.sched_runtime = MAINLOOP_RUNTIME;
    sched_attr.sched_period = MAINLOOP_PERIOD;
    sched_attr.sched_deadline = MAINLOOP_DEADLINE;

    if (sched_setattr(0, &sched_attr, flags) < 0) {
        log_fatal("MainLoop sched_setattr error: %s", strerror(errno));
        goto __error;
    }

    if (mainloop_init() < 0) {
        log_fatal("Unable to init the MainLoop thread");
        goto __error;
    }

    log_info("MainLoop thread is started");
    for (;;) {
        mainloop_exec();
        sched_yield();
    }

    __error:
    log_fatal("End of the MainLoop thread");
    mainloop_clean();
    kill(getppid(), SIGTERM);
    return NULL;
}

// ------------------------------------------------------------------------------------------

int mainloop_start(void) {
    int rc = 0;
    log_info("Start MainLoop");
    if ((rc = pthread_create(&thread_id, NULL, mainloop_thread, NULL)) != 0) {
        rc = -1;
        log_fatal("Unable to create the mainloop thread: %s", strerror(errno));
        goto __error;
    }

    __error:
    return rc;
}

int mainloop_stop(void) {
    log_info("Stop MainLoop");
    if (thread_id != -1) {
        pthread_cancel(thread_id);
        mainloop_clean();
        pthread_join(thread_id, NULL);
    }
    log_fatal("MainLoop thread is stopped");
    return 0;
}