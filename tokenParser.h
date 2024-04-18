#pragma once
#include <string>
#include <string.h>
#include <stdio.h>
#include "sqlCommon.h"

using namespace std;

class tokenParser
{
    bool keepQuotes = false;

    public:
    tokenParser();
    tokenParser(const char*);
    
    void        parse(const char*);
    void        parse(const char*,bool);
    char*       getToken();
    bool        delimiterAhead();
    bool        eof = false;
    const char* parseString;
    signed long parseStringLength;
    signed long pos;
};
tokenParser::tokenParser()
{
    
}
tokenParser::tokenParser(const char* _parseString)
{
    parseString         = _parseString;
    parseStringLength   = (signed long)strlen(parseString);
    pos = 0;
}
void tokenParser::parse(const char* _parseString)
{
    parseString         = _parseString;
    parseStringLength   = (signed long)strlen(parseString);
    pos = 0;
}
void tokenParser::parse(const char* _parseString, bool _keepQuotes)
{
    parseString         = _parseString;
    parseStringLength   = (signed long)strlen(parseString);
    keepQuotes          = _keepQuotes;
    pos                 = 0;
}
bool tokenParser::delimiterAhead()
{
    signed long lookAhead   = pos; // Look ahead position
    char c = ' ';
    while(lookAhead < parseStringLength)
    {
        c = parseString[lookAhead];
        lookAhead++;
        
        if(c == NEWLINE
        || c == RETURN
        || c == FORMFEED
        || c == TAB
        || c == VTAB)
            continue;

        if(c == COMMA)
            return true;

        return false;
    }
    return false;
};
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
        
        if(pos > parseStringLength)
        {
            eof = true;
            break;
        }

        if((c == GTR
        || c == LST)
        && parseString[pos] == EQUAL)
        {
           pos++; 
           retToken = (char*)malloc(3); 
           retToken[0] = c;
           retToken[1] = EQUAL;
           retToken[2] = '\0';
           return retToken;
        }

        if(c == QUOTE)
        {
            if(betweenQuotes)
            {
                betweenQuotes = false;
                if(keepQuotes)
                {
                    token[t] = c;
                    t++;
                }
                // Looking for end of value list without a comma
                // example Update tablename set column = "" where....
                if(!delimiterAhead())
                {
                    token[t] = '\0';
                    retToken = (char*)malloc(t+1);
                    strcpy(retToken,token);
                    return retToken;
                }
            }
            else
            {
                betweenQuotes = true; 
            }
            if(keepQuotes)
            {
                token[t] = c;
                t++;
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
                retToken = (char*)malloc(t+1);
                strcpy(retToken,token);
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
                if(debug)
                   fprintf(traceFile,"\n token = %s",retToken);
                return retToken;
            }
            else
            {
                token[0] = c;
                token[1] = '\0';
                t = 1;
                retToken = (char*)malloc(t+1);
                strcpy(retToken,token);
                if(debug)
                   fprintf(traceFile,"\n token = %s",retToken);
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
        if(debug)
            fprintf(traceFile,"\n token = %s",retToken);
        return retToken;
    }
    eof = true;
    return nullptr;
}
