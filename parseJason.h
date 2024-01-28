#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <string.h>
#include "keyValue.h"
#include "print.h"
#include "tokenizer.h"
#include "query.h"

using namespace std;

char colon = ':';
char comma = ',';
char quote = '"';
char openObject = '{';
char closeObject = '}';
char openList = '[';
char closeList = ']';
char space = ' ';
char newline = '\n';

keyValue* getKeyValues(ifstream& in);
keyValue* getObject(ifstream& in);

tokenResult* tkResult;
/******************************************************
 * getToken  Read tokens from file
 ******************************************************/
tokenResult* getToken(ifstream& in)
{
    tkResult = new tokenResult();
    char token[50];
    char c = ' ';
   // bool isToken = false;
    bool betweenQuotes = false;
    int i = 0;

    while(in.good())
    {
        in.get(c);
        tkResult->lastChar = c;                //pass last character to caller
        if(debug)
            printf("%c",c);

        if(c == quote)
        {
            if(betweenQuotes)
            {
                betweenQuotes = false;
            }
            else
            {
                betweenQuotes = true; 
              //  isToken = true;  
            }
            continue;
       }

    /*  if(c >= '0'
        && c <= '9')
        {
            isToken = true;
        } */

        if((c == space && !betweenQuotes)  //Elinimate white space but not inside token
        || c == newline)
        {
            continue;
        }

        if(c == openList
        || c == openObject)
        {
            return tkResult;  
        }

        if(c == comma
        || c == colon
        || c == closeObject
        || c == closeList)
        {
            token[i] = '\0';                        //terminate string
            tkResult->token = (char*)malloc(i);       //create new char pointer
            strcpy(tkResult->token,token);            //copy char[] before it is destroyed
            return tkResult;                          //return unique pointer address
        }

        token[i] = c;                               //populate char[]
        i++;                                        //increment char cursor
    }
    return tkResult;                                  //if done, return nul
}

/******************************************************
 * Get List
 ******************************************************/
valueList* getList(ifstream& in)
{
    printDebug((char*)"getList");
    if(tkResult->lastChar != openList)
    {
        printf("\nFailed to find openList");
        return nullptr;
    }

    valueList* newValue = nullptr;
    valueList* head = nullptr;
    valueList* tail = nullptr;

    while(true)
    {
        tkResult = getToken(in);

        newValue = new valueList;

        if(tkResult->lastChar == openObject)
        {
            newValue->t_type = t_Object;
            newValue->value = (void*)getObject(in);
        }
        else
        {
            newValue->t_type = t_string;
            newValue->value = tkResult->token;
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
            printDebug((char*)"value list chain next");
        }

        if(tkResult->lastChar == closeList)
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
     
    tkResult = getToken(in);              //key
    kv->key = tkResult->token;

    if(tkResult->lastChar != colon)
        printf("\n colon expected");

    tkResult = getToken(in);              //value

    if(tkResult->lastChar == openList)
    {
        kv->value = getList(in);
        return kv;
    }

    valueList* vl = new valueList{};
    if(tkResult->lastChar == openObject)
    {
        vl->value = (void*)getObject(in);
        vl->t_type = t_Object;
        kv->value = vl;
        printDebug((char*)"Object embedded in value list");
        return kv;
    }
    
    vl->value = (void*)tkResult->token;
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
    while(tkResult->lastChar == comma)
    {
        newkv = getKeyValue(in);
        keyValue* temp = tail;
        tail = newkv;
        temp->next = newkv;
        printDebug((char*)"keyValue chain next");
    }
    if(tkResult->lastChar == closeObject
    || tkResult->lastChar == closeList)
        tkResult = getToken(in);

    printDebug((char*)"getKeyValuesEnd");
    return head;
}
/******************************************************
 * Get Object
 ******************************************************/
keyValue* getObject(ifstream& in)
{ 
    printDebug((char*)"getObjectBegin");
    if(tkResult->lastChar != openObject)
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
keyValue* parseDatabaseDefinition()
{
    const char* fileName = "dbDef.json";
    ifstream in (fileName);
    if(!in.is_open())
    {
        printf("\nUnable to open file ");
        printf("%s",fileName); 
        printf("\n");
        return nullptr;
    }
    
    getToken(in);
        
    keyValue* db = getObject(in);

    in.close();

    return db;
}