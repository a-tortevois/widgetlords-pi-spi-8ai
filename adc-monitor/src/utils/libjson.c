// Sources : https://github.com/a-tortevois/libjson

#include "utils/libjson.h"
#include "utils/stdfun.h"

static jason_parser parser;

static void json_init(json_object mem_stack[], int mem_stack_size) {
    for (int i = 0; i < mem_stack_size; i++) {
        mem_stack[i].key = NULL;
        mem_stack[i].type = JSON_UNDEFINED;
        mem_stack[i].parent = NULL;
        mem_stack[i].next = NULL;
        mem_stack[i].value = NULL;
        mem_stack[i].child = NULL;
        mem_stack[i].size = 0;
    }
    parser.pos = 0;
    parser.current = &mem_stack[0];
    parser.parent = NULL;
    parser.next = 1;
}

static int json_alloc_child(json_object mem_stack[], int mem_stack_size) {
    if (parser.next < mem_stack_size) {
        parser.parent = parser.current;
        parser.current = &mem_stack[parser.next++];
        parser.parent->child = parser.current;
        parser.current->parent = parser.parent;
        parser.parent->size++;
    } else {
        return JSON_NOMEM_ERROR;
    }
    return 0;
}

static void json_free_child() {
    parser.next--;
    // parser.current->key = NULL;
    // parser.current->type = JSON_UNDEFINED;
    parser.current->parent = NULL;
    parser.current->next = NULL;
    parser.current->value = NULL;
    parser.current->child = NULL;
    parser.current->size = 0;
    parser.parent->child = NULL;
    parser.parent->size--;
}

static int json_alloc_next(json_object mem_stack[], int mem_stack_size) {
    if (parser.parent->type != JSON_OBJECT && parser.parent->type != JSON_ARRAY) {
        return JSON_SYNTAX_ERROR;
    }
    if (parser.next < mem_stack_size) {
        parser.current->next = &mem_stack[parser.next++];
        parser.current = parser.current->next;
        parser.current->parent = parser.parent;
        parser.parent->size++;
    } else {
        return JSON_NOMEM_ERROR;
    }
    return 0;
}

static int json_parseString(char *str, size_t str_len) {
    str[parser.pos] = '\0';
    char *ptr = &str[++parser.pos];
    char *s = ptr;
    for (; parser.pos < str_len && str[parser.pos] != '\0'; parser.pos++, s++) {
        char c = str[parser.pos];
        *s = c;
        if (c == '\"') { // End of sting
            // int p = &str[parser.pos] - s;
            for (char *i = s; i < &str[parser.pos]; i++) {
                *s = '\0';
            }
            str[parser.pos] = '\0';
            if (parser.parent->type == JSON_OBJECT && parser.current->key == NULL) {
                parser.current->key = ptr;
            } else {
                parser.current->type = JSON_STRING;
                parser.current->value = ptr;
            }
            return 0;
        }
        // Backslash: Quoted symbol expected
        if (c == '\\' && ++parser.pos < str_len && str[parser.pos] != '\0') {
            c = str[parser.pos];
            // Allowed escaped symbols
            switch (c) {
                case '\"': {
                    *s = '\"';
                    break;
                }
                case '/': {
                    *s = '/';
                    break;
                }
                case '\\': {
                    *s = '\\';
                    break;
                }
                case 'b': {
                    *s = '\b';
                    break;
                }
                case 'f': {
                    *s = '\f';
                    break;
                }
                case 'r': {
                    *s = '\r';
                    break;
                }
                case 'n': {
                    *s = '\n';
                    break;
                }
                case 't': {
                    *s = '\t';
                    break;
                }
                case 'u': { // Allows escaped symbol \uXXXX
                    int uc = 0, hex_val = 0, nc = 0;
                    unsigned char dest[4] = {0, 0, 0, 0};
                    for (int i = 0; i < 4 && ++parser.pos < str_len && str[parser.pos] != '\0'; i++) {
                        // If it isn't a hex character we have an error
                        if (atoh(str[parser.pos], &hex_val) < 0) {
                            return JSON_SYNTAX_ERROR;
                        }
                        uc += hex_val << (12 - i * 4);
                    }
                    nc = uc_to_utf8(uc, dest);
                    for (int i = 0; i < nc; i++, s++) {
                        sprintf(s, "%c", dest[i]);
                    }
                    break;
                }
                default: // Unexpected symbol
                    return JSON_SYNTAX_ERROR;
            }

        }
    }
    return JSON_SYNTAX_ERROR;
}

/** Parse int_32: -2 147 483 648 ... 2 147 483 647 */
static int json_parseNumber(char *str, size_t str_len) {
    if (parser.current->key == NULL && parser.parent->type != JSON_ARRAY) {
        return JSON_SYNTAX_ERROR;
    }
    char *ptr = &str[parser.pos];
    if (str[parser.pos] == '-') {
        parser.pos++;
    }
    for (; parser.pos < str_len && str[parser.pos] != '\0'; parser.pos++) {
        char c = str[parser.pos];
        if ((&str[parser.pos] - ptr) > 10) {
            return JSON_SYNTAX_ERROR;
        }
        if (c == ',' || c == ']' || c == '}') {
            parser.pos--;
            parser.current->value = ptr;
            parser.current->type = JSON_INTEGER;
            return 0;
        }
        if (c < 0x30 || c > 0x39) {
            return JSON_SYNTAX_ERROR;
        }
    }
    return JSON_SYNTAX_ERROR;
}

