
#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <string.h>
#include "keyValue.h"
#include "tokenizer.h"

using namespace std;

void printObject(keyValue* kv, int8_t _t);

void printDebug(char* _txt)
{
    if(debug)
        printf("\n%s",_txt);
}
void indent(int8_t _t)
{
    for(int8_t i=0;i<_t;i++)
            printf("\t");
}
void printValue(keyValue* kv, int8_t _t)
{
    valueList* list = (valueList*)kv->value;
    while(list != nullptr)
    {
        switch (list->t_type)
        {
            case t_string:
            {
                printf("%s",(char*)list->value);
                break;
            }
            case t_Object:
            {
                printObject((keyValue*)list->value,_t);
                break;
            }
        }
        if(list->next != nullptr)
            printf(",");
        list = list->next;
    }
}
void printObject(keyValue* kv, int8_t _t)
{
    _t++;
    //**************
    // Start printing here
    //*****************
    keyValue* list = kv;
    while(list != nullptr)
    {
        printf("\n");
        indent(_t);
        printf("%s:",list->key);
        printValue(list,_t);
        list = list->next;
    }
}
