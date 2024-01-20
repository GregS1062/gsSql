#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <string.h>
#include "keyValue.h"
#include "print.h"
#include "tokenizer.h"

using namespace std;

keyValue* getList(ifstream& in);


keyValue* head = nullptr;  //first element
keyValue* tail = nullptr;  //tail of list (the tail grows with each addition)

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

    kv->value = result->token;

    return kv;
}
/******************************************************
 * Get Object
 ******************************************************/
keyValue* getObject(ifstream& in)
{ 
    result = getToken(in);
    
    //is this an embedded object
    if(result->lastChar == closeObject)
        result = getToken(in);

    //is this a list member
    if(result->lastChar == comma)
        result = getToken(in);
        
    if(result->lastChar != openObject)
    {
        printf("\nExpecting open object");
        return nullptr;
    }

    keyValue* kv = getKeyValue(in);
    
    if(result->lastChar == comma)
        kv->value = getList(in);
    
    result = getToken(in);


    return kv;
}
/******************************************************
 * Get List
 ******************************************************/
keyValue* getList(ifstream& in)
{
    keyValue* dbList = new keyValue();
    result = getToken(in);  //key 
    dbList->key = result->token;

    result = getToken(in);  

    if(result->lastChar != openList)
    {
        printf("\nFailed to find openList");
        return nullptr;
    }
    keyValue* head = new keyValue;
    head = getObject(in);

    keyValue* tail = new keyValue;
    tail = head;
    dbList->value = head;
    while(true)
    {
        keyValue* newKV = getObject(in);

        keyValue* temp = tail;
        tail = newKV;
        temp->next = newKV;

        if(result->lastChar == closeList)
            return dbList;
    }
    return dbList;
}
/******************************************************
 * main
 ******************************************************/
int main()
{
    const char* fileName = "testDB.json";
    ifstream in (fileName);
    if(!in.is_open())
    {
        printf("\nUnable to open file ");
        printf("%s",fileName); 
        printf("\n");
        return 0;
    }

    //get the root object key
    result = getToken(in);
    if(result->lastChar != openObject)
    {
        printf("\nDatabase not found");
        return 0;
    }

    keyValue* kv = getKeyValue(in);

    in.close();

    print(kv,-1);
    printf("\n\n");
}