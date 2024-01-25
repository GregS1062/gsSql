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
   Parse Database Definition
 ****************************************************************/
keyValue* parseDatabaseDefinition(keyValue* qKV, const char* _key, const char* _target)
{
    valueList* list = (valueList*)qKV->value;
    printf("\nsearch on %s ",qKV->key);
    while(list != nullptr)
    {
        keyValue* kv = (keyValue*)list->value;
        printf("\n\tsearching %s ",kv->key);
        if(kv->key != nullptr)
        {
            if (strcmp(kv->key, _key) == 0)
            {
                valueList* v = (valueList*)kv->value;
                if(v->t_type == t_string)
                {
                    printf("\nvalue=%s",(char*)v->value);
                    if (strcmp((char*)v->value, _target) == 0)
                    {
                        printf("\nfound");
                        return (keyValue*)kv->next;
                    }
                }
                if(v->t_type == t_Object)
                {
                    keyValue* nkv = (keyValue*)v->value;
                    printf("\nobject key=%s",nkv->key);
                    if (strcmp(nkv->key, _target) == 0)
                    {
                        printf("\nfound");
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
   Query
 ****************************************************************/
void query(keyValue* dbDef)
{
    string queryString; 
    char* lit_node = (char*)lit_database;
    while(true)
    {
        printf("\nEnter ");
        printf("%s ",lit_node);
        printf("name:");
        getline(cin,queryString);
        keyValue* result = parseDatabaseDefinition(dbDef,lit_node,queryString.c_str());
        if(result != nullptr)
        {
            dbDef = result;
            if(lit_node == lit_database)
            {
                lit_node = (char*)lit_table;
            }
            else
            {
                if(lit_node == lit_table)
                    lit_node = (char*)lit_column;
            }
        }
        if(queryString.compare("exit") == 0)
            return;
    }
}