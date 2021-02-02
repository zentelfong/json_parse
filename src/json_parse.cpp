#include "json_parse.h"
#include <string.h>
#include <stdlib.h>
#include <math.h>

static const char* parse_array(const char* value, const char **ep, json_parse_callback callback, void* ud);
static const char* parse_object(const char* value, const char **ep, json_parse_callback callback, void* ud);

static const char *global_ep = NULL;

/* Utility to jump whitespace and cr/lf */
static const char *skip(const char *in){
	while (in && *in && ((unsigned char)*in <= 32)){
		in++;
	}
	return in;
}

static const char *parse_key(const char *str, const char **ep, JsonKey* key) {
	const char *ptr = str + 1;
	int len = 0;
	if (*str != '\"') {
		*ep = str;
		return NULL;
	}

	while ((*ptr != '\"') && *ptr && ++len) {
		if (*ptr++ == '\\') {
			if (*ptr == '\0') {
				/* prevent buffer overflow when last input character is a backslash */
				return NULL;
			}
			/* Skip escaped quotes. */
			ptr++;
		}
	}

	key->idx = 0;
	key->key = str + 1;
	key->key_len = len;

	if (*ptr == '\"') {
		ptr++;
	}
	return ptr;
}


/* Parse the input text into an unescaped cstring, and populate item. */
static const char *parse_string(const char *str, const char **ep, JsonValue *item)
{
	const char *ptr = str + 1;
	const char *end_ptr = str + 1;
	int len = 0;

	/* not a string! */
	if (*str != '\"') {
		*ep = str;
		return NULL;
	}

	while ((*end_ptr != '\"') && *end_ptr && ++len){
		if (*end_ptr++ == '\\'){
			if (*end_ptr == '\0'){
				/* prevent buffer overflow when last input character is a backslash */
				return NULL;
			}
			/* Skip escaped quotes. */
			end_ptr++;
		}
	}
	item->type = JSON_STRING;
	item->string_value = ptr;
	item->string_len = len;

	if (*end_ptr == '\"') {
		end_ptr++;
	}
	return end_ptr;
}

static const char *parse_number(const char *num, JsonValue *item)
{
	double n = 0;
	double sign = 1;
	double scale = 0;
	int subscale = 0;
	int signsubscale = 1;

	/* Has sign? */
	if (*num == '-')
	{
		sign = -1;
		num++;
	}
	/* is zero */
	if (*num == '0')
	{
		num++;
	}
	/* Number? */
	if ((*num >= '1') && (*num <= '9'))
	{
		do
		{
			n = (n * 10.0) + (*num++ - '0');
		} while ((*num >= '0') && (*num <= '9'));
	}
	/* Fractional part? */
	if ((*num == '.') && (num[1] >= '0') && (num[1] <= '9'))
	{
		num++;
		do
		{
			n = (n  *10.0) + (*num++ - '0');
			scale--;
		} while ((*num >= '0') && (*num <= '9'));
	}
	/* Exponent? */
	if ((*num == 'e') || (*num == 'E'))
	{
		num++;
		/* With sign? */
		if (*num == '+')
		{
			num++;
		}
		else if (*num == '-')
		{
			signsubscale = -1;
			num++;
		}
		/* Number? */
		while ((*num >= '0') && (*num <= '9'))
		{
			subscale = (subscale * 10) + (*num++ - '0');
		}
	}

	/* number = +/- number.fraction * 10^+/- exponent */
	n = sign * n * pow(10.0, (scale + subscale * signsubscale));

	item->double_value = n;
	item->int_value = (int)n;
	item->type = JSON_NUMBER;
	return num;
}

const char* parse_value(const char* value, const char **ep, 
	JsonKey* key,json_parse_callback callback, void* ud) {

	JsonValue item;
	item.string_value = NULL;
	item.string_len = 0;

	if (!strncmp(value, "null", 4)) {
		item.type = JSON_NULL;
		callback(key, &item,ud);
		return value + 4;
	}
	if (!strncmp(value, "false", 5)) {
		item.type = JSON_BOOL;
		item.bool_value = false;
		callback(key, &item, ud);
		return value + 5;
	}
	if (!strncmp(value, "true", 4)) {
		item.type = JSON_BOOL;
		item.bool_value = true;
		callback(key, &item, ud);
		return value + 4;
	}
	if (*value == '\"') {
		value = parse_string(value, ep, &item);
		if (value) {
			callback(key, &item, ud);
		}
		return value;
	}
	if ((*value == '-') || ((*value >= '0') && (*value <= '9'))) {
		value = parse_number(value, &item);
		callback(key, &item, ud);
		return value;
	}
	if (*value == '[') {
		item.type = JSON_ARRAY_BEGIN;
		callback(key, &item, ud);
		value = parse_array(value, ep,callback,ud);
		item.type = JSON_ARRAY_END;
		callback(key, &item, ud);
		return value;
	}
	if (*value == '{') {
		item.type = JSON_OBJECT_BEGIN;
		callback(key, &item, ud);
		value = parse_object(value, ep, callback, ud);
		item.type = JSON_OBJECT_END;
		callback(key, &item, ud);
		return value;
	}

	*ep = value;
	return NULL;
}

