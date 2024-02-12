
#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <string.h>
#include "keyValue.h"
#include "global.h"

using namespace std;

void printObject(keyValue* kv, int8_t _t);

void printDebug(char* _txt)
{
    if(debug)
        printf("\n%s",_txt);
}
void printDebug(const char* _txt, const char* _txt2)
{
    if(debug)
    {
        printf("\n%s",_txt);
        printf("%s",_txt2);
    }
}
void indent(int8_t _t)
{
    for(int8_t i=0;i<_t;i++)
            printf("\t");
}
void printValueList(valueList* list)
{
    while(list != nullptr)
    {
        if(list->t_type == t_string)
        {
           // printf(" value = %s",(char*)list->value);
        }
        else
        {
            if(list->t_type == t_Object)
            {
                keyValue* kv = (keyValue*)list->value;
                valueList* v2 = (valueList*)kv->value;
                printf("\n key = %s",kv->key);
                if(v2->t_type == t_string)
                {
                    printf(" value = %s",(char*)v2->value);
                }
            }
        }
        list = list->next;
    }
}
void printValue(keyValue* kv, int8_t _t)
{
    valueList* list = (valueList*)kv->value;
    while(list != nullptr)
    {
        if(list->value == nullptr)
        {
            printf("\n value = null");
            indent(_t);
        }
        else
        {
            switch (list->t_type)
            {
                case t_string:
                {
                    printf("\t value=%s",(char*)list->value);
                    break;
                }
                case t_Object:
                {
                    printDebug((char*)"\n next value found in object");
                    printObject((keyValue*)list->value,_t);
                    break;
                }
            }
        }
        if(list->next != nullptr
        && list->value != nullptr)
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
        printf("\n %s \t",list->key);
        indent(_t);
        printf("%s:",list->key);
        printValue(list,_t);
        list = list->next;
    }
}
