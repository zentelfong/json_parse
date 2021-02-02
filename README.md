# JSON PARSE

json parse 是一个sax风格json解析器。

**特点**

1. C语言实现
2. 0内存分配
3. 0字符串拷贝
4. sax接口风格
5. 高性能
6. 不用生成dom结构，占用内存低



**使用接口**

```c
bool json_parse(const char* str, json_parse_callback callback, void* ud);
```