static const char* parse_array(const char* value, const char **ep, json_parse_callback callback, void* ud) {
	JsonKey key;

	if (*value != '['){
		/* not an array! */
		*ep = value;
		return NULL;
	}
	
	value = skip(value + 1);
	if (*value == ']'){
		return value + 1;
	}
	
	key.key = NULL;
	key.idx = 0;

	value = skip(parse_value(skip(value), ep,&key,callback,ud));
	if (!value){
		return NULL;
	}

	/* loop through the comma separated array elements */
	while (*value == ',') {
		/* go to the next comma */
		++key.idx;
		value = skip(parse_value(skip(value + 1), ep, &key, callback, ud));
		if (!value){
			/* memory fail */
			return NULL;
		}
	}

	if (*value == ']'){
		/* end of array */
		return value + 1;
	}

	/* malformed. */
	*ep = value;
	return NULL;
}

static const char* parse_object(const char* value, const char **ep, json_parse_callback callback, void* ud) {
	JsonKey key;
	if (*value != '{') {
		/* not an object! */
		*ep = value;
		return NULL;
	}

	value = skip(value + 1);
	if (*value == '}')
	{
		/* empty object. */
		return value + 1;
	}

	/* parse first key */
	value = skip(parse_key(skip(value), ep,&key));
	if (!value) {
		return NULL;
	}

	if (*value != ':')
	{
		/* invalid object. */
		*ep = value;
		return NULL;
	}

	/* skip any spacing, get the value. */
	value = skip(parse_value(skip(value + 1), ep,&key,callback,ud));

	if (!value){
		return NULL;
	}

	while (*value == ','){
		value = skip(parse_key(skip(value + 1), ep,&key));
		if (!value){
			return NULL;
		}

		if (*value != ':') {
			/* invalid object. */
			*ep = value;
			return NULL;
		}

		/* skip any spacing, get the value. */
		value = skip(parse_value(skip(value + 1), ep,&key,callback,ud));

		if (!value) {
			return NULL;
		}
	}

	/* end of object */
	if (*value == '}'){
		return value + 1;
	}

	/* malformed */
	*ep = value;
	return NULL;
}


bool json_parse2(const char* str, const char **return_parse_end,json_parse_callback callback, void* ud) {
	const char **ep = return_parse_end ? return_parse_end : &global_ep;
	str = skip(str);
	if (*str == '[') {
		return parse_array(str, ep, callback,ud) != NULL;
	} else if (*str == '{') {
		return parse_object(str, ep, callback, ud) != NULL;
	} else {
		return false;
	}
}

bool json_parse(const char* str, json_parse_callback callback, void* ud) {
	return json_parse2(str, NULL, callback, ud);
}

/* parse 4 digit hexadecimal number */
static unsigned parse_hex4(const char *str) {
	unsigned h = 0;
	/* first digit */
	if ((*str >= '0') && (*str <= '9'))
	{
		h += (*str) - '0';
	}
	else if ((*str >= 'A') && (*str <= 'F'))
	{
		h += 10 + (*str) - 'A';
	}
	else if ((*str >= 'a') && (*str <= 'f'))
	{
		h += 10 + (*str) - 'a';
	}
	else /* invalid */
	{
		return 0;
	}


	/* second digit */
	h = h << 4;
	str++;
	if ((*str >= '0') && (*str <= '9'))
	{
		h += (*str) - '0';
	}
	else if ((*str >= 'A') && (*str <= 'F'))
	{
		h += 10 + (*str) - 'A';
	}
	else if ((*str >= 'a') && (*str <= 'f'))
	{
		h += 10 + (*str) - 'a';
	}
	else /* invalid */
	{
		return 0;
	}

	/* third digit */
	h = h << 4;
	str++;
	if ((*str >= '0') && (*str <= '9'))
	{
		h += (*str) - '0';
	}
	else if ((*str >= 'A') && (*str <= 'F'))
	{
		h += 10 + (*str) - 'A';
	}
	else if ((*str >= 'a') && (*str <= 'f'))
	{
		h += 10 + (*str) - 'a';
	}
	else /* invalid */
	{
		return 0;
	}

	/* fourth digit */
	h = h << 4;
	str++;
	if ((*str >= '0') && (*str <= '9'))
	{
		h += (*str) - '0';
	}
	else if ((*str >= 'A') && (*str <= 'F'))
	{
		h += 10 + (*str) - 'A';
	}
	else if ((*str >= 'a') && (*str <= 'f'))
	{
		h += 10 + (*str) - 'a';
	}
	else /* invalid */
	{
		return 0;
	}

	return h;
}


