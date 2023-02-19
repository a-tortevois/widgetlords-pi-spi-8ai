//
// Created by Alexandre Tortevois

#include <errno.h>
#include <pthread.h>
#include <regex.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/time.h>

#include "config.h"
#include "tasks/api.h"
#include "drivers/adc_mcp3008.h"
#include "drivers/gpio.h"
#include "utils/libjson.h"
#include "utils/log.h"

volatile sig_atomic_t server_is_running = true;
static pthread_t thread_id = -1;
static int server_socket = -1, client_socket = -1;
static char reply_buffer[API_MAX_BUFFER_SIZE];
static char read_buffer[API_MAX_BUFFER_SIZE];
static char i_buffer[API_IV_BUFFER_SIZE];
static char v_buffer[API_IV_BUFFER_SIZE];
static json_object parsed_stack[API_PARSED_STACK_SIZE];

static api_status STATUS[API_STATUS_CODE_COUNT] = {
    // API_SUCCESS
    {
        .code = 200,
        .text = NULL,
    },
    // API_SUCCESS_UPDATE
    {
        .code = 201,
        .text = NULL,
    },
    // API_SUCCESS_WITHOUT_DATA
    {
        .code = 204,
        .text = NULL,
    },
    // API_ERR_BUFFER_REPLY_OVERFLOW
    {
        .code = 400,
        .text = "Unable to build the reply: buffer overflow.",
    },
    // API_ERR_JSON_SYNTAX
    {
        .code = 400,
        .text = "Bad request: unable to parse JSON.",
    },
    // API_ERR_MSG_ID_UNDEFINED
    {
        .code = 401,
        .text = "Bad request: `_msg_id` is undefined.",
    },
    // API_ERR_MSG_ID_NULL
    {
        .code = 402,
        .text = "Bad request: `_msg_id` field is null.",
    },
    // API_ERR_TOPIC_UNDEFINED
    {
        .code = 403,
        .text = "Bad request: `topic` is undefined.",
    },
    // API_ERR_TOPIC_NOT_A_STRING
    {
        .code = 404,
        .text = "Bad request: `topic` is not a string.",
    },
    // API_ERR_TOPIC_NULL
    {
        .code = 405,
        .text = "Bad request: `topic` field is null.",
    },
    // API_ERR_TOPIC_UNKNOWN
    {
        .code = 406,
        .text = "Bad request: `topic` is unknown.",
    },
    // API_ERR_PAYLOAD_UNDEFINED
    {
        .code = 407,
        .text = "Bad request: `payload` is undefined.",
    },
    // API_ERR_INDEX_UNDEFINED
    {
        .code = 408,
        .text = "Bad request: `param.i` is undefined.",
    },
    // API_ERR_INDEX_NOT_AN_ARRAY
    {
        .code = 409,
        .text = "Bad request: `param.i` is not an array.",
    },
    // API_ERR_INDEX_NULL
    {
        .code = 410,
        .text = "Bad request: `param.i` is null.",
    },
    // API_ERR_INDEX_NAN
    {
        .code = 411,
        .text = "Bad request: `param.i:%s` is not a number.",
    },
    // API_ERR_VALUE_UNDEFINED
    {
        .code = 412,
        .text = "Bad request: `param.v` is undefined.",
    },
    // API_ERR_VALUE_NOT_AN_ARRAY
    {
        .code = 413,
        .text = "Bad request: `param.v` is not an array.",
    },
    // API_ERR_VALUE_NULL
    {
        .code = 414,
        .text = "Bad request: `param.v` is null.",
    },
    // API_ERR_VALUE_NAN
    {
        .code = 415,
        .text = "Bad request: `param.v:%s` is not a number.",
    },
    // API_ERR_ARR_NOT_EQUAL
    {
        .code = 416,
        .text = "Bad request: array `param.i` length not equals array `param.v`.",
    },
    // API_ERR_INDEX_NOT_FOUND
    {
        .code = 417,
        .text = "Bad request: `param.i[%d]` not found.",
    },
    // API_ERR_VALUE_NOT_FOUND
    {
        .code = 418,
        .text = "Bad request: `param.v[%d]` not found.",
    },
    // API_ERR_GPIO_ID_UNDEFINED
    {
        .code = 419,
        .text = "Bad request: unable to set the value for `io:%d`, id is undefined.",
    },
    // API_ERR_GPIO_VALUE_OUT_OF_RANGE
    {
        .code = 420,
        .text = "Bad request: value is out of range; unable to set `var:%d` with `val:%d`.",
    },
    // API_ERR_GPIO_UNABLE_TO_SET_VALUE
    {
        .code = 421,
        .text = "Bad request: %s.",
    },
};

