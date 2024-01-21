#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <string.h>
#include "keyValue.h"
#include "print.h"
#include "tokenizer.h"

using namespace std;

keyValue* getKeyValues(ifstream& in);

keyValue* head = nullptr;  //first element
keyValue* tail = nullptr;  //tail of list (the tail grows with each addition)
/******************************************************
 * Link ValueList
 ******************************************************/
void linkValueList(valueList* newValue, valueList* tail)
{
    valueList* temp = tail;
    tail = newValue;
    temp->next = newValue;
}
/******************************************************
 * Get List
 ******************************************************/
valueList* getList(ifstream& in)
{

    if(result->lastChar != openList)
    {
        printf("\nFailed to find openList");
        return nullptr;
    }

    valueList* newValue = new valueList;

    //what kind of list is this?
    result = getToken(in); 
    newValue->t_type = t_string;
    newValue->value = result->token;

    valueList* head = newValue;
    valueList* tail = head;

    //if(result->lastChar == openObject)
    //    head = getObject(in);

    while(true)
    {
        result = getToken(in);

        newValue = new valueList;
        newValue->t_type = t_string;
        newValue->value = result->token;

        valueList* temp = tail;
        tail = newValue;
        temp->next = newValue;

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
    keyValue* kv = new keyValue();
     
    result = getToken(in);              //key

    kv->key = result->token;

    if(result->lastChar != colon)
        printf("\n colon expected");

    result = getToken(in);              //value

    if(result->lastChar == openList)
    {
        kv->value = getList(in);
        result = getToken(in);              //value
        return kv;
    }

    valueList* vl = new valueList{};
    if(result->lastChar == openObject)
    {
        vl->value = (void*)getKeyValues(in);
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
    return head;
}
/******************************************************
 * Get Object
 ******************************************************/
keyValue* getObject(ifstream& in)
{ 
    
   /* //is this an embedded object
    if(result->lastChar == closeObject)
        result = getToken(in);

    //is this a list member
    if(result->lastChar == comma)
        result = getToken(in);
    */

    if(result->lastChar != openObject)
    {
        printf("\nExpecting open object");
        return nullptr;
    }

    keyValue* kv = getKeyValues(in);
    
    //result = getToken(in);

    return kv;
}
/******************************************************
 * main
 ******************************************************/
int main()
{
    const char* fileName = "test1.json";
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