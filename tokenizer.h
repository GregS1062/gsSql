#pragma once

#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <string.h>
#include "keyValue.h"

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
tokenResult* result;
bool debug = true;
/******************************************************
 * getToken  Read tokens from file
 ******************************************************/
tokenResult* getToken(ifstream& in)
{
    tokenResult* result = new tokenResult();
    char token[50];
    char c = ' ';
    bool isToken = false;
    bool betweenQuotes = false;
    int i = 0;

    while(in.good())
    {
        in.get(c);
        result->lastChar = c;                //pass last character to caller
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
                isToken = true;  
            }
       }

        if(c >= '0'
        && c <= '9')
        {
            isToken = true;
        }

        if((c == space && !betweenQuotes)  //Elinimate white space but not inside token
        || c == newline)
        {
            continue;
        }

        if(c == openList
        || c == openObject)
        {
            return result;  
        }

        if(c == comma
        || c == colon
        || c == closeObject
        || c == closeList)
        {
            token[i] = '\0';                        //terminate string
            result->token = (char*)malloc(i);       //create new char pointer
            strcpy(result->token,token);            //copy char[] before it is destroyed
            return result;                          //return unique pointer address
        }

        token[i] = c;                               //populate char[]
        i++;                                        //increment char cursor
    }
    return result;                                  //if done, return nul
}