// -- Private functions ---------------------------------------------------------------------

static void generate_message_id(char *msg_id, size_t msg_id_len) {
    memset(msg_id, 0, msg_id_len);
    struct timeval tval;
    gettimeofday(&tval, NULL);
    unsigned long long ms = ((unsigned long long) tval.tv_sec * SEC_TO_MS) + (tval.tv_usec / US_TO_MS);
    snprintf(msg_id, msg_id_len - 1, "%llx", ms);
}

static int add_to_payload(char **payload, const char *key, int count, bool with_all, bool (*is_update)(int), int (*get_value)(int), void (*set_update)(int, bool)) {
    int rc = 0;
    char *i_ptr = i_buffer;
    char *v_ptr = v_buffer;
    memset(i_buffer, 0, sizeof(i_buffer));
    memset(v_buffer, 0, sizeof(v_buffer));

    for (int id = 0; id < count; id++) {
        if (with_all || (*is_update)(id)) {
            int value = (*get_value)(id);
            (*set_update)(id, false);
            if ((i_ptr - i_buffer) < (API_IV_BUFFER_SIZE - 4) && (v_ptr - v_buffer) < (API_IV_BUFFER_SIZE - 4)) {
                i_ptr += sprintf(i_ptr, "%d,", id); // len is max 4 char
                v_ptr += sprintf(v_ptr, "%d,", value); // len is max 4 char
            } else {
                rc = -1;
                log_warn("Unable to add id #%d for key \"%s\" to the payload: buffer overflow", id, key);
                goto __error;
            }
        }
    }

    if ((i_ptr - i_buffer) > 0) {
        // Remove the last comma
        *(--i_ptr) = '\0';
        *(--v_ptr) = '\0';

        // *payload - reply_buffer : reply length
        // i_ptr - i_buffer : i length
        // v_ptr - v_buffer : v length
        // 23: "key":{"i":[],"v":[]},\0
        if (((*payload - reply_buffer) + (i_ptr - i_buffer) + (v_ptr - v_buffer) + 23) > (API_MAX_BUFFER_SIZE - 2)) { // add 2 char }} at the end
            rc = -1;
            log_warn("Unable to add key \"%s\" to the payload: buffer overflow");
            goto __error;
        }
        *payload += sprintf(*payload, "\"%s\":{\"i\":[%s],\"v\":[%s]},", key, i_buffer, v_buffer);
    }

    __error:
    return rc;
}

static int build_payload(char **payload, bool with_io, bool with_all_io, bool with_adc, bool with_all_adc, bool with_sw_version) {
    int rc = 0;
    char *ptr = *payload;

    // Add IO to the payload
    if (with_io) {
        if ((rc = add_to_payload(payload, "io", IO_COUNT, with_all_io, (bool (*)(int)) &gpio_is_updated, (int (*)(int)) gpio_get_value, (void (*)(int, bool)) gpio_set_is_updated)) < 0) {
            goto __error;
        }
    }

    // Add ADC to the payload
    if (with_adc) {
        if ((rc = add_to_payload(payload, "adc", ADC_INPUTS_COUNT, with_all_adc, (bool (*)(int)) &adc_mcp3008_is_updated, (int (*)(int)) adc_mcp3008_get_raw_value, (void (*)(int, bool)) adc_mcp3008_set_is_updated)) < 0) {
            goto __error;
        }
    }

    // Add Software version
    if (with_sw_version) {
        if ((*payload - reply_buffer + 50) < API_MAX_BUFFER_SIZE) {
            *payload += sprintf(*payload, "\"sw_version\":\"%s (%x-%d)\",", ADC_MONITOR_VERSION, ADC_MONITOR_COMPILE_TIMESTAMP, ADC_MONITOR_VERSION_BUILD);
        } else {
            rc = -1;
            goto __error;
        }
    }

    if (*payload - ptr > 0) {
        // Remove the last comma
        *(--*payload) = '\0';
    }

    __error:
    return rc;
}

