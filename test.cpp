#include "src/json_parse.h"
#include "TimeChecker.h"
#include <stdio.h>
#include <iostream>
#include <string>

std::string readFile(const char* filename) {
	FILE* file = fopen(filename, "rb");
	std::string str;
	if (file) {
		fseek(file, 0, SEEK_END);
		auto size = ftell(file);
		str.resize(size);
		fseek(file, 0, SEEK_SET);

		if (fread(&str[0], size, 1, file)) {
			fclose(file);
			return str;
		}
		else {
			fclose(file);
			return "";
		}
	}
	return str;
}

std::string JsonToString(JsonValue* value) {
	switch (value->type) {
	case JSON_NULL:
		return "null";
	case JSON_BOOL:
		return std::to_string(value->bool_value);
	case JSON_NUMBER:
		return std::to_string(value->double_value);
	case JSON_STRING:
		return std::string(value->string_value,value->string_len);
	case JSON_ARRAY_BEGIN:
		return "JSON_ARRAY_BEGIN";
	case JSON_ARRAY_END:
		return "JSON_ARRAY_END";
	case JSON_OBJECT_BEGIN:
		return "JSON_OBJECT_BEGIN";
	case JSON_OBJECT_END:
		return "JSON_OBJECT_END";
	}
}


void null_parse(JsonKey* key, JsonValue* value, void* ud) {
}

void on_parse(JsonKey* key, JsonValue* value, void* ud) {
	if (key->key) {
		std::string skey(key->key, key->key_len);
		printf("%s => %s\n",skey.c_str(), JsonToString(value).c_str());
	} else {
		printf("%d => %s\n", key->idx, JsonToString(value).c_str());
	}
}

void bechmark() {
	auto data = readFile("../data/citm_catalog.json");
	std::string error;
	TimeChecker checker;
	bool rslt = json_parse(data.c_str(), null_parse, NULL);
	printf("json_parse cost %d rslt=%s\n", (int)checker.elapsed(),rslt?"true":"false");
}

int main() {

	std::string testjs = "{\"num\":1.234567890123,\"int\":1234567890123456}";
	json_parse(testjs.c_str(), on_parse, NULL);
	bechmark();

	getchar();
	return 0;
}

