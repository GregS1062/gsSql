#pragma once
#include <string>
#include <string.h>
#include <stdio.h>
#include "global.h"

using namespace std;

class tokenParser
{
    public:

    tokenParser(const char*);

    char*       getToken();
    const char* parseString;
    signed long parseStringLength;
};
tokenParser::tokenParser(const char* _parseString)
{
    parseString         = _parseString;
    parseStringLength   = (signed long)strlen(parseString);
    pos = 0;
}
/******************************************************
 * Get Token
 ******************************************************/
char* tokenParser::getToken()
{
    bool betweenQuotes = false;
    char token[MAXSQLTOKENSIZE];            //token buffer space            
    char c = ' ';                           //character in question
    int  t = 0;                             //token character pointer
    token[0] = '\0';
    char* retToken;

    //read string loop
    while(pos <= parseStringLength)
    {
        
        c = parseString[pos];
        pos++;
        
        if(c == QUOTE)
        {
            if(betweenQuotes)
            {
                betweenQuotes = false;
            }
            else
            {
                betweenQuotes = true; 
            }
            continue;
        }
        
        if( betweenQuotes
        &&  c == SPACE)
        {
            token[t] = c;
            t++;
            continue;
        }

        if(betweenQuotes
        && 
        ( c == NEWLINE
        || c == RETURN
        || c == FORMFEED
        || c == TAB
        || c == VTAB))
        {
            continue;
        }

        if(c == COMMA
        || c == SPACE
        || c == NEWLINE
        || c == RETURN
        || c == FORMFEED
        || c == TAB
        || c == VTAB)
        {
            if(t > 0)
            {
                
                token[t] = '\0';
                //printf("\n %s %d", token,t);
                retToken = (char*)malloc(t+1);
                strcpy(retToken,token);
                //printf("\n token = %s t = %d, len = %d",retToken, t, strlen(token));
                return retToken;
            }

            continue;
        }

        if ( c == OPENPAREN      //Note difference between OPENPAREN and sqlTokenOpenParen
        ||   c == CLOSEPAREN
        ||   c == EQUAL)
        {
            if(t > 0)
            {
                pos = pos -1;
                token[t] = '\0';
                retToken = (char*)malloc(t+1);
                strcpy(retToken,token);
                ////printf("\n token = %s t = %d, len = %d",retToken, t, strlen(token));
                return retToken;
            }
            else
            {
                token[0] = c;
                token[1] = '\0';
                t = 1;
                retToken = (char*)malloc(t+1);
                strcpy(retToken,token);
                ////printf("\n token = %s t = %d, len = %d",retToken, t, strlen(token));
                return retToken;
            }
         }
        
        token[t] = c;
        t++;
    }
    if(t > 0)
    {
        token[t] = '\0';
        retToken = (char*)malloc(t+1);
        strcpy(retToken,token);
        ////printf("\n token = %s t = %d, len = %d",retToken, t, strlen(token));
        return retToken;
    }
    return nullptr;
}