static void build_error_reply(char *msg_id, api_status_code status_code, ...) {
    int rc = 0;
    char error[128];
    memset(error, 0, sizeof(error));
    memset(reply_buffer, 0, sizeof(reply_buffer));
    char *ptr = reply_buffer;

    if (status_code < 0 || status_code >= API_STATUS_CODE_COUNT) {
        log_fatal("Unable to build error reply: status_code #%d is out of range", status_code);
        exit(EXIT_FAILURE);
    }

    va_list args;
    va_start(args, status_code);
    rc = vsnprintf(error, sizeof(error) - 1, STATUS[status_code].text, args);
    va_end(args);

    if (rc < 0) {
        log_warn("Unable to build error reply");
        return;
    }

    // Log the error message
    log_warn(error);

    ptr += sprintf(ptr, "{");
    if (msg_id != NULL) {
        ptr += sprintf(ptr, "\"_msgid\":\"%s\",", msg_id);
    }
    sprintf(ptr, "\"status\":%d,\"timestamp\":%d,\"error\":%s}", STATUS[status_code].code, (int) time(NULL), error);
}

static void build_timeout_reply(void) {
    memset(reply_buffer, 0, sizeof(reply_buffer));
    char *ptr = reply_buffer;
    char msg_id[API_MSG_ID_LEN];
    generate_message_id(msg_id, sizeof(msg_id));
    ptr += sprintf(ptr, "{\"_msgid\":\"%s\",\"status\":%d,\"timestamp\":%d,\"payload\":{", msg_id, STATUS[API_SUCCESS].code, (int) time(NULL));
    if (build_payload(&ptr, true, false, true, false, false) < 0) {
        build_error_reply(msg_id, API_ERR_BUFFER_REPLY_OVERFLOW);
        return;
    }
    // Ensure we don't have a buffer overflow
    // if ((ptr - reply_buffer + 2) < MAX_BUFFER_SIZE) {
    sprintf(ptr, "}}");
    // }
}

static void build_get_all_reply(char *msg_id) {
    memset(reply_buffer, 0, sizeof(reply_buffer));
    char *ptr = reply_buffer;
    ptr += sprintf(ptr, "{\"_msgid\":\"%s\",\"status\":%d,\"timestamp\":%d,\"payload\":{", msg_id, STATUS[API_SUCCESS].code, (int) time(NULL));
    if (build_payload(&ptr, true, true, true, true, true) < 0) {
        build_error_reply(msg_id, API_ERR_BUFFER_REPLY_OVERFLOW);
        return;
    }
    // Ensure we don't have a buffer overflow
    // if ((ptr - reply_buffer + 2) < MAX_BUFFER_SIZE) {
    sprintf(ptr, "}}");
    // }
}

static void build_set_out_reply(char *msg_id) {
    memset(reply_buffer, 0, sizeof(reply_buffer));
    char *ptr = reply_buffer;
    ptr += sprintf(ptr, "{\"_msgid\":\"%s\",\"status\":%d,\"timestamp\":%d,\"payload\":{", msg_id, STATUS[API_SUCCESS_UPDATE].code, (int) time(NULL));
    if (build_payload(&ptr, true, false, false, false, false) < 0) {
        build_error_reply(msg_id, API_ERR_BUFFER_REPLY_OVERFLOW);
        return;
    }
    // Ensure we don't have a buffer overflow
    // if ((ptr - reply_buffer + 2) < MAX_BUFFER_SIZE) {
    sprintf(ptr, "}}");
    // }
}

static int check_indexes_values_array(char *msg_id, json_object *jobj_payload, json_object **jobj_arr_i, json_object **jobj_arr_v) {
    int arr_i_len, arr_v_len;

    if ((*jobj_arr_i = json_get(jobj_payload, "i")) == NULL) {
        build_error_reply(msg_id, API_ERR_INDEX_UNDEFINED);
        return -1;
    }

    if (!json_isType(*jobj_arr_i, JSON_ARRAY)) {
        build_error_reply(msg_id, API_ERR_INDEX_NOT_AN_ARRAY);
        return -1;
    }

    if ((arr_i_len = json_arrayLength(*jobj_arr_i)) < 1) {
        build_error_reply(msg_id, API_ERR_INDEX_NULL);
        return -1;
    }

    if ((*jobj_arr_v = json_get(jobj_payload, "v")) == NULL) {
        build_error_reply(msg_id, API_ERR_VALUE_UNDEFINED);
        return -1;
    }

    if (!json_isType(*jobj_arr_v, JSON_ARRAY)) {
        build_error_reply(msg_id, API_ERR_VALUE_NOT_AN_ARRAY);
        return -1;
    }

    if ((arr_v_len = json_arrayLength(*jobj_arr_v)) < 1) {
        build_error_reply(msg_id, API_ERR_VALUE_NULL);
        return -1;
    }

    if (arr_i_len != arr_v_len) {
        build_error_reply(msg_id, API_ERR_ARR_NOT_EQUAL);
        return -1;
    }

    return arr_i_len;
}

static void exec_set_out(char *msg_id, json_object *jobj_payload) {
    int arr_len, id, value;
    json_object *jobj_arr_i = NULL;
    json_object *jobj_arr_v = NULL;
    json_object *jobj_i = NULL;
    json_object *jobj_v = NULL;

    if ((arr_len = check_indexes_values_array(msg_id, jobj_payload, &jobj_arr_i, &jobj_arr_v)) < 0) {
        return;
    }

    jobj_i = json_getFirst(jobj_arr_i);
    jobj_v = json_getFirst(jobj_arr_v);

    if (jobj_i == NULL) {
        build_error_reply(msg_id, API_ERR_INDEX_NOT_FOUND, 0);
        return;
    }

    if (jobj_v == NULL) {
        build_error_reply(msg_id, API_ERR_VALUE_NOT_FOUND, 0);
        return;
    }

    do {
        if (json_getInteger(jobj_i, &id) < 0) {
            build_error_reply(msg_id, API_ERR_INDEX_NAN, jobj_i->value);
            continue;
        }

        if (json_getInteger(jobj_v, &value) < 0) {
            build_error_reply(msg_id, API_ERR_VALUE_NAN, jobj_v->value);
            continue;
        }

        if (gpio_id_is_available(id) < 0) {
            build_error_reply(msg_id, API_ERR_GPIO_ID_UNDEFINED, id);
            return;
        }

        switch (id) {
            case O_KA1: {
                switch (value) {
                    case 0: {
                        if (gpio_write(O_KA1, IO_OPEN) < 0) {
                            return build_error_reply(msg_id, API_ERR_GPIO_UNABLE_TO_SET_VALUE, "Unable to open O_KA1");
                        }
                        break;
                    }
                    case 1: {
                        if (gpio_write(O_KA1, IO_CLOSE) < 0) {
                            return build_error_reply(msg_id, API_ERR_GPIO_UNABLE_TO_SET_VALUE, "Unable to close O_KA1");
                        }
                        break;
                    }
                    default: {
                        return build_error_reply(msg_id, API_ERR_GPIO_VALUE_OUT_OF_RANGE, "Unknown value to set O_KA1");
                    }
                }
                break;
            }
            case O_KA2: {
                switch (value) {
                    case 0: {
                        if (gpio_write(O_KA2, IO_OPEN) < 0) {
                            return build_error_reply(msg_id, API_ERR_GPIO_UNABLE_TO_SET_VALUE, "Unable to open O_KA2");
                        }
                        break;
                    }
                    case 1: {
                        if (gpio_write(O_KA2, IO_CLOSE) < 0) {
                            return build_error_reply(msg_id, API_ERR_GPIO_UNABLE_TO_SET_VALUE, "Unable to close O_KA2");
                        }
                        break;
                    }
                    default: {
                        return build_error_reply(msg_id, API_ERR_GPIO_VALUE_OUT_OF_RANGE, "Unknown value to set O_KA2");
                    }
                }
                break;
            }

            default: {
                break;
            }
        }
        arr_len--;
    } while ((jobj_i = json_getNext(jobj_i)) != NULL && (jobj_v = json_getNext(jobj_v)) != NULL);

    if (arr_len != 0) {
        log_warn("ERROR: incomplete set/out (arr_len: %d)", arr_len);
    }

    build_set_out_reply(msg_id);
}


static void query_exec(char *msg_id, char *topic, json_object *jobj_payload) {
    // get/all
    if (strcasecmp(topic, "get/all") == 0) {
        build_get_all_reply(msg_id);
        return;
    }
    // set/out
    if (strcasecmp(topic, "set/out") == 0) {
        exec_set_out(msg_id, jobj_payload);
        return;
    }
    // Unknown topic
    build_error_reply(msg_id, API_ERR_TOPIC_UNKNOWN);
}

