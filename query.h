#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <string.h>
#include "keyValue.h"
using namespace std;

string darabase;
string table;
string column;

bool parseDatabase(keyValue* dbDef, const char* _queryString)
{
    valueList* list = (valueList*)dbDef->value;
    while(list != nullptr)
    {
        keyValue* kv = (keyValue*)list->value;
        if(kv->key != nullptr)
        {
            printf("\nkey = %s",kv->key); 
            if(list->t_type == t_string)
            {          
                printf("\nvalue = %s",(char*)kv->value);
            }
            else
            {
                keyValue* k = (keyValue*)kv->value;
                printf("\nkey = %s",k->key);
                printf("\nvalue = %s",(char*)k->value);
            }
           /*if (strcmp(kv->key,"""database""") == 0)
           {
            printf("\nkey = %s",kv->key);
                printf("\nvalue = %s",(char*)kv->value);
           } 
            
            if(list->t_type == t_string)
            {
                printf("\nvalue = %s",(char*)kv->value);
                if (strcmp((char*)kv->value, _queryString) == 0)
                {
                    printf("\nfound");
                    return true;
                }
            }*/
        }
 
        list = list->next;
    }
    printf("\nnot found");
    return false;
}
void query(keyValue* dbDef)
{

    string queryString;
    while(true)
    {
        printf("\nEnter database name: ");
        getline(cin,queryString);
        parseDatabase(dbDef,queryString.c_str());
        if(queryString.compare("exit") == 0)
            return;
    }
}