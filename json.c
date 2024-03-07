#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include <stdlib.h>
#include <assert.h>
#include <string.h>

#define MAX_NUMBER_LENGTH 32
#define MAX_DEPTH 256
#define MAX_BUFFER 1024

typedef enum json_error_e {
    JSON_ERROR_NONE = 0,
    JSON_ERROR_INVALID_CHAR,
    JSON_ERROR_INVALID_UNICODE,
    JSON_ERROR_INVALID_NUMBER,
    JSON_ERROR_UNEXPECTED_END,
    JSON_ERROR_TRAILING_GARBAGE
} json_error_e;

typedef enum json_type_e {
    JSON_NULL,
    JSON_ARRAY,
    JSON_OBJECT,
    JSON_INTEGER,
    JSON_DOUBLE,
    JSON_STRING,
    JSON_TRUE,
    JSON_FALSE
} json_type_e;

typedef enum json_mode_e {
    MODE_VALUE,   // Parsing a value.
    MODE_NEXT,    // Waiting for the next value in sequence.
    MODE_KEY,    // Parsing a key
    MODE_STRING, // Parsing a string.
    MODE_NUMBER, // Parsing a number.,
    MODE_COLON, // Waiting for the next member.
    MODE_TRUE,    // literal true
    MODE_FALSE,  // literal false
    MODE_NULL,   // literal null
    MODE_UTF8 // \u in a string.
} json_mode_e;

struct json_t;
typedef struct json_t {
    json_type_e type;
    union {
        char* string;
        struct json_t* array;
        struct json_t* object;
        double number;
        long long integer;
    };
    uint32_t length;
    uint32_t capacity;
} json_t;

static void cjson_free(json_t* json) {
    switch (json->type) {
        case JSON_ARRAY:
            for (int i = 0; i < json->length; ++i)
                cjson_free(&json->array[i]);
            free(json->array);
        break;
        case JSON_OBJECT:
            for (int i = 0; i < json->length; ++i) {
                cjson_free(&json->object[i*2 + 0]);
                cjson_free(&json->object[i*2 + 1]);
            }
            free(json->object);
        break;
        case JSON_STRING:
            free(json->string);
        break;
    }
}

static int cjson_encode(lua_State* L) {
    return 1;
}

static void json_append(json_t* array, json_t* value) {
    assert(array->type == JSON_ARRAY);
    if (array->length <= array->capacity) {
        if (array->capacity == 0) {
            array->capacity = 2;
            array->array = malloc(array->capacity*sizeof(json_t));
        } else {
            array->capacity *= 2;
            array->array = realloc(array->array, array->capacity*sizeof(json_t));
        }
    }
    array->array[array->length++] = *value;
}

static void json_str_append(json_t* string, const char* data, size_t len) {
    assert(string->type == JSON_STRING);
    if (string->length <= string->capacity) {
        if (string->capacity == 0) {
            string->capacity = 16;
            string->string = malloc(sizeof(char)*16);
        } else {
            string->capacity *= 2;
            string->string = realloc(string->string, string->capacity*sizeof(char));
        }
    }
    memcpy(&string->string[string->length], data, len);
    string->length += len;
}

static int bsearch_compare(const void* key, const void* item) {
    return strncmp(key, ((json_t*)item)->string, ((json_t*)item)->length);
}

static json_t* json_get(json_t* object, const char* key) {
    assert(object->type == JSON_OBJECT);
    json_t* item = bsearch(key, object->object, object->length, sizeof(json_t)*2, bsearch_compare);
    if (item)
        return item + 1;
    return NULL;
}

static json_t* json_index(json_t* object, long long index) {
    assert(object->type == JSON_ARRAY);
    if (index >= 0 && index < object->length)
        return &object->array[index];
    return NULL;
}

static void json_set(json_t* object, json_t* key, json_t* value) {
    assert(object->type == JSON_OBJECT);
    if (object->length <= object->capacity) {
        if (object->capacity == 0) {
            object->capacity = 2;
            object->object = malloc(object->capacity*sizeof(json_t) * 2);
        } else {
            object->capacity *= 2;
            object->object = realloc(object->object, object->capacity*sizeof(json_t) * 2);
        }
    }
    object->object[object->length*2 + 0] = *key;
    object->object[object->length*2 + 1] = *value;
    object->length++;
}

