#include "src/json_parse.h"
#include "TimeChecker.h"
#include <stdio.h>
#include <iostream>

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


void on_parse(JsonKey* key, JsonValue* value, void* ud) {
	//if (key->key) {
	//	std::string skey(key->key, key->key_len);
	//	printf("%s =>\n",skey.c_str());
	//}
	//else {
	//	printf("%d =>\n", key->idx);
	//}
}

void bechmark() {
	auto data = readFile("../data/citm_catalog.json");
	std::string error;
	TimeChecker checker;
	json_parse(data.c_str(), on_parse, NULL);
	printf("json_parse cost %d\n", checker.elapsed());
}

int main() {

	std::string testjs = "{\"num\":1.234567890123,\"int\":1234567890123456}";
	json_parse(testjs.c_str(), on_parse, NULL);
	bechmark();

	getchar();
	return 0;
}

