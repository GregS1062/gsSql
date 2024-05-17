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
    tokenParser(){}
    tokenParser(const char*);
    
    void        parse(const char*);
    void        parse(const char*,bool);
    char*       getToken();
    char*       cleanString(char*);
    bool        delimiterAhead();
    bool        eof = false;
    const char* parseString;
    signed int parseStringLength;
    signed int pos;
};

char* tokenParser::cleanString(char* _string)
{
    // ABC  DEF
    size_t  len = strlen(_string);
    char*   newString = dupString(_string);
    char    c;
    size_t  posNewString = 0;
    bool    betweenQuotes = false;
    bool    priorSpace = false;

    for(size_t posString = 0; posString<len; posString++)
    {
        c =_string[posString];

        if(c == NEWLINE
        || c == RETURN
        || c == FORMFEED
        || c == TAB
        || c == VTAB)  
        {
            c =  SPACE;
        }
            
        
        if(c == QUOTE)
        {
            if(betweenQuotes)
                betweenQuotes = false;
            else
                betweenQuotes = true;
        }


        if(c == EQUAL
        && (_string[posString-1] != GTR
        &&  _string[posString-1] != LST)
        )
        {
            if(_string[posString-1] != SPACE)
            {
                newString[posNewString] = SPACE;
                posNewString++;
            }
            newString[posNewString] = c;
            posNewString++;
            if(_string[posString+1] != SPACE)
            {
                newString[posNewString] = SPACE;
                posNewString++;
                priorSpace = false;
            }
            continue;
        }
        
        if(!betweenQuotes
        && c == SPACE)
        {
            if(priorSpace)
                continue;
            else
                priorSpace = true;
        }

        if(c != SPACE)
            priorSpace = false;

        newString[posNewString] = c;
        posNewString++;
    }
    newString[posNewString] = '\0';
    return newString;
}
tokenParser::tokenParser(const char* _parseString)
{
    parseString         = _parseString;
    parseStringLength   = (signed int)strlen(parseString);
    pos = 0;
}
void tokenParser::parse(const char* _parseString)
{
    parseString         = _parseString;
    parseStringLength   = (signed int)strlen(parseString);
    pos = 0;
}
void tokenParser::parse(const char* _parseString, bool _keepQuotes)
{
    parseString         = _parseString;
    parseStringLength   = (signed int)strlen(parseString);
    keepQuotes          = _keepQuotes;
    pos                 = 0;
}
bool tokenParser::delimiterAhead()
{
    signed int lookAhead   = pos; // Look ahead position
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
           token[0] = c;
           token[1] = EQUAL;
           token[2] = '\0';
           return dupString(token);
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
                    return dupString(token);
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
                return dupString(token);
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
                return dupString(token);
            }
            else
            {
                token[0] = c;
                token[1] = '\0';
                return dupString(token);
            }
         }
        
        token[t] = c;
        t++;
    }
    if(t > 0)
    {
        token[t] = '\0';
        return dupString(token);
    }
    eof = true;
    return nullptr;
}