static int json_parsePrimitive(char *str, size_t str_len, const char *search) {
    if (parser.current->key == NULL && parser.parent->type != JSON_ARRAY) {
        return JSON_SYNTAX_ERROR;
    }
    int search_len = (int) strlen(search);
    char *ptr = &str[parser.pos];
    if ((parser.pos + search_len) < str_len) {
        for (int i = 0; i < search_len && parser.pos < str_len && str[parser.pos] != '\0'; i++, parser.pos++) {
            if (search[i] != str[parser.pos]) {
                return JSON_SYNTAX_ERROR;
            }
        }
        parser.pos--;
        parser.current->value = ptr;
        parser.current->type = (strcmp(search, "null") == 0) ? JSON_NULL : JSON_BOOLEAN;
        return 0;
    }
    return JSON_SYNTAX_ERROR;
}

json_object *json_parse(char *str, size_t str_len, json_object mem_stack[], int mem_stack_size) {
    if (str == NULL || str_len < 3 || mem_stack == NULL || mem_stack_size < 2) {
        return NULL;
    }
    if (str[0] != '{' || str[str_len - 1] != '}') {
        return NULL;
    }
    json_init(mem_stack, mem_stack_size);
    for (; parser.pos < str_len && str[parser.pos] != '\0'; parser.pos++) {
        char c = str[parser.pos];
        switch (c) {
            case '{':
            case '[': {
                parser.current->type = (c == '{' ? JSON_OBJECT : JSON_ARRAY);
                if (json_alloc_child(mem_stack, mem_stack_size) < 0) {
                    return NULL;
                }
                str[parser.pos] = '\0';
                break;
            }
            case ':': {
                if (parser.parent->type != JSON_OBJECT || parser.current->key == NULL /*|| parser.current->type != JSON_UNDEFINED*/) {
                    return NULL;
                }
                str[parser.pos] = '\0';
                break;
            }
            case ',': {
                if (json_alloc_next(mem_stack, mem_stack_size) < 0) {
                    return NULL;
                }
                str[parser.pos] = '\0';
                break;
            }
            case '\"': {
                if (json_parseString(str, str_len) < 0) {
                    return NULL;
                }
                break;
            }
            case '-':
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9': {
                if (json_parseNumber(str, str_len) < 0) {
                    return NULL;
                }
                break;
            }
            case 'f': {
                if (json_parsePrimitive(str, str_len, "false") < 0) {
                    return NULL;
                }
                break;
            }
            case 't': {
                if (json_parsePrimitive(str, str_len, "true") < 0) {
                    return NULL;
                }
                break;
            }
            case 'n': {
                if (json_parsePrimitive(str, str_len, "null") < 0) {
                    return NULL;
                }
                break;
            }
            case '}':
            case ']': {
                json_type type = (c == '}' ? JSON_OBJECT : JSON_ARRAY);
                if (parser.current->key == NULL && parser.current->type == JSON_UNDEFINED /*&& parser.current->value == NULL*/) {
                    json_free_child();
                }
                if (parser.parent && parser.parent->type == type) {
                    parser.current = parser.parent;
                    parser.parent = parser.current->parent;
                }
                str[parser.pos] = '\0';
                break;
            }
            case '\t':
            case '\r':
            case '\n':
            case ' ': {
                str[parser.pos] = '\0';
                break;
            }
            default: {
                return NULL;
            }
        }
    }
    return (parser.parent == NULL) ? parser.current : NULL;
}

json_object *json_get(json_object *json_obj, char *key) {
    json_object *this = NULL;
    if (json_obj && json_obj->type == JSON_OBJECT) {
        this = json_obj->child;
        for (int i = 0; i < json_obj->size; i++) {
            if (strcmp(key, this->key) == 0) {
                return this;
            }
            this = this->next;
        }
    }
    return NULL;
}

bool json_isType(json_object *json_obj, json_type type) {
    return (json_obj && json_obj->type == type);
}

int json_getString(json_object *json_obj, char **result) {
    if (json_obj && json_obj->type == JSON_STRING) {
        *result = json_obj->value;
        return 0;
    }
    return -1;
}

int json_getInteger(json_object *json_obj, int *result) {
    if (json_obj && json_obj->type == JSON_INTEGER) {
        return strtoint(json_obj->value, result);
    }
    return -1;
}

int json_getBool(json_object *json_obj, bool *result) {
    if (json_obj && json_obj->type == JSON_BOOLEAN) {
        *result = (strcmp(json_obj->value, "true") == 0) ? true : false;
        return 0;
    }
    return -1;
}

int json_arrayLength(json_object *json_obj) {
    if (json_obj && json_obj->type == JSON_ARRAY) {
        return json_obj->size;
    }
    return -1;
}

json_object *json_arrayGetIndex(json_object *json_obj, int index) {
    json_object *this = NULL;
    if (json_obj && json_obj->type == JSON_ARRAY && index <= json_obj->size) {
        this = json_obj->child;
        for (int i = 0; i < index; i++) {
            this = this->next;
        }
        return this;
    }
    return NULL;
}

json_object *json_getFirst(json_object *json_obj) {
    if (json_obj && (json_obj->type == JSON_OBJECT || json_obj->type == JSON_ARRAY) /* && json_obj->size > 0 */) {
        return json_obj->child;
    }
    return NULL;
}

json_object *json_getNext(json_object *json_obj) {
    if (json_obj && (json_obj->parent->type == JSON_OBJECT || json_obj->parent->type == JSON_ARRAY)) {
        return json_obj->next;
    }
    return NULL;
}