static int key_sort(const void* a, const void* b) {
    return strcmp(((json_t*)a)->string, ((json_t*)b)->string);
}

static void json_finalize(json_t* object) {
    switch (object->type) {
        case JSON_OBJECT: qsort(object->object, object->length, sizeof(json_t)*2, key_sort); break;
    }
}

static int json_parse_number(json_t* json, char* buffer) {
    switch (json->type) {
        case JSON_DOUBLE: json->number = strtod(buffer, NULL); break;
        case JSON_INTEGER: json->integer = atoll(buffer); break;
        default: return 0; break;
    }
    return 1;
}

static const char* utf8_to_codepoint(const char *p, const char* end, unsigned int *dst) {
    const unsigned char *up = (unsigned char*)p;
    unsigned res, n;
    switch (*p & 0xf0) {
        case 0xf0 :  res = *up & 0x07;  n = 3;  break;
        case 0xe0 :  res = *up & 0x0f;  n = 2;  break;
        case 0xd0 :
        case 0xc0 :  res = *up & 0x1f;  n = 1;  break;
        default   :  res = *up;         n = 0;  break;
    }
    while (n--) {
        res = (res << 6) | (*(++up) & 0x3f);
    }
    *dst = res;
    return (const char*)up + 1;
}

static int codepoint_to_utf8(char* str, int codepoint) {
    if (codepoint < 0x7F) {
        str[0] = codepoint;
        return 1;
    } else if (codepoint < 0x7FF) {
        str[0] = (codepoint / 64) + 192;
        str[1] = (codepoint % 64) + 128;
        return 2;
    } else if (codepoint < 0xFFFF) {
        str[0] = (codepoint / 4096) + 224;
        str[1] = ((codepoint % 4096) / 64) + 128;
        str[2] = (codepoint % 64) + 128;
        return 3;
    } else if (codepoint < 0x10FFFF) {
        str[0] = (codepoint / 262114) + 240;
        str[1] = ((codepoint % 262144) / 4096) + 128;
        str[2] = ((codepoint % 4096) / 64) + 128;
        str[3] = (codepoint % 64) + 128;
        return 4;
    }
    return 0;
}

