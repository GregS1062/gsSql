#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <string.h>
#include "keyValue.h"
#include "print.h"

using namespace std;

char openObject = '{';
char closeObject = '}';
char openList = '[';
char closeList = ']';


keyValue* getKeyValues(ifstream& in);
keyValue* getObject(ifstream& in);

tokenResult* tkResult;
/*-------------------------------------------------------------
    These functions parses a json text file into a hierachial
    structure consisting of key/values and value lists.

    The purpose is generic and not tied to SQL.
 --------------------------------------------------------------*/
/******************************************************
 * getToken  Read tokens from file
 ******************************************************/
tokenResult* getToken(ifstream& in)
{
    try{
        tkResult = new tokenResult();
        char token[50];
        char c = ' ';
        bool betweenQuotes = false;
        int i = 0;

        while(in.good())
        {
            in.get(c);
            tkResult->lastChar = c;                //pass last character to caller
            if(debug)
                printf("%c",c);

            if(c == QUOTE)
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

            if((c == SPACE && !betweenQuotes)  //Elinimate white SPACE but not inside token
            || c == NEWLINE)
            {
                continue;
            }

            if(c == openList
            || c == openObject)
            {
                return tkResult;  
            }

            if(c == COMMA
            || c == COLON
            || c == closeObject
            || c == closeList)
            {
                token[i] = '\0';                          //terminate string
                tkResult->token = (char*)malloc(i);       //create new char pointer
                strcpy(tkResult->token,token);            //copy char[] before it is destroyed
                return tkResult;                          //return unique pointer address
            }

            token[i] = c;                //populate char[]
            i++;                                        //increment char cursor
        }
        return tkResult;
    }
    catch(const std::exception& e)
    {
        errText.append( e.what());
        return nullptr;
    }                                  
}

/******************************************************
 * Get List
 ******************************************************/
valueList* getList(ifstream& in)
{
    try
    {

    
        if(tkResult->lastChar != openList)
        {
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
            }

            if(tkResult->lastChar == closeList)
            return head;
        }
        return head;
    }
    catch(const std::exception& e)
    {
        errText.append( e.what());
        return nullptr;
    }   
}
/******************************************************
 * Get KeyValue
 ******************************************************/
keyValue* getKeyValue(ifstream& in)
{
    try
    {
        keyValue* kv = new keyValue();
     
        tkResult = getToken(in);              //key
        kv->key = tkResult->token;

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
            return kv;
        }
        
        vl->value = (void*)tkResult->token;
        vl->t_type = t_string;
        kv->value = vl;

        return kv;
    }
    catch(const std::exception& e)
    {
        errText.append( e.what());
        return nullptr;
    }  
}
/******************************************************
 * Get KeyValues
 ******************************************************/
keyValue* getKeyValues(ifstream& in)
{
    try
    {
        keyValue* newkv = getKeyValue(in);
        keyValue* head = newkv;
        keyValue* tail = head;
        while(tkResult->lastChar == COMMA)
        {
            newkv = getKeyValue(in);
            keyValue* temp = tail;
            tail = newkv;
            temp->next = newkv;
        }
        if(tkResult->lastChar == closeObject
        || tkResult->lastChar == closeList)
            tkResult = getToken(in);

        return head;
    }
    catch(const std::exception& e)
    {
        errText.append( e.what());
        return nullptr;
    }
}  
/******************************************************
 * Get Object
 ******************************************************/
keyValue* getObject(ifstream& in)
{ 
    try
    {
        if(tkResult->lastChar != openObject)
        {
            return nullptr;
        }

        keyValue* kv = getKeyValues(in);
        return kv;
    }
    catch(const std::exception& e)
    {
        errText.append( e.what());
        return nullptr;
    }  
}
/****************************************************************
   Target Not Found
 ****************************************************************/
