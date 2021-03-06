#include "src/json_parse.h"
#include "src/json_write.h"
#include "TimeChecker.h"
#include <stdio.h>
#include <iostream>
#include <string>
#include <assert.h>

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


bool null_parse(JsonKey* key, JsonValue* value, void* ud) {
	return true;
}

bool on_parse(JsonKey* key, JsonValue* value, void* ud) {
	if (key->key) {
		std::string skey(key->key, key->key_len);
		printf("%s => %s\n",skey.c_str(), JsonToString(value).c_str());
	} else {
		printf("%d => %s\n", key->idx, JsonToString(value).c_str());
	}
	return true;
}

#define TEST_COUNT 10

void bechmark() {
	auto data = readFile("../data/citm_catalog.json");

	{
		TimeChecker checker;
		for (int i = 0; i < TEST_COUNT; ++i) {
			bool rslt = json_parse(data.c_str(), null_parse, NULL);
			assert(rslt);
		}
		printf("json_parse cost %d\n", (int)checker.elapsed());
	}

	{
		TimeChecker checker;
		for (int i = 0; i < TEST_COUNT; ++i) {
			char* rslt = strdup(data.c_str());
			free(rslt);
		}
		printf("strdup cost %d\n", (int)checker.elapsed());
	}
}

int write(const char* s, size_t len, void* ud) {
	std::string str(s, len);
	printf("%s", str.c_str());
	return 0;
}

void test_writer() {
	json_writer writer;
	json_write_init(&writer, write, NULL);

	printf("generate js:");

	json_write_object_begin(&writer);

	json_write_name(&writer, "test");
	json_write_string(&writer, "data");

	json_write_name(&writer, "int");
	json_write_int32(&writer, 123456);

	json_write_name(&writer, "double");
	json_write_double(&writer, 0.123456789);
	json_write_object_end(&writer);
	printf("\n");
}


bool parse_write(JsonKey* key, JsonValue* value, void* ud) {
	json_write((json_writer*)ud, key, value);
	return true;
}

int write_string(const char* s, size_t len, void* ud) {
	std::string* str = (std::string*)ud;
	str->append(s, len);
	return 0;
}

const char* TEST_JSON_VALUE = R"(
{
    "num": 1.234567890123,
    "int": 1234567890123456,
	"str": "hello",
	"null": null,
	"true":true,
	"false":false,
	"arr": [1, 2, 3, 4],
    "map": {
        "a": "aaaaaa",
        "b": "bbbbbb"
    }
}
)";

void test_parse_write() {
	std::string strout;
	json_writer writer;
	json_write_init(&writer, write_string, &strout);

	std::string testjs = TEST_JSON_VALUE;
	json_parse(testjs.c_str(), parse_write, &writer);

	printf("outjson:%s\n", strout.c_str());
}



int main() {

	std::string testjs = TEST_JSON_VALUE;
	json_parse(testjs.c_str(), on_parse, NULL);

	std::string testjs2 = "\"hello world\"";
	json_parse(testjs2.c_str(), on_parse, NULL);

	//bechmark();
	test_writer();
	test_parse_write();
	getchar();
	return 0;
}