static int cjson_decode(lua_State* L) {
    size_t len;
    const char* str = luaL_checklstring(L, 1, &len);
    const char* end = str + len;
    int codepoint;
    int line = 1;
    int character = 1;
    json_mode_e mode = MODE_VALUE;
    json_error_e err = JSON_ERROR_NONE;

    char buffer[MAX_BUFFER];
    int buffer_length = 0;

    int depth = -1;
    char utf8_buffer[5] = {0};
    int utf8_buffer_length;
    json_t stack[MAX_DEPTH];

    int slash_count = 0;
    int key = 0;

    do {
        const char* next_str = utf8_to_codepoint(str, end, &codepoint);
        if (next_str > end)
          break;
        #ifdef CJSON_DEBUG
            fprintf(stderr, "CODE: %c %d %d %d\n", codepoint, depth, mode, stack[depth].type);
        #endif
        int char_length = next_str - str;
        int newline = 0;
        int terminate = 0;
        switch (mode) {
            case MODE_TRUE: {
                const char characters[] = { 'r', 'u', 'e' };
                switch (buffer_length) {
                    case 1:
                    case 2:
                        if (codepoint != characters[buffer_length++ - 1])
                            err = JSON_ERROR_INVALID_CHAR;
                    break;
                    case 3:
                        if (codepoint != characters[buffer_length++ - 1])
                            err = JSON_ERROR_INVALID_CHAR;
                        else {
                            mode = MODE_NEXT;
                            terminate = 1;
                        }
                    break;
                }
            } break;
            case MODE_FALSE: {
                const char characters[] = { 'a', 'l', 's', 'e' };
                switch (buffer_length) {
                    case 1:
                    case 2:
                    case 3:
                        if (codepoint != characters[buffer_length++ -1])
                            err = JSON_ERROR_INVALID_CHAR;
                    break;
                    case 4:
                        if (codepoint != characters[buffer_length++ -1])
                            err = JSON_ERROR_INVALID_CHAR;
                        else {
                            mode = MODE_NEXT;
                            terminate = 1;
                        }
                    break;
                }
            } break;
            case MODE_NULL: {
                const char characters[] = { 'u', 'l', 'l' };
                switch (buffer_length) {
                    case 1:
                    case 2:
                        if (codepoint != characters[buffer_length++ -1])
                            err = JSON_ERROR_INVALID_CHAR;
                    break;
                    case 3:
                        if (codepoint != characters[buffer_length++ -1])
                            err = JSON_ERROR_INVALID_CHAR;
                        else {
                            mode = MODE_NEXT;
                            terminate = 1;
                        }
                    break;
                }
            } break;
            case MODE_NEXT:
            case MODE_VALUE: {
                buffer_length = 0;
                switch (codepoint) {
                    case '\n': newline = 1; break;
                    case ' ':
                    case '\t':
                    break;
                    case 't': {
                        stack[++depth].type = JSON_TRUE;
                        buffer[buffer_length++] = codepoint;
                        mode = MODE_TRUE;
                    } break;
                    case 'f': {
                        stack[++depth].type = JSON_FALSE;
                        buffer[buffer_length++] = codepoint;
                        mode = MODE_FALSE;
                    } break;
                    case 'n': {
                        stack[++depth].type = JSON_NULL;
                        buffer[buffer_length++] = codepoint;
                        mode = MODE_NULL;
                    } break;
                    case '0':
                    case '1':
                    case '2':
                    case '3':
                    case '4':
                    case '5':
                    case '6':
                    case '7':
                    case '8':
                    case '9':
                    case '-': {
                        stack[++depth].type = JSON_INTEGER;
                        buffer[buffer_length++] = codepoint;
                        mode = MODE_NUMBER;
                    } break;
                    case '"': {
                        json_t* str = &stack[++depth];
                        str->type = JSON_STRING;
                        str->capacity = 0;
                        str->length = 0;
                        mode = key ? MODE_KEY : MODE_STRING;
                    } break;
                    case '[': {
                        json_t* array = &stack[++depth];
                        array->type = JSON_ARRAY;
                        array->length = 0;
                        array->capacity = 0;
                    } break;
                    case '{': {
                        json_t* obj = &stack[++depth];
                        obj->type = JSON_OBJECT;
                        obj->length = 0;
                        obj->capacity = 0;
                        key = 1;
                    } break;
                    case ',': {
                        if (mode != MODE_NEXT)
                            err = JSON_ERROR_INVALID_CHAR;
                        mode = MODE_VALUE;
                    } break;
                    case ']': {
                        if (depth >= 0 && stack[depth].type != JSON_ARRAY)
                            err = JSON_ERROR_INVALID_CHAR;
                        if (mode == MODE_NEXT)
                            terminate = 1;
                        else
                            depth--;
                    } break;
                    case '}': {
                        if (depth >= 0 && stack[depth].type != JSON_OBJECT)
                            err = JSON_ERROR_INVALID_CHAR;
                        if (mode == MODE_NEXT)
                            terminate = 1;
                        else
                            depth--;
                    } break;
                    default:
                        err = JSON_ERROR_INVALID_CHAR;
                }
            } break;
            case MODE_UTF8: {
                switch (codepoint) {
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
                        utf8_buffer[utf8_buffer_length++] = codepoint;
                        if (utf8_buffer_length == 4) {
                            int utf8_codepoint;
                            if (sscanf(utf8_buffer, "%x", &utf8_codepoint) == 1) {
                                int length = codepoint_to_utf8(&buffer[buffer_length], utf8_codepoint);
                                if (length > 0) {
                                    buffer_length += length;
                                    mode = MODE_STRING;
                                } else
                                    err = JSON_ERROR_INVALID_UNICODE;
                            } else
                                err = JSON_ERROR_INVALID_CHAR;

                        }
                    } break;
                    default:
                        err = JSON_ERROR_INVALID_CHAR;
                }
            } break;
            case MODE_KEY:
            case MODE_STRING: {
                switch (codepoint) {
                    case '\\': ++slash_count; break;
                    case '"':
                        if (slash_count % 2 == 0)
                            terminate = 1;
                        slash_count = 0;
                    break;
                    case '\n': err = JSON_ERROR_INVALID_CHAR; break;
                    case '/':
                        if (slash_count % 2 == 0)
                            buffer[buffer_length++] = '/';
                        slash_count = 0;
                    break;
                    case 't':
                        if (slash_count % 2 == 0)
                            buffer[buffer_length++] = '\t';
                        slash_count = 0;
                    break;
                    case 'n':
                        if (slash_count % 2 == 0)
                            buffer[buffer_length++] = '\n';
                        slash_count = 0;
                    break;
                    case 'f':
                        if (slash_count % 2 == 0)
                            buffer[buffer_length++] = '\f';
                        slash_count = 0;
                    break;
                    case 'r':
                        if (slash_count % 2 == 0)
                            buffer[buffer_length++] = '\r';
                        slash_count = 0;
                    break;
                    case 'u':
                        if (slash_count % 2 == 1) {
                            utf8_buffer_length = 0;
                            mode = MODE_UTF8;
                        } else {
                            buffer[buffer_length++] = '\r';
                        }
                    break;
                    default:
                        memcpy(&buffer[buffer_length], str, char_length);
                        buffer_length += char_length;
                    break;
                }
                if (!err && terminate) {
                    json_str_append(&stack[depth], buffer, buffer_length);
                    if (mode == MODE_KEY) {
                        mode = MODE_COLON;
                        terminate = 0;
                        key = 0;
                    } else {
                        mode = MODE_NEXT;
                    }
                }
            } break;
            case MODE_COLON: {
                switch (codepoint) {
                    case '\n': newline = 1; break;
                    case '\t':
                    case ' ':
                    break;
                    case ':':
                        mode = MODE_VALUE;
                    break;
                }
            } break;
            case MODE_NUMBER: {
                switch (codepoint) {
                    case '\n':
                    case ' ':
                    case '\t': {
                        mode = MODE_NEXT;
                    } break;
                    case '0':
                    case '1':
                    case '2':
                    case '3':
                    case '4':
                    case '5':
                    case '6':
                    case '7':
                    case '8':
                    case '9':
                        if (buffer_length > MAX_BUFFER)
                            return luaL_error(L, "number too long");
                        buffer[buffer_length++] = codepoint;
                    break;
                    case '.':
                        if (stack[depth].type == JSON_DOUBLE)
                            return luaL_error(L, "invalid character");
                        stack[depth].type = JSON_DOUBLE;
                        buffer[buffer_length++] = codepoint;
                    break;
                    case ',':
                        mode = MODE_VALUE;
                    break;
                    case ']':
                        mode = MODE_NEXT;
                        if (stack[depth - 1].type != JSON_ARRAY)
                            err = JSON_ERROR_INVALID_CHAR;
                        ++terminate;
                    break;
                    case '}':
                        mode = MODE_NEXT;
                        if (stack[depth - 1].type != JSON_OBJECT)
                            err = JSON_ERROR_INVALID_CHAR;
                        ++terminate;
                    break;
                    default:  err = JSON_ERROR_INVALID_CHAR; break;
                }
                if (mode == MODE_NUMBER && err == JSON_ERROR_NONE && buffer_length > MAX_NUMBER_LENGTH)
                    err = JSON_ERROR_INVALID_NUMBER;
                else if (mode != MODE_NUMBER && err == JSON_ERROR_NONE) {
                     if (stack[depth - 1].type != JSON_ARRAY && stack[depth - 1].type != JSON_OBJECT)
                        err = JSON_ERROR_INVALID_CHAR;
                    else {
                        buffer[buffer_length] = 0;
                        if (json_parse_number(&stack[depth], buffer)) {
                            ++terminate;
                        } else
                            err = JSON_ERROR_INVALID_NUMBER;
                    }
                }
            } break;
        }
        if (err == JSON_ERROR_NONE) {
            for (int i = 0; i < terminate; ++i) {
                json_finalize(&stack[depth]);
                if (depth > 0) {
                    if (depth > 1 && stack[depth - 1].type == JSON_STRING && stack[depth - 2].type == JSON_OBJECT) {
                        json_set(&stack[depth - 2], &stack[depth - 1], &stack[depth]);
                        --depth;
                    } else if (stack[depth - 1].type == JSON_ARRAY) {
                        json_append(&stack[depth - 1], &stack[depth]);
                    } else
                        err = JSON_ERROR_INVALID_CHAR;
                }
                --depth;
            }
        }
        str = next_str;
        if (err != JSON_ERROR_NONE)
            break;
        if (newline) {
            character = 1;
            ++line;
        } else
            ++character;
    } while (depth >= 0);
    if (err == JSON_ERROR_NONE) {
        if (depth >= 0)
            err = JSON_ERROR_UNEXPECTED_END;
        else if (str < end)
            err = JSON_ERROR_TRAILING_GARBAGE;
    }
    if (err != JSON_ERROR_NONE) {
        for (int i = depth - 1; i > 0; --i)
            cjson_free(&stack[i]);
        switch (err) {
            case JSON_ERROR_UNEXPECTED_END:
                return luaL_error(L, "unexepcted end of json string");
            case JSON_ERROR_INVALID_CHAR:
                return luaL_error(L, "invalid character found '%c' on line %d, character %d", codepoint, line, character);
            case JSON_ERROR_TRAILING_GARBAGE:
                return luaL_error(L, "trailing garbage after json string at line %d character %d", line, character);
            case JSON_ERROR_INVALID_UNICODE:
                return luaL_error(L, "invalid unicode codepoint found on line %d, character %d", line, character);
        }
    }
    json_finalize(&stack[0]);
    json_t* result = lua_newuserdata(L, sizeof(json_t));
    *result = stack[0];
    luaL_setmetatable(L, "cjson_object");
    return 1;
}


static void cjson_push(lua_State* L, json_t* json) {
    if (json) {
        switch (json->type) {
            case JSON_ARRAY:
            case JSON_OBJECT: {
                json_t* value = lua_newuserdata(L, sizeof(json_t*));
                luaL_setmetatable(L, "cjson_value");
                *value = *json;
            } break;
            case JSON_NULL: lua_pushnil(L); break;
            case JSON_DOUBLE: lua_pushnumber(L, json->number); break;
            case JSON_INTEGER: lua_pushinteger(L, json->integer); break;
            case JSON_STRING: lua_pushlstring(L, json->string, json->length); break;
        }
    } else
        lua_pushnil(L);
}

static int cjson_object_gc(lua_State* L) {
    cjson_free(lua_touserdata(L, 1));
}

static int cjson_object_index(lua_State* L) {
    json_t* json = lua_touserdata(L, 1);
    switch (json->type) {
        case JSON_OBJECT: cjson_push(L, json_get(json, luaL_checkstring(L, 2))); break;
        case JSON_ARRAY: cjson_push(L, json_index(json, luaL_checkinteger(L, 2) - 1)); break;
        default: return luaL_error(L, "invalid index");
    }
    return 1;
}

static int cjson_object_len(lua_State* L) {
    json_t* json = lua_touserdata(L, 1);
    switch (json->type) {
        case JSON_OBJECT:
        case JSON_ARRAY:
            lua_pushinteger(L, json->length);
        break;
        default:
          return luaL_error(L, "invalid object");
    }
    return 1;
}


static int cjson_object_newindex(lua_State* L) {
    json_t* json = lua_touserdata(L, 1);
    return luaL_error(L, "not implemented");
}


static int cjson_object_pairs(lua_State* L) {
    json_t* json = lua_touserdata(L, 1);
    return luaL_error(L, "not implemented");
}

static const luaL_Reg cjson_value[] = {
    { "__index",     cjson_object_index },
    { "__newindex",  cjson_object_newindex },
    { "__pairs",     cjson_object_pairs },
    { "__len",       cjson_object_len },
    { NULL,          NULL             }
};

static const luaL_Reg cjson_object[] = {
    { "__gc",        cjson_object_gc   },
    { "__index",     cjson_object_index },
    { "__newindex",  cjson_object_newindex },
    { "__pairs",     cjson_object_pairs },
    { "__len",       cjson_object_len },
    { NULL,          NULL             }
};

static const luaL_Reg cjson_lib[] = {
  { "encode",    cjson_encode  },
  { "decode",    cjson_decode  },
  { NULL,        NULL }
};

int luaopen_cjson(lua_State* L) {
    luaL_newmetatable(L, "cjson_object");
    luaL_setfuncs(L, cjson_object, 0);
    luaL_newmetatable(L, "cjson_value");
    luaL_setfuncs(L, cjson_value, 0);
    lua_newtable(L);
    luaL_setfuncs(L, cjson_lib, 0);
    return 1;
}
