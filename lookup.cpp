#pragma once
#include "parseSQL.cpp"
#include "sqlCommon.h"
#include "utilities.cpp"

class lookup
{
    public:

    static Column*     getColumnByName(list<Column*>, char*);
    static Column*     getColumnByAlias(list<Column*>, char*);
    static sTable*     getTableByName(list<sTable*>, char*);
    static sTable*     getTableByAlias(list<sTable*>, char*);
    static TokenPair*  tokenSplit(char*,char*);
    static signed int  findDelimiter(char*, char*);
    static signed int  findDelimiterFromList(char*, list<char*>);
    static SQLACTION   determineAction(char* _token);
};

/******************************************************
 * Get Table By Alias
 ******************************************************/
sTable* lookup::getTableByAlias(list<sTable*> _tables, char* _alias)
{
    for(sTable* tbl : _tables)
    {
       // fprintf(traceFile,"\n looking for %s found %s",_alias,tbl->alias);
       if(tbl->alias == nullptr)
            continue;
        if(strcasecmp(tbl->alias, _alias) == 0)
            return tbl;
    }
    return nullptr;
}
/******************************************************
 * Get Table By Name
 ******************************************************/
sTable* lookup::getTableByName(list<sTable*> _tables, char* _tableName)
{
    if(_tableName == nullptr)
        return nullptr;

    for(sTable* tbl : _tables)
    {
       // fprintf(traceFile,"\n looking for %s found %s",_tableName,tbl->name);
        if(strcasecmp(tbl->name, _tableName) == 0)
            return tbl;
    }
    return nullptr;
}
/******************************************************
 * Get Column By Name
 ******************************************************/
Column* lookup::getColumnByName(list<Column*> _columns, char* _name)
{
    if(_name == nullptr)
        return nullptr;
    for(Column* col : _columns)
    {
        if(strcasecmp(col->name,_name) == 0)
            return col;
    }
    return nullptr;
}
/******************************************************
 * Get Column By Alias
 ******************************************************/
Column* lookup::getColumnByAlias(list<Column*> _columns, char* _name)
{
    if(_name == nullptr)
        return nullptr;
    for(Column* col : _columns)
    {
        if(col->alias != nullptr)
            if(strcasecmp(col->alias,_name) == 0)
                return col;
    }
    return nullptr;
}

/******************************************************
 * Find Delimiter
 ******************************************************/
signed int lookup::findDelimiter(char* _string, char* _delimiter)
{
    if(_string == nullptr
    || _delimiter == nullptr)
        return DELIMITERERR;

    char buff[MAXSQLSTRINGSIZE];
    bool betweenQuotes = false;
    char* str = dupString(_string);

    //if the delimeter is enclosed in quotes, ignore it.
    // To do so, blank out everything between quotes in the search buffer
    for(size_t i = 0;i<strlen(str);i++)
    {
        if(str[i] == QUOTE)
        {
            if(betweenQuotes)
                betweenQuotes = false; 
            else
                betweenQuotes = true; 
        }
        
        if(betweenQuotes)
            buff[i] = ' ';
        else
            if((int)str[i] > 96
            && (int)str[i] < 123)
            {
                buff[i] = (char)toupper(str[i]);
            }
            else{
                buff[i] = str[i];
            }
            
    }
    buff[strlen(str)] = '\0';
   // if(debug)
   //    fprintf(traceFile,"\nFind delimiter buffer:%s delimiter=%s",buff,_delimiter);

    char *s;
    s = strstr(buff, _delimiter);      // search for string "hassasin" in buff

    if (s != NULL)                     // if successful then s now points at "hassasin"
    {
        size_t result = s - buff;

        //Need to make sure this is an actual delimiter rather than a keyword embedded in another word.  example "DELETE" found in DELETED
        // in short, a space must follow a keyword

        size_t expectingSpace = strlen(_delimiter) + result;
        if(expectingSpace < strlen(str))
        {
            if(str[expectingSpace] != SPACE)
                return NEGATIVE;
        }

        return (signed int)result;
    }                                  

    return NEGATIVE;
}
/******************************************************
 * Find Delimiter From List
 ******************************************************/
//Returns the first delimiter to appear in the string
signed int lookup::findDelimiterFromList(char* _string, list<char*> _list)
{
    signed int len = (signed int)strlen(_string);
    signed int found = len;
    signed int result;
    for(char* delimiter : _list)
    {
        result = findDelimiter(_string,delimiter);
        if(result == DELIMITERERR)
            return DELIMITERERR;

        if(result != NEGATIVE
        && result < found)
            found = result;
    }
    if(found < len)
        return found;
    return NEGATIVE;
}
/******************************************************
 * Determine Actionm
 ******************************************************/
SQLACTION lookup::determineAction(char* _token)
{
    if(debug)
      fprintf(traceFile,"\n determine action");

    if(strcasecmp(_token,sqlTokenSelect) == 0)
    {
        return SQLACTION::SELECT;
    }

    if(strcasecmp(_token,sqlTokenInsert) == 0)
    {
        return SQLACTION::INSERT;
    }
    
    if(strcasecmp(_token,sqlTokenUpdate) == 0)
    {
        return  SQLACTION::UPDATE;
    }

    if(strcasecmp(_token,sqlTokenDelete) == 0)
    {
        return SQLACTION::DELETE;
    }

    if(strcasecmp(_token,sqlTokenCreate) == 0)
    {
        return  SQLACTION::CREATE;
    }


    if(strcasecmp(_token,sqlTokenJoin) == 0)
    {
        return  SQLACTION::JOIN;
    }

    if(strcasecmp(_token,sqlTokenInner) == 0)
    {
        return  SQLACTION::INNER;
    }

    if(strcasecmp(_token,sqlTokenOuter) == 0)
    {
        return  SQLACTION::OUTER;
    }

    if(strcasecmp(_token,sqlTokenLeft) == 0)
    {
        return  SQLACTION::LEFT;
    }

    if(strcasecmp(_token,sqlTokenRight) == 0)
    {
        return  SQLACTION::RIGHT;
    }

    if(strcasecmp(_token,sqlTokenNatural) == 0)
    {
        return  SQLACTION::NATURAL;
    }

    if(strcasecmp(_token,sqlTokenCross) == 0)
    {
        return  SQLACTION::CROSS;
    }

    if(strcasecmp(_token,sqlTokenFull) == 0)
    {
        return  SQLACTION::FULL;
    }


    return  SQLACTION::INVALID;

}
/******************************************************
 * Token Split
 ******************************************************/
TokenPair* lookup::tokenSplit(char* _token, char* delimiter)
{
    if(_token == nullptr)
    {
        return nullptr;
    }
    TokenPair* tp = new TokenPair();
    
    size_t len = strlen(_token);
    if(len > 1)
    {
        lTrim(_token,' ');
        rTrim(_token);
       //TODO scrub(_token);
    }
    
    
    char *s;
    s = strstr(_token, delimiter);      // search for string "hassasin" in buff
    if (s == NULL)                     // if successful then s now points at "hassasin"
    {
        tp->one = _token;
        tp->two = nullptr;
        return tp;
    } 
    size_t position = s - _token;
    tp->one = (char*)malloc(position);
    strncpy(tp->one, _token, position);
    tp->one[position] = '\0';
    
    position++;
    tp->two = (char*)malloc(len-position);

    strncpy(tp->two, _token+position, len-position);
    tp->two[len-position] = '\0';
    return tp;
}
