// Sources : https://github.com/a-tortevois/libjson

#ifndef H_LIBJSON_H
#define H_LIBJSON_H

#include <stdbool.h>
#include <string.h>

typedef enum {
    JSON_SYNTAX_ERROR = -1,
    JSON_NOMEM_ERROR = -2,
} json_error;

typedef enum {
    JSON_UNDEFINED,
    JSON_OBJECT,
    JSON_ARRAY,
    JSON_STRING,
    JSON_INTEGER,
    JSON_FLOAT,
    JSON_BOOLEAN,
    JSON_NULL
} json_type;

typedef struct json_object_s {
    char *key;
    json_type type;
    struct json_object_s *parent;
    struct json_object_s *next;
    union {
        char *value;
        struct {
            struct json_object_s *child;
            int size;
        };
    };
} json_object;

typedef struct {
    int pos;                // offset in the JSON string
    json_object *current;   // current index
    json_object *parent;    // parent index
    int next;               // next index available
} jason_parser;

json_object *json_parse(char *str, size_t str_len, json_object mem_stack[], int mem_stack_size);

/** Helpers */
json_object *json_get(json_object *json_obj, char *key);
bool json_isType(json_object *json_obj, json_type type);
int json_getString(json_object *json_obj, char **result);
int json_getInteger(json_object *json_obj, int *result);
int json_getBool(json_object *json_obj, bool *result);
int json_arrayLength(json_object *json_obj);
json_object *json_arrayGetIndex(json_object *json_obj, int index);
json_object *json_getFirst(json_object *json_obj);
json_object *json_getNext(json_object *json_obj);

#endif //H_LIBJSON_H
