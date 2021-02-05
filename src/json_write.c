#include "json_write.h"
#include <memory.h>
#include <stdlib.h>
#include <string.h>

extern int   dtoa(double d, char dest[24]);
extern char* u32toa(uint32_t value, char* buffer);
extern char* i32toa(int32_t value, char* buffer);
extern char* u64toa(uint64_t value, char* buffer);
extern char* i64toa(int64_t value, char* buffer);

enum _FLAGS {
	_FLAG_PUT_COMMA = 1 << 0,
	_FLAG_NAME_PUTS_COMMA = 1 << 1,
};

static const char _escapees[0xff] = {
	['\0'] = '0',   /* Zero */
	['\b'] = 'b',  /* Backspace (ascii code 08) */
	['\f'] = 'f',  /* Form feed (ascii code 0C) */
	['\n'] = 'n',  /* New line */
	['\r'] = 'r',  /* Carriage return */
	['\t'] = 't',  /* Tab */
	['\"'] = '\"', /* Double quote */
	['\\'] = '\\', /* Backslash caracter */
};

static inline const char *_escapee(const char* b, const char* e) {
	for(const char *p = b; e != p; ++p) {
		if (0 != _escapees[(unsigned char)*p]) {
			return p;
		}
	}
	return e;
}

static inline int _put(json_writer * writer,
				const char* s,size_t len) {
	if (0 == writer->err) {
		writer->err = writer->write(s, len, writer->ud);
	}
	return writer->err;
}

static int _put_escaped(json_writer * writer,
						const char* s, size_t len)
{
	const char *b = s;
	const char *const e = b + len;
	const char *p;
	char esc[2];
	while(0 == writer->err)
	{
		p = _escapee(b, e);
		if (b < p)
		{
			_put(writer, b, p - b);
		}
		if (p == e)
		{
			break;
		}
		esc[0] = '\\';
		esc[1] = _escapees[(unsigned char)*p++];
		_put(writer, esc, 2);
		b = p;
	}
	return writer->err;
}

static inline int _put_quoted_string(json_writer * writer,
					 const char* value,size_t len) {
	_put(writer, "\"", 1);
	_put_escaped(writer, value, len);
	return _put(writer, "\"", 1);
}

static inline int _put_quoted_escaped(json_writer* writer,
	const char* value, size_t len) {
	_put(writer, "\"", 1);
	_put(writer, value, len);
	return _put(writer, "\"", 1);
}


static inline int _put_name_comma(json_writer * writer) {
	if (_FLAG_PUT_COMMA & writer->flags) {
		_put(writer, ",", 1);
	}
	return writer->err;
}

static inline int _put_value_comma(json_writer * writer) {
	if (_FLAG_PUT_COMMA & writer->flags && !(_FLAG_NAME_PUTS_COMMA & writer->flags)) {
		_put(writer, ",", 1);
	}
	return writer->err;
}

void json_write_init(json_writer* writer, json_write_cb write, void* ud) {
	writer->err = 0;
	writer->flags = 0;
	writer->ud = ud;
	writer->write = write;
}

int json_write_object_begin(json_writer * writer) {
	_put_value_comma(writer);
	writer->flags = 0;
	return _put(writer, "{", 1);
}

int json_write_object_end(json_writer * writer) {
	writer->flags = _FLAG_PUT_COMMA;
	return _put(writer, "}", 1);
}

int json_write_array_begin(json_writer * writer) {
	_put_value_comma(writer);
	writer->flags = 0;
	return _put(writer, "[", 1);
}

int json_write_array_end(json_writer * writer) {
	writer->flags = _FLAG_PUT_COMMA;
	return _put(writer, "]", 1);
}

int json_write_name(json_writer * writer, const char* name) {
	return json_write_name_len(writer, name, strlen(name));
}

int json_write_name_len(json_writer * writer, const char* name,
					  size_t len) {
	_put_name_comma(writer);
	writer->flags |= _FLAG_PUT_COMMA | _FLAG_NAME_PUTS_COMMA;
	_put_quoted_string(writer, name, len);
	return _put(writer, ":", 1);
}

int json_write_string(json_writer * writer, const char* value) {
	return	json_write_string_len(writer, value, strlen(value));
}

int json_write_string_len(json_writer * writer, const char* value,
						size_t len) {
	_put_value_comma(writer);
	writer->flags |= _FLAG_PUT_COMMA;
	return _put_quoted_string(writer, value, len);
}

int json_write_escaped(json_writer* writer, const char* value) {
	return json_write_escaped_len(writer, value, strlen(value));
}

int json_write_escaped_len(json_writer* writer, const char* value,
	size_t len) {
	_put_value_comma(writer);
	writer->flags |= _FLAG_PUT_COMMA;
	return _put_quoted_escaped(writer, value, len);
}


int json_write_uint32(json_writer * writer, uint32_t value) {
	_put_value_comma(writer);
	writer->flags |= _FLAG_PUT_COMMA;
	char str[32];
	char* e = u32toa(value, str);
	return _put(writer, str, e - str);
}

int json_write_int32(json_writer * writer, int32_t value) {
	_put_value_comma(writer);
	writer->flags |= _FLAG_PUT_COMMA;
	char str[32];
	char* e = i32toa(value, str);
	return _put(writer, str, e - str);
}

int json_write_uint64(json_writer * writer, uint64_t value) {
	_put_value_comma(writer);
	writer->flags |= _FLAG_PUT_COMMA;
	char str[32];
	char* e = u64toa(value, str);
	return _put(writer, str, e - str);
}

int json_write_int64(json_writer * writer, int64_t value) {
	_put_value_comma(writer);
	writer->flags |= _FLAG_PUT_COMMA;
	char str[32];
	char* e = i64toa(value, str);
	return _put(writer, str, e - str);
}

int json_write_double(json_writer * writer, double value) {
	_put_value_comma(writer);
	writer->flags |= _FLAG_PUT_COMMA;
	char str[32];
	int len = dtoa(value, str);
	return _put(writer, str, len);
}

int json_write_null(json_writer* writer) {
	_put_value_comma(writer);
	writer->flags |= _FLAG_PUT_COMMA;
	return _put(writer, "null", 4);
}

int json_write_bool(json_writer* writer, bool value) {
	_put_value_comma(writer);
	writer->flags |= _FLAG_PUT_COMMA;
	if(value)
		return _put(writer, "true", 4);
	else
		return _put(writer, "false", 5);
}

int json_write_key(json_writer* writer, const JsonKey* key) {
	if (key->key) {
		_put_name_comma(writer);
		writer->flags |= _FLAG_PUT_COMMA | _FLAG_NAME_PUTS_COMMA;
		_put_quoted_escaped(writer, key->key, key->key_len);
		return _put(writer, ":", 1);
	} else {
		return 0;
	}
}

int json_write_value(json_writer* writer, const JsonValue* value) {

	switch (value->type) {
	case JSON_NULL:
		return json_write_null(writer);
	case JSON_BOOL:
		return json_write_bool(writer, value->bool_value);
	case JSON_NUMBER:
		return json_write_double(writer, value->double_value);
	case JSON_STRING:
		return json_write_escaped_len(writer, value->string_value, value->string_len);
	case JSON_ARRAY_BEGIN:
		return json_write_array_begin(writer);
	case JSON_ARRAY_END:
		return json_write_array_end(writer);
	case JSON_OBJECT_BEGIN:
		return json_write_object_begin(writer);
	case JSON_OBJECT_END:
		return json_write_object_end(writer);
	default:
		return 0;
	}
}

int json_write(json_writer* writer, const JsonKey* key, const JsonValue* value) {
	int rslt = 0;
	switch (value->type) {
	case JSON_NULL:
		rslt = json_write_key(writer, key);
		return rslt == 0?json_write_null(writer):rslt;
	case JSON_BOOL:
		rslt = json_write_key(writer, key);
		return rslt == 0 ? json_write_bool(writer, value->bool_value):rslt;
	case JSON_NUMBER:
		rslt = json_write_key(writer, key);
		return rslt == 0 ? json_write_double(writer, value->double_value):rslt;
	case JSON_STRING:
		rslt = json_write_key(writer, key);
		return rslt == 0 ? json_write_escaped_len(writer, value->string_value, value->string_len):rslt;
	case JSON_ARRAY_BEGIN:
		rslt = json_write_key(writer, key);
		return rslt == 0 ? json_write_array_begin(writer):rslt;
	case JSON_OBJECT_BEGIN:
		rslt = json_write_key(writer, key);
		return rslt == 0 ? json_write_object_begin(writer) : rslt;
	case JSON_ARRAY_END:
		return json_write_array_end(writer);
	case JSON_OBJECT_END:
		return json_write_object_end(writer);
	default:
		return 0;
	}
}
