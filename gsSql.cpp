#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <string.h>
#include "keyValue.h"
#include "print.h"
#include "tokenizer.h"

using namespace std;

keyValue* getKeyValues(ifstream& in);
keyValue* getObject(ifstream& in);

keyValue* head = nullptr;  //first element
keyValue* tail = nullptr;  //tail of list (the tail grows with each addition)
/******************************************************
 * Get List
 ******************************************************/
valueList* getList(ifstream& in)
{
    printDebug((char*)"getList");
    if(result->lastChar != openList)
    {
        printf("\nFailed to find openList");
        return nullptr;
    }

    valueList* newValue = nullptr;
    valueList* head = nullptr;
    valueList* tail = nullptr;

    while(true)
    {
        result = getToken(in);

        newValue = new valueList;

        if(result->lastChar == openObject)
        {
            newValue->t_type = t_Object;
            newValue->value = (void*)getObject(in);
        }
        else
        {
            newValue->t_type = t_string;
            newValue->value = result->token;
        }

        if(head == nullptr)
        {
            head = newValue;
            tail = head;
        }
        else
        {
            valueList* temp = tail;
            tail = newValue;
            temp->next = newValue;
        }

        if(result->lastChar == closeList)
           return head;
    }
    return head;
}
/******************************************************
 * Get KeyValue
 ******************************************************/
keyValue* getKeyValue(ifstream& in)
{
    printDebug((char*)"getKeyValueBegin");
    keyValue* kv = new keyValue();
     
    result = getToken(in);              //key
    kv->key = result->token;

    if(result->lastChar != colon)
        printf("\n colon expected");

    result = getToken(in);              //value

    if(result->lastChar == openList)
    {
        kv->value = getList(in);
        return kv;
    }

    valueList* vl = new valueList{};
    if(result->lastChar == openObject)
    {
        vl->value = (void*)getObject(in);
        vl->t_type = t_Object;
        kv->value = vl;
        return kv;
    }
    
    vl->value = (void*)result->token;
    vl->t_type = t_string;
    kv->value = vl;

    return kv;
}
/******************************************************
 * Get KeyValues
 ******************************************************/
keyValue* getKeyValues(ifstream& in)
{
    printDebug((char*)"getKeyValuesBegin");
    keyValue* newkv = getKeyValue(in);
    keyValue* head = newkv;
    keyValue* tail = head;
    while(result->lastChar == comma)
    {
        newkv = getKeyValue(in);
        keyValue* temp = tail;
        tail = newkv;
        temp->next = newkv;
    }
    if(result->lastChar == closeObject
    || result->lastChar == closeList)
        result = getToken(in);

    printDebug((char*)"getKeyValuesEnd");
    return head;
}
/******************************************************
 * Get Object
 ******************************************************/
keyValue* getObject(ifstream& in)
{ 
    printDebug((char*)"getObjectBegin");
    if(result->lastChar != openObject)
    {
        printf("\nExpecting open object");
        return nullptr;
    }

    keyValue* kv = getKeyValues(in);
    printDebug((char*)"getObjectEnd");
    return kv;
}
/******************************************************
 * main
 ******************************************************/
int main()
{
    const char* fileName = "test3.json";
    ifstream in (fileName);
    if(!in.is_open())
    {
        printf("\nUnable to open file ");
        printf("%s",fileName); 
        printf("\n");
        return 0;
    }
    result = getToken(in);
        
    keyValue* kv = getObject(in);

    in.close();
    printObject(kv,-1);
    printf("\n\n");
}