/* first bytes of UTF8 encoding for a given length in bytes */
static const unsigned char firstByteMark[7] = {
	0x00, /* should never happen */
	0x00, /* 0xxxxxxx */
	0xC0, /* 110xxxxx */
	0xE0, /* 1110xxxx */
	0xF0, /* 11110xxx */
	0xF8,
	0xFC
};



bool json_unescape(const char* str, size_t len, char* buffer) {
	const char *ptr = str;
	const char *end_ptr = ptr + len;
	char *ptr2 = buffer;
	unsigned uc = 0;
	unsigned uc2 = 0;

	/* loop through the string literal */
	while (ptr < end_ptr)
	{
		if (*ptr != '\\') {
			*ptr2++ = *ptr++;
		}
		/* escape sequence */
		else
		{
			ptr++;
			switch (*ptr)
			{
			case 'b':
				*ptr2++ = '\b';
				break;
			case 'f':
				*ptr2++ = '\f';
				break;
			case 'n':
				*ptr2++ = '\n';
				break;
			case 'r':
				*ptr2++ = '\r';
				break;
			case 't':
				*ptr2++ = '\t';
				break;
			case '\"':
			case '\\':
			case '/':
				*ptr2++ = *ptr;
				break;
			case 'u':
				/* transcode utf16 to utf8. See RFC2781 and RFC3629. */
				uc = parse_hex4(ptr + 1); /* get the unicode char. */
				ptr += 4;
				if (ptr >= end_ptr){
					/* invalid */
					return false;
				}
				/* check for invalid. */
				if (((uc >= 0xDC00) && (uc <= 0xDFFF)) || (uc == 0)) {
					return false;
				}

				/* UTF16 surrogate pairs. */
				if ((uc >= 0xD800) && (uc <= 0xDBFF)) {
					if ((ptr + 6) > end_ptr){
						/* invalid */
						return false;
					}
					if ((ptr[1] != '\\') || (ptr[2] != 'u')){
						/* missing second-half of surrogate. */
						return false;
					}
					uc2 = parse_hex4(ptr + 3);
					ptr += 6; /* \uXXXX */
					if ((uc2 < 0xDC00) || (uc2 > 0xDFFF)){
						/* invalid second-half of surrogate. */
						return false;
					}
					/* calculate unicode codepoint from the surrogate pair */
					uc = 0x10000 + (((uc & 0x3FF) << 10) | (uc2 & 0x3FF));
				}

				/* encode as UTF8
				* takes at maximum 4 bytes to encode:
				* 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx */
				len = 4;
				if (uc < 0x80)
				{
					/* normal ascii, encoding 0xxxxxxx */
					len = 1;
				}
				else if (uc < 0x800)
				{
					/* two bytes, encoding 110xxxxx 10xxxxxx */
					len = 2;
				}
				else if (uc < 0x10000)
				{
					/* three bytes, encoding 1110xxxx 10xxxxxx 10xxxxxx */
					len = 3;
				}
				ptr2 += len;

				switch (len) {
				case 4:
					/* 10xxxxxx */
					*--ptr2 = ((uc | 0x80) & 0xBF);
					uc >>= 6;
				case 3:
					/* 10xxxxxx */
					*--ptr2 = ((uc | 0x80) & 0xBF);
					uc >>= 6;
				case 2:
					/* 10xxxxxx */
					*--ptr2 = ((uc | 0x80) & 0xBF);
					uc >>= 6;
				case 1:
					/* depending on the length in bytes this determines the
					* encoding ofthe first UTF8 byte */
					*--ptr2 = (uc | firstByteMark[len]);
				}
				ptr2 += len;
				break;
			default:
				return false;
			}
			ptr++;
		}
	}
	*ptr2 = '\0';
	return true;
}