static int write_to_client(void) {
    int rc = 0;
    size_t msg_len = strlen(reply_buffer);
    if (write(client_socket, reply_buffer, msg_len) != msg_len) {
        if (errno == EPIPE) {
            log_debug("Catch error EPIPE");
            rc = 1;
            goto __error;
        } else {
            log_fatal("Error write #%d: %s", errno, strerror(errno));
            rc = -1;
            goto __error;
        }
    }
#if API_TRACE_ENABLED
    log_trace("Message send to the client: %s", reply_buffer);
#endif
    __error:
    return rc;
}

static int api_server_create(void) {
    int rc = 0, on = 1;
    struct sockaddr_in server_addr;

    // Create an endpoint for communication
    if ((server_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        log_fatal("Unable to open the API Server socket: %s", strerror(errno));
        rc = -1;
        goto __error;
    }

    // Allow address to be reused instantly
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    // Allow to keep alive the connection
    setsockopt(server_socket, SOL_SOCKET, SO_KEEPALIVE, &on, sizeof(on));

    // Set socket to be nonblocking
    /*
    if (ioctl(*server_socket, FIONBIO, (char *) &on) < 0) {
        log_fatal("Unable to set the API Server socket to be nonblocking: %s\n", strerror(errno));
        rc = -1;
        goto __error;
    }
    */

    // Assign IP, PORT
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(API_SOCKET_PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // Bind the newly created socket
    if (bind(server_socket, (const struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        log_fatal("Unable to bind the API Server socket: %s", strerror(errno));
        rc = -1;
        goto __error;
    }

    log_info("API Server thread is running on port %d", API_SOCKET_PORT);

    __error:
    return rc;
}

static void *api_server_thread(void __attribute__((unused)) *p) {
    fd_set set;
    int max_fd_set;
    int rc;
    struct timeval timeout;
    json_object *jobj_parsed = NULL;
    json_object *jobj_msg_id = NULL;
    json_object *jobj_topic = NULL;
    json_object *jobj_payload = NULL;
    char *msg_id = NULL;
    char *topic = NULL;

    // Allow the thread to be canceled
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);

    // Disable SIGPIPE
    signal(SIGPIPE, SIG_IGN);

    if (api_server_create() < 0) {
        goto __error;
    }

    // Listen for connections on a socket
    if (listen(server_socket, 1) < 0) { // only one client
        log_fatal("Unable to listen the API Server socket: %s", strerror(errno));
        goto __error;
    }

    while (server_is_running) {
#if API_TRACE_ENABLED
        log_trace("Waiting for a new client");
#endif
        // Accept a connection on a socket : wait a new client
        if ((client_socket = accept(server_socket, NULL, NULL)) < 0) {
            log_fatal("Unable to accept a client: %s", strerror(errno));
            goto __error;
        }
#if API_TRACE_ENABLED
        log_trace("New client incoming connection: %d", client_socket);
#endif
        while (server_is_running) {
            // Initialize fd_set
            FD_ZERO(&set);
            FD_SET(server_socket, &set);
            FD_SET(client_socket, &set);
            max_fd_set = MAX(server_socket, client_socket) + 1;

            // Initialize timeout
            timeout.tv_sec = API_SOCKET_TIMEOUT_SEC;
            timeout.tv_usec = API_SOCKET_TIMEOUT_US;

            // Initialize read_buffer
            memset(&reply_buffer, 0, sizeof(reply_buffer));

            rc = select(max_fd_set, &set, NULL, NULL, &timeout);

            // Error
            if (rc < 0) {
                log_fatal("Select failed: %s", strerror(errno));
                goto __error;
            }

            // Timeout
            if (rc == 0) {
                build_timeout_reply();

                if (strlen(reply_buffer) > 0) {
                    // Send a response to the client
                    rc = write_to_client();
                    // Error
                    if (rc < 0) goto __error; // Maybe it's too brute...
                    // Client is disconnected => break;
                    if (rc > 0) break;
                }

                // Go back to the `select`
                continue;
            }

            // There is an event: get the message from client
            ssize_t msg_len = read(client_socket, read_buffer, sizeof(read_buffer) - 1);
            if (msg_len < 0) {
                log_fatal("Unable to read a message from the client socket: %s", strerror(errno));
                goto __close;
            }

            // Client is disconnected
            if (msg_len == 0) break;

            // Check the minimal length
            if (msg_len < 2) {
                log_warn("Client socket read error: incomplete chain");
                continue;
            }

            // To be sure to have a finished chain
            read_buffer[msg_len] = 0;

#if API_TRACE_ENABLED
            log_trace("Message received from the client: %s", read_buffer);
#endif

            if ((jobj_parsed = json_parse(read_buffer, (int) msg_len, parsed_stack, API_PARSED_STACK_SIZE)) == NULL) {
                build_error_reply(msg_id, API_ERR_JSON_SYNTAX);
                log_warn("Buffer: %s\nJSON Syntax Error", read_buffer);
                goto __reply;
            }

            if ((jobj_msg_id = json_get(jobj_parsed, "_msgid")) == NULL) {
                build_error_reply(msg_id, API_ERR_MSG_ID_UNDEFINED);
                log_warn("Bad request: unable to get `_msgid` field");
                goto __reply;
            }

            if (json_getString(jobj_msg_id, &msg_id) < 0) {
                build_error_reply(msg_id, API_ERR_MSG_ID_NULL);
                log_warn("Bad request: unable to get `_msgid` string");
                goto __reply;
            }

            if ((jobj_topic = json_get(jobj_parsed, "topic")) == NULL) {
                build_error_reply(msg_id, API_ERR_TOPIC_UNDEFINED);
                log_warn("Bad request: unable to get `topic` field");
                goto __reply;
            }

            if (json_getString(jobj_topic, &topic) < 0) {
                build_error_reply(msg_id, API_ERR_TOPIC_NOT_A_STRING);
                log_warn("Bad request: unable to get `topic` string");
                goto __reply;
            }

            if (topic == NULL || strlen(topic) == 0) {
                build_error_reply(msg_id, API_ERR_TOPIC_NULL);
                log_warn("Bad request: `topic` string is empty");
                goto __reply;
            }

            if ((jobj_payload = json_get(jobj_parsed, "payload")) == NULL) {
                build_error_reply(msg_id, API_ERR_PAYLOAD_UNDEFINED);
                log_warn("Bad request: unable to get `payload` field");
                goto __reply;
            }

            query_exec(msg_id, topic, jobj_payload);

            __reply:
            // Send a response to the client
            rc = write_to_client();
            // Error
            if (rc < 0) goto __error; // Maybe it's too brute...
            // Client is disconnected => break;
            if (rc > 0) break;
        }

        __close:
        // Client is disconnected
#if API_TRACE_ENABLED
        log_trace("Client is now disconnected");
#endif
        if (close(client_socket) < 0) {
            log_fatal("Unable to close the client socket: %s", strerror(errno));
            goto __error;
        }
    }

    __error:
    if (client_socket != -1) {
        if (close(client_socket) < 0) {
            log_fatal("Unable to close the client socket: %s", strerror(errno));
        }
        client_socket = -1;
    }

    if (server_socket != -1) {
        if (close(server_socket) < 0) {
            log_fatal("Unable to close the API Server socket: %s", strerror(errno));
        }
        server_socket = -1;
    }

    log_fatal("End of the API Server thread");
    kill(getppid(), SIGTERM);

    return NULL;
}

// ------------------------------------------------------------------------------------------

int api_server_start(void) {
    int rc = 0;
    log_info("Start API Server socket");
    if ((rc = pthread_create(&thread_id, NULL, api_server_thread, NULL)) != 0) {
        rc = -1;
        log_fatal("Unable to create the API Server socket thread: %s", strerror(errno));
        goto __error;
    }

    __error:
    return rc;
}

int api_server_stop(void) {
    log_info("Stop API Server socket");
    server_is_running = false;
    if (thread_id != -1) {
        pthread_cancel(thread_id);
        if (client_socket != -1) {
            if (shutdown(client_socket, SHUT_RDWR) < 0) {
                log_fatal("Unable to close the client socket: %s", strerror(errno));
            }
        }
        if (server_socket != -1) {
            if (shutdown(server_socket, SHUT_RDWR) < 0) {
                log_fatal("Unable to close the server socket: %s", strerror(errno));
            }

        }
        pthread_join(thread_id, NULL);
    }
    log_fatal("API Server socket thread is stopped");
    return 0;
}