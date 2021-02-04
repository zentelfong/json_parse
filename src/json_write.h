#pragma once
#include <string.h>
#include <stdint.h>
#include <stdbool.h>

#include "json_parse.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*json_write_cb)(const char *s, size_t len, void* ud);

typedef struct json_writer
{
	int		err;
	int		flags;
	json_write_cb write;
	void* ud;
}json_writer;

void json_write_init(json_writer * writer,json_write_cb write,void* ud);

int json_write_object_begin(json_writer * writer);
int json_write_object_end(json_writer * writer);
int json_write_array_begin(json_writer * writer);
int json_write_array_end(json_writer * writer);

int json_write_name(json_writer * writer, const char* name);
int json_write_name_len(json_writer * writer, const char* name,size_t len);

int json_write_string(json_writer * writer, const char* value);
int json_write_string_len(json_writer * writer, const char* value,size_t len);

int json_write_escaped(json_writer* writer, const char* value);
int json_write_escaped_len(json_writer* writer, const char* value, size_t len);

int json_write_uint32(json_writer * writer, uint32_t value);
int json_write_int32(json_writer * writer, int32_t value);

int json_write_uint64(json_writer * writer, uint64_t value);
int json_write_int64(json_writer * writer, int64_t value);

int json_write_double(json_writer * writer, double value);
int json_write_null(json_writer* writer);
int json_write_bool(json_writer* writer,bool value);

int json_write_key(json_writer* writer, const JsonKey* key);
int json_write_value(json_writer* writer, const JsonValue* value);

int json_write(json_writer* writer, const JsonKey* key, const JsonValue* value);

#ifdef __cplusplus
} // extern "C"
#endif
