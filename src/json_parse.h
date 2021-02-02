#pragma once

struct JsonKey {
	const char* key;
	int key_len;
	int idx;
};

enum JsonValueType{
	JSON_NULL,
	JSON_BOOL,
	JSON_NUMBER,
	JSON_STRING,
	JSON_ARRAY_BEGIN,
	JSON_ARRAY_END,
	JSON_OBJECT_BEGIN,
	JSON_OBJECT_END
};

//json��ֵ
struct JsonValue {
	JsonValueType type;
	bool bool_value;
	int  int_value;
	double double_value;
	const char* string_value;
	int         string_len;
};

typedef bool (*json_parse_callback)(JsonKey* key, JsonValue* value, void* ud);

//����json�ַ���
bool json_parse2(const char* str, const char **return_parse_end, json_parse_callback callback,void* ud);

bool json_parse(const char* str, json_parse_callback callback, void* ud);

//ȥ��json�ַ���ת��
bool json_unescape(const char* str,size_t len, char* buffer);
