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
    tokenParser(string);
    
    void        parse(string);
    void        parse(string,bool);
    string      getToken();
    string      cleanString(string);
    bool        delimiterAhead();
    bool        eof = false;
    string      parseString;
    size_t      parseStringLength;
    size_t      pos;
};

string tokenParser::cleanString(string _string)
{
    // ABC  DEF
    size_t  len = _string.length();
    char    c;
    bool    betweenQuotes = false;
    bool    priorSpace = false;
    string  newString;

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
                newString += SPACE;
            }
            newString += c;
            if(_string[posString+1] != SPACE)
            {
                newString += SPACE;
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

        newString += c;
    }
    return newString;
}
tokenParser::tokenParser(string _parseString)
{
    parseString         = _parseString;
    parseStringLength   = parseString.length();
    pos = 0;
}
void tokenParser::parse(string _parseString)
{
    parseString         = _parseString;
    parseStringLength   = parseString.length();
    pos = 0;
}
void tokenParser::parse(string _parseString, bool _keepQuotes)
{
    parseString         = _parseString;
    parseStringLength   = parseString.length();
    keepQuotes          = _keepQuotes;
    pos                 = 0;
}
bool tokenParser::delimiterAhead()
{
    size_t lookAhead   = pos; // Look ahead position
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
string tokenParser::getToken()
{
    bool betweenQuotes = false;      
    char c = ' ';                           //character in question
    int  t = 0;                             //token character pointer
    string token;

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
           token += c;
           token += EQUAL;
           return token;
        }

        if(c == QUOTE)
        {
            if(betweenQuotes)
            {
                betweenQuotes = false;
                if(keepQuotes)
                {
                    token += c;
                    t++;
                }
                // Looking for end of value list without a comma
                // example Update tablename set column = "" where....
                if(!delimiterAhead())
                    return token;
            }
            else
                betweenQuotes = true; 

            if(keepQuotes)
            {
                token += c;
                t++;
            }

            continue;
        }
        
        if( betweenQuotes
        &&  c == SPACE)
        {
            token += c;
            continue;
        }

        if(betweenQuotes
        && 
        ( c == NEWLINE
        || c == RETURN
        || c == FORMFEED
        || c == TAB
        || c == VTAB))
            continue;


        if(c == COMMA
        || c == SPACE
        || c == NEWLINE
        || c == RETURN
        || c == FORMFEED
        || c == TAB
        || c == VTAB)
        {
            if(t > 0)
                return token;

            continue;
        }

        if ( c == OPENPAREN      //Note difference between OPENPAREN and sqlTokenOpenParen
        ||   c == CLOSEPAREN
        ||   c == EQUAL)
        {
            if(t > 0)
            {
                token.pop_back();
                return token;
            }
            else
            {
                token += c;
                return token;
            }
         }
        t++;
        token += c;
    }
    eof = true;
    return token;
}
