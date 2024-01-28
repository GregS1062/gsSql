#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>
#include "keyValue.h"
using namespace std;

#define lit_database            "database"
#define lit_table               "table"
#define lit_column              "column"
#define lit_type                "type"
string column;
/****************************************************************
   Get List Node
 ****************************************************************/
keyValue* parseListNode(keyValue* qKV, const char* _key, const char* _target)
{
    valueList* list = (valueList*)qKV->value;
    printDebug("\nsearch on ",qKV->key);
    while(list != nullptr)
    {
        keyValue* kv = (keyValue*)list->value;
        printDebug("\n\tsearching ",kv->key);
        if(kv->key != nullptr)
        {
            if (strcmp(kv->key, _key) == 0)
            {
                valueList* v = (valueList*)kv->value;
                if(v->t_type == t_string)
                {
                    printDebug("\nvalue=",(char*)v->value);
                    if (strcmp((char*)v->value, _target) == 0)
                    {
                        printf("\nfound %s", _target);
                        return (keyValue*)kv->next;
                    }
                }
                if(v->t_type == t_Object)
                {
                    keyValue* nkv = (keyValue*)v->value;
                    printDebug("\nobject key=",nkv->key);
                    if (strcmp(nkv->key, _target) == 0)
                    {
                        printDebug("\nfound",_target);
                        return nkv;
                    }
                }
            }  
        }
        
        list = list->next;
    }
    return nullptr;
}
/****************************************************************
   Get Node
 ****************************************************************/
keyValue* getNode(keyValue* _node, const char* _key)
{
    while(_node != nullptr)
    {
        printDebug("\ngetNode key = ",_node->key);
        if(_node->key != nullptr)
        {
        if(strcmp(_node->key,_key) == 0)
            return _node;
        }
        _node = _node->next;
    }
    return nullptr;
}
keyValue* getDatabaseEntity(keyValue* _dbDef, const char* _list, const char* _member)
{
    //get database list node
    string list = _list;
    list.append("s");            //adding an 's' for the list key
    keyValue* result = getNode(_dbDef,list.c_str());
    if(result == nullptr)
    {
        printDebug("\ncannot find ", list.data());
        return nullptr;
    }
    return parseListNode(result,_list,_member);
}
/****************************************************************
   Query
 ****************************************************************/
void query(keyValue* _dbDef)
{
    string queryString; 
    char* lit_node = (char*)lit_database;
    string key = lit_node;
    key.append("s");
    keyValue* dbDef = getNode(_dbDef,key.c_str());
    if(dbDef == nullptr)
    {
        printDebug("\ncannot find ",key.data());
        return;
    }
    while(true)
    {
        printf("\nEnter ");
        printf("%s ",lit_node);
        printf("name:");
        getline(cin,queryString);
        keyValue* result = parseListNode(dbDef,lit_node,queryString.c_str());
        if(result != nullptr)
        {
            printDebug("\n result key=",result->key);
            if(strcmp(lit_node, lit_database) == 0)
            {
                lit_node = (char*)lit_table;
            }
            else
            {
                if(strcmp(lit_node, lit_table) == 0)
                    lit_node = (char*)lit_column;
            }
            key.clear();
            key.append(lit_node);
            key.append("s");
            dbDef = getNode(result,key.c_str());
            printDebug("\n result key=",result->key);
            if(dbDef == nullptr)
            {
                printDebug("\nfailed to find ",key.data());
                return;
            }
        }
        if(queryString.compare("exit") == 0)
            return;
    }
}