void targetNotFound(string _message, string _target)
{
    try
    {
        errText.append("<p>");
        errText.append(_message);
        errText.append(" ");
        errText.append(_target);
        errText.append(" not found");
    }
    catch(const std::exception& e)
    {
        errText.append( e.what());
    }  
}
/****************************************************************
   Get Node
 ****************************************************************/
keyValue* getNode(keyValue* _node, const char* _key)
{
    try
    {
        while(_node != nullptr)
        {
            if(_node->key != nullptr)
            {
            if(strcmp(_node->key,_key) == 0)
                return _node;
            }
            _node = _node->next;
        }
        return nullptr;
    }
    catch(const std::exception& e)
    {
        errText.append( e.what());
        return nullptr;
    }  
}

/****************************************************************
   Get Node List
 ****************************************************************/
valueList* getNodeList(keyValue* _kv,const char* _key)
{
    /*  Returns the child list from a parent list based on a key*/
    try{
        //Get the child list
        keyValue* result = getNode(_kv,_key);
        if(result == nullptr)
        {
            targetNotFound("",_key);
            return nullptr;
        }
        return result->value;
        }
    catch(const std::exception& e)
    {
        errText.append( e.what());
        return nullptr;
    }  
}
/****************************************************************
   Validate List Member
 ****************************************************************/
bool validateListMember(valueList* _vl, const char* _key)
{
    try
    { 
        while(_vl != nullptr)
        {
            if(_vl->t_type == t_Object)
            {
                keyValue* nkv = (keyValue*)_vl->value;
                valueList* v2 = (valueList*)nkv->value;
                if(v2->t_type == t_string)
                {
                if(strcmp(_key,(char*)v2->value) == 0)
                    return true;
                }
            }
            _vl = _vl->next;
        }  
        return false;
    }
    catch(const std::exception& e)
    {
        errText.append( e.what());
        return false;
    }  
}
/****************************************************************
   Get Member
 ****************************************************************/
keyValue* getMember(valueList* _vl, const char* _key)
{
    try
    {
        while(_vl != nullptr)
        {
            if(_vl->t_type == t_Object)
            {
                keyValue* nkv = (keyValue*)_vl->value;
                valueList* v2 = (valueList*)nkv->value;
                if(v2->t_type == t_string)
                {
                if(strcmp(_key,(char*)v2->value) == 0)
                    return nkv;
                }
            }
            _vl = _vl->next;
        }  
        return nullptr;
    }
    catch(const std::exception& e)
    {
        errText.append( e.what());
        return nullptr;
    }  
}
/****************************************************************
   Get Member
 ****************************************************************/
keyValue* getMember(keyValue* _kv, const char* _key)
{
    try
    {
        while(_kv != nullptr)
        {
            if(strcmp(_kv->key,_key) == 0)
                return _kv;
            _kv = _kv->next;
        }  
        return nullptr;
    }
    catch(const std::exception& e)
    {
        errText.append( e.what());
        return nullptr;
    }  
}
/****************************************************************
   Get Member Value
 ****************************************************************/
char* getMemberValue(keyValue* _kv, const char* _key)
{
    try
    { 
        while(_kv != nullptr)
        {
            if(strcmp(_key,_kv->key) == 0)
            {
                valueList* vl = (valueList*)_kv->value;
                return (char*)vl->value;
            }
            _kv = _kv->next;
        }  
        return nullptr;
    }
    catch(const std::exception& e)
    {
        errText.append( e.what());
        return nullptr;
    }  
}
/******************************************************
 * Parse Database Definition
 ******************************************************/
keyValue* parseJasonDatabaseDefinition(const char* _fileName)
{
    try
    {
        const char* fileName = "dbDef.json";
        ifstream in (_fileName);
        if(!in.is_open())
        {
            errText.append("<p>");
            errText.append("unable to jason db definition file ");
            errText.append(fileName); 
            return nullptr;
        }
        
        getToken(in);
            
        keyValue* db = getObject(in);

        in.close();

        return db;
    }
    catch(const std::exception& e)
    {
        errText.append( e.what());
        return nullptr;
    }   
}