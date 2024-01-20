#pragma once

#include <string>
enum ValueType
{
    t_string,
    t_Object
};
class valueList
{
    public:
    int8_t t_type;
    void* value = nullptr;
    valueList* next = nullptr;
};
class keyValue
{
    public:
        char* key = nullptr;
        valueList* value = nullptr;
        keyValue* next = nullptr;
};
class tokenResult
{
    public:
    char  lastChar;
    char* token = nullptr;
};
