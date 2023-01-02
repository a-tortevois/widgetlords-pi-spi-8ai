//
// Created by Alexandre Tortevois

#ifndef H_API_H
#define H_API_H

#define API_TRACE_ENABLED                                                0
#define API_SOCKET_PORT                                               3000
#define API_SOCKET_TIMEOUT_SEC                                           0
#define API_SOCKET_TIMEOUT_US                             (500 * MS_TO_US)
#define API_MAX_BUFFER_SIZE                                       (2*1024) // 2ko
#define API_IV_BUFFER_SIZE                                            1024  // 1ko
#define API_PARSED_STACK_SIZE                                           48 // should be equal: obj_root + _msg_id + topic + payload + arr_i + arr_v
#define API_MSG_ID_LEN                                                  16

typedef enum {
    API_SUCCESS,
    API_SUCCESS_UPDATE,
    API_SUCCESS_WITHOUT_DATA,
    API_ERR_BUFFER_REPLY_OVERFLOW,
    API_ERR_JSON_SYNTAX,
    API_ERR_MSG_ID_UNDEFINED,
    API_ERR_MSG_ID_NULL,
    API_ERR_TOPIC_UNDEFINED,
    API_ERR_TOPIC_NOT_A_STRING,
    API_ERR_TOPIC_NULL,
    API_ERR_TOPIC_UNKNOWN,
    API_ERR_PAYLOAD_UNDEFINED,
    API_ERR_INDEX_UNDEFINED,
    API_ERR_INDEX_NOT_AN_ARRAY,
    API_ERR_INDEX_NULL,
    API_ERR_INDEX_NAN,
    API_ERR_VALUE_UNDEFINED,
    API_ERR_VALUE_NOT_AN_ARRAY,
    API_ERR_VALUE_NULL,
    API_ERR_VALUE_NAN,
    API_ERR_ARR_NOT_EQUAL,
    API_ERR_INDEX_NOT_FOUND,
    API_ERR_VALUE_NOT_FOUND,
    API_ERR_VAR_NOT_FOUND,
    API_ERR_VAR_READ_ONLY,
    API_ERR_VAR_ID_UNDEFINED,
    API_ERR_VALUE_OUT_OF_RANGE,
    API_ERR_TIMEZONE_UPDATE,
    API_ERR_CERAMIC_REG_COMPILE_FAIL,
    API_ERR_CERAMIC_REG_NOMATCH,
    API_ERR_CERAMIC_REG_FAILED,
    API_ERR_CERAMIC_ID_OUT_OF_RANGE,
    API_ERR_CERAMIC_VAR_ID_OUT_OF_RANGE,
    API_ERR_CERAMIC_VAR_ID_IS_READONLY,
    API_ERR_CERAMIC_VALUE_OUT_OF_RANGE,
    API_STATUS_CODE_COUNT,
} api_status_code;

typedef struct {
    int code;
    char *text;
} api_status;

int api_server_start(void);
int api_server_stop(void);


#endif //H_API_H
