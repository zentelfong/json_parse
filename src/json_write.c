#include "json_write.h"
#include <memory.h>
#include <stdlib.h>

enum _FLAGS
{
	_FLAG_PUT_COMMA = 1 << 0,
	_FLAG_NAME_PUTS_COMMA = 1 << 1,
};

static const char _escapees[0xff] =
{
	['\0'] = '0',   /* Zero */
	['\b'] = 'b',  /* Backspace (ascii code 08) */
	['\f'] = 'f',  /* Form feed (ascii code 0C) */
	['\n'] = 'n',  /* New line */
	['\r'] = 'r',  /* Carriage return */
	['\t'] = 't',  /* Tab */
	['\"'] = '\"', /* Double quote */
	['\\'] = '\\', /* Backslash caracter */
};

static const char  *_escapee(const char *const b, const char *const e)
{
	for(const char *p = b; e != p; ++p)
	{
		if (0 != _escapees[(unsigned char)*p])
		{
			return p;
		}
	}
	return e;
}

static char  *_uint_str(char *const e, unsigned value)
{
	char *p = e;
	do
	{
		*--p = '0' + value % 10;
	}
	while (0 != (value /= 10));
	return p;
}

static char  *_int_str(char *const e, int value)
{
	char *p = _uint_str(e, 0 <= value? value: -value);
	if (0 > value)
	{
		*--p = '-';
	}
	return p;
}

static int _put(json_writer * writer,
				const char *const s, const size_t len)
{
	if (0 == writer->err) {
		writer->err = writer->write(s, len, writer->ud);
	}
	return writer->err;
}

static int _put_escaped(json_writer * writer,
						const char *const s, const size_t len)
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

static int _put_quoted(json_writer * writer,
					 const char *const value, const size_t len)
{
	_put(writer, "\"", 1);
	_put_escaped(writer, value, len);
	return _put(writer, "\"", 1);
}

static int _put_name_comma(json_writer * writer)
{
	if (_FLAG_PUT_COMMA & writer->flags)
	{
		_put(writer, ",", 1);
	}
	return writer->err;
}

static int _put_value_comma(json_writer * writer)
{
	if (_FLAG_PUT_COMMA & writer->flags && !(_FLAG_NAME_PUTS_COMMA & writer->flags))
	{
		_put(writer, ",", 1);
	}
	return writer->err;
}

void json_write_init(json_writer* writer, const json_write_cb write, void* ud)
{
	writer->err = 0;
	writer->flags = 0;
	writer->ud = ud;
	writer->write = write;
}

int json_write_object_begin(json_writer * writer)
{
	_put_value_comma(writer);
	writer->flags = 0;
	return _put(writer, "{", 1);
}

int json_write_object_end(json_writer * writer)
{
	writer->flags = _FLAG_PUT_COMMA;
	return _put(writer, "}", 1);
}

int json_write_array_begin(json_writer * writer)
{
	_put_value_comma(writer);
	writer->flags = 0;
	return _put(writer, "[", 1);
}

int json_write_array_end(json_writer * writer)
{
	writer->flags = _FLAG_PUT_COMMA;
	return _put(writer, "]", 1);
}

int json_write_name(json_writer * writer, const char *const name)
{
	return json_write_name_len(writer, name, strlen(name));
}

int json_write_name_len(json_writer * writer, const char *const name,
					  const size_t len)
{
	_put_name_comma(writer);
	writer->flags |= _FLAG_PUT_COMMA | _FLAG_NAME_PUTS_COMMA;
	_put_quoted(writer, name, len);
	return _put(writer, ":", 1);
}

int json_write_string(json_writer * writer, const char *const value)
{
	return	json_write_string_len(writer, value, strlen(value));
}

int json_write_string_len(json_writer * writer, const char *const value,
						const size_t len)
{
	_put_value_comma(writer);
	writer->flags |= _FLAG_PUT_COMMA;
	return _put_quoted(writer, value, len);
}

int json_write_uint(json_writer * writer, const unsigned value)
{
	_put_value_comma(writer);
	writer->flags |= _FLAG_PUT_COMMA;
	char str[16];
	char *const e = str + sizeof(str);
	char *const p = _uint_str(e, value);
	return _put(writer, p, e - p);
}

int json_write_int(json_writer * writer, const int value)
{
	_put_value_comma(writer);
	writer->flags |= _FLAG_PUT_COMMA;
	char str[16];
	char *const e = str + sizeof(str);
	char *const p = _int_str(e, value);
	return _put(writer, p, e - p);
}

