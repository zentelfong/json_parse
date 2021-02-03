#pragma once
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef int (*json_write_cb)(const char *s, size_t len, void* buffer);

typedef struct json_writer
{
	int		err;
	int		flags;
	json_write_cb write;
	void* ud;
}json_writer;

void json_write_init(json_writer * writer, const json_write_cb write,void* ud);

int json_write_object_begin(json_writer * writer);
int json_write_object_end(json_writer * writer);
int json_write_array_begin(json_writer * writer);
int json_write_array_end(json_writer * writer);


int json_write_name(json_writer * writer, const char *const name);
int json_write_name_len(json_writer * writer, const char *const name,
					  const size_t len);
int json_write_string(json_writer * writer, const char *const value);
int json_write_string_len(json_writer * writer, const char *const value,
						const size_t len);
int json_write_uint(json_writer * writer, const unsigned value);
int json_write_int(json_writer * writer, const int value);


#ifdef __cplusplus
} // extern "C"
#endif
