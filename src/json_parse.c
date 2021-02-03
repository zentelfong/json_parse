#include "json_parse.h"
#include <string.h>
#include <stdlib.h>
#include <assert.h>

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
				*ep = ptr;
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

	while ((*end_ptr != '\"') && *end_ptr && ++len) {
		if (*end_ptr++ == '\\') {
			if (*end_ptr == '\0') {
				/* prevent buffer overflow when last input character is a backslash */
				*ep = end_ptr;
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


static double pow10(int n) {
	static const double e[] = { // 1e-0...1e308: 309 * 8 bytes = 2472 bytes
		1e+0,
		1e+1,  1e+2,  1e+3,  1e+4,  1e+5,  1e+6,  1e+7,  1e+8,  1e+9,  1e+10, 1e+11, 1e+12, 1e+13, 1e+14, 1e+15, 1e+16, 1e+17, 1e+18, 1e+19, 1e+20,
		1e+21, 1e+22, 1e+23, 1e+24, 1e+25, 1e+26, 1e+27, 1e+28, 1e+29, 1e+30, 1e+31, 1e+32, 1e+33, 1e+34, 1e+35, 1e+36, 1e+37, 1e+38, 1e+39, 1e+40,
		1e+41, 1e+42, 1e+43, 1e+44, 1e+45, 1e+46, 1e+47, 1e+48, 1e+49, 1e+50, 1e+51, 1e+52, 1e+53, 1e+54, 1e+55, 1e+56, 1e+57, 1e+58, 1e+59, 1e+60,
		1e+61, 1e+62, 1e+63, 1e+64, 1e+65, 1e+66, 1e+67, 1e+68, 1e+69, 1e+70, 1e+71, 1e+72, 1e+73, 1e+74, 1e+75, 1e+76, 1e+77, 1e+78, 1e+79, 1e+80,
		1e+81, 1e+82, 1e+83, 1e+84, 1e+85, 1e+86, 1e+87, 1e+88, 1e+89, 1e+90, 1e+91, 1e+92, 1e+93, 1e+94, 1e+95, 1e+96, 1e+97, 1e+98, 1e+99, 1e+100,
		1e+101,1e+102,1e+103,1e+104,1e+105,1e+106,1e+107,1e+108,1e+109,1e+110,1e+111,1e+112,1e+113,1e+114,1e+115,1e+116,1e+117,1e+118,1e+119,1e+120,
		1e+121,1e+122,1e+123,1e+124,1e+125,1e+126,1e+127,1e+128,1e+129,1e+130,1e+131,1e+132,1e+133,1e+134,1e+135,1e+136,1e+137,1e+138,1e+139,1e+140,
		1e+141,1e+142,1e+143,1e+144,1e+145,1e+146,1e+147,1e+148,1e+149,1e+150,1e+151,1e+152,1e+153,1e+154,1e+155,1e+156,1e+157,1e+158,1e+159,1e+160,
		1e+161,1e+162,1e+163,1e+164,1e+165,1e+166,1e+167,1e+168,1e+169,1e+170,1e+171,1e+172,1e+173,1e+174,1e+175,1e+176,1e+177,1e+178,1e+179,1e+180,
		1e+181,1e+182,1e+183,1e+184,1e+185,1e+186,1e+187,1e+188,1e+189,1e+190,1e+191,1e+192,1e+193,1e+194,1e+195,1e+196,1e+197,1e+198,1e+199,1e+200,
		1e+201,1e+202,1e+203,1e+204,1e+205,1e+206,1e+207,1e+208,1e+209,1e+210,1e+211,1e+212,1e+213,1e+214,1e+215,1e+216,1e+217,1e+218,1e+219,1e+220,
		1e+221,1e+222,1e+223,1e+224,1e+225,1e+226,1e+227,1e+228,1e+229,1e+230,1e+231,1e+232,1e+233,1e+234,1e+235,1e+236,1e+237,1e+238,1e+239,1e+240,
		1e+241,1e+242,1e+243,1e+244,1e+245,1e+246,1e+247,1e+248,1e+249,1e+250,1e+251,1e+252,1e+253,1e+254,1e+255,1e+256,1e+257,1e+258,1e+259,1e+260,
		1e+261,1e+262,1e+263,1e+264,1e+265,1e+266,1e+267,1e+268,1e+269,1e+270,1e+271,1e+272,1e+273,1e+274,1e+275,1e+276,1e+277,1e+278,1e+279,1e+280,
		1e+281,1e+282,1e+283,1e+284,1e+285,1e+286,1e+287,1e+288,1e+289,1e+290,1e+291,1e+292,1e+293,1e+294,1e+295,1e+296,1e+297,1e+298,1e+299,1e+300,
		1e+301,1e+302,1e+303,1e+304,1e+305,1e+306,1e+307,1e+308
	};

	if (n >= 0) {
		return e[n];
	} else {
		return 1.0/e[-n];
	}
}

static const char *parse_number(const char *num, JsonValue *item)
{
	double n = 0;
	double sign = 1;
	int scale = 0;
	int subscale = 0;
	int signsubscale = 1;

	/* Has sign? */
	if (*num == '-') {
		sign = -1;
		num++;
	}
	/* is zero */
	if (*num == '0') {
		num++;
	}
	/* Number? */
	if ((*num >= '1') && (*num <= '9')) {
		do {
			n = (n * 10.0) + (*num++ - '0');
		} while ((*num >= '0') && (*num <= '9'));
	}
	/* Fractional part? */
	if ((*num == '.') && (num[1] >= '0') && (num[1] <= '9')) {
		num++;
		do {
			n = (n  *10.0) + (*num++ - '0');
			scale--;
		} while ((*num >= '0') && (*num <= '9'));
	}
	/* Exponent? */
	if ((*num == 'e') || (*num == 'E')) {
		num++;
		/* With sign? */
		if (*num == '+') {
			num++;
		}
		else if (*num == '-') {
			signsubscale = -1;
			num++;
		}
		/* Number? */
		while ((*num >= '0') && (*num <= '9')) {
			subscale = (subscale * 10) + (*num++ - '0');
		}
	}

	/* number = +/- number.fraction * 10^+/- exponent */
	n = sign * n * pow10(scale + subscale * signsubscale);

	item->double_value = n;
	item->int_value = (int)n;
	item->type = JSON_NUMBER;
	return num;
}

const char* parse_value(const char* value, const char **ep, 
	JsonKey* key,json_parse_callback callback, void* ud) {

	JsonValue item;
	item.type = JSON_NULL;
	item.string_value = NULL;
	item.string_len = 0;

	switch (*value) {
	case 'n':
		if (!strncmp(value, "null", 4)){
			if (callback(key, &item, ud)) {
				return value + 4;
			} else {
				*ep = value;
				return NULL;
			}
		}
		break;
	case 'f':
		if (!strncmp(value, "false", 5)) {
			item.type = JSON_BOOL;
			item.bool_value = false;
			if (callback(key, &item, ud)) {
				return value + 5;
			} else {
				*ep = value;
				return NULL;
			}
		}
		break;
	case 't':
		if (!strncmp(value, "true", 4)) {
			item.type = JSON_BOOL;
			item.bool_value = true;
			if (callback(key, &item, ud)){
				return value + 4;
			} else {
				*ep = value;
				return NULL;
			}
		}
		break;
	case '\"':
		{
			value = parse_string(value, ep, &item);
			if (value && callback(key, &item, ud)) {
				return value;
			} else {
				if (value) *ep = value;
				return NULL;
			}
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
	case '9':
	case '.':
	{
		value = parse_number(value, &item);
		if (value && callback(key, &item, ud)) {
			return value;
		} else {
			if (value) *ep = value;
			return NULL;
		}
	}
	case '[':
	{
		item.type = JSON_ARRAY_BEGIN;
		if (!callback(key, &item, ud))
			return NULL;
		value = parse_array(value, ep, callback, ud);
		item.type = JSON_ARRAY_END;
		if (value && callback(key, &item, ud)) {
			return value;
		} else {
			if (value) *ep = value;
			return NULL;
		}
	}
	case '{':
	{
		item.type = JSON_OBJECT_BEGIN;
		if (!callback(key, &item, ud))
			return NULL;
		value = parse_object(value, ep, callback, ud);
		item.type = JSON_OBJECT_END;
		if (value && callback(key, &item, ud)) {
			return value;
		} else {
			if (value) *ep = value;
			return NULL;
		}
	}
	default:
		break;
	}
	*ep = value;
	return NULL;
}

static const char* parse_array(const char* value, const char **ep, json_parse_callback callback, void* ud) {
	JsonKey key;

	if (*value != '[') {
		/* not an array! */
		*ep = value;
		return NULL;
	}
	
	value = skip(value + 1);
	if (*value == ']') {
		return value + 1;
	}
	
	key.key = NULL;
	key.idx = 0;

	value = skip(parse_value(skip(value), ep,&key,callback,ud));
	if (!value) {
		return NULL;
	}

	/* loop through the comma separated array elements */
	while (*value == ',') {
		/* go to the next comma */
		++key.idx;
		value = skip(parse_value(skip(value + 1), ep, &key, callback, ud));
		if (!value) {
			/* memory fail */
			return NULL;
		}
	}

	if (*value == ']') {
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
