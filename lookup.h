#pragma once
#include "sqlParser.h"
#include "sqlCommon.h"
#include "utilities.h"

class lookup
{
    public:

    static sTable*     scrollTableList(map<char*,sTable*>, size_t);
    static Column*     scrollColumnList(list<Column*>, size_t);
    static Column*     getColumnByName(list<Column*>, char*);
    static sTable*     getTableByAlias(list<sTable*>, char*);
    static sTable*     getTableByName(list<sTable*>, char*);
    static TokenPair*  tokenSplit(char*,char*);
    static signed long findDelimiter(char*, char*);
    static signed long findDelimiterFromList(char*, list<char*>);
    static SQLACTION   determineAction(char* _token);
};
/******************************************************
 * Get Query Table
 ******************************************************/
sTable* lookup::scrollTableList(map<char*,sTable*> _tables, size_t _index)
{
    if(_tables.size() == 0)
        return nullptr;
    
    if(_tables.size() < _index+1)
        return nullptr;

    auto it = _tables.begin();
    std::advance(it, _index);
    return (sTable*)it->second;
}
/******************************************************
 * Get Table By Alias
 ******************************************************/
sTable* lookup::getTableByAlias(list<sTable*> _tables, char* _alias)
{
    for(sTable* tbl : _tables)
    {
       // printf("\n looking for %s found %s",_alias,tbl->alias);
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
    for(sTable* tbl : _tables)
    {
       // printf("\n looking for %s found %s",_tableName,tbl->name);
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
    for(Column* col : _columns)
    {
        if(strcasecmp(col->name,_name) == 0)
            return col;
    }
    return nullptr;
}
/******************************************************
 * ScrollColumns (returns nth column)
 ******************************************************/
Column* lookup::scrollColumnList(list<Column*> _columns, size_t _index)
{
    size_t count = 0;
    for(Column* col : _columns)
    {
        if(count == _index)
            return col;
        count++;
    }
    return nullptr;
}
/******************************************************
 * Find Delimiter
 ******************************************************/
signed long lookup::findDelimiter(char* _string, char* _delimiter)
{
    char buff[MAXSQLSTRINGSIZE];
    bool betweenQuotes = false;

    //if the delimeter is enclosed in quotes, ignore it.
    // To do so, blank out everything between quotes in the search buffer
    for(size_t i = 0;i<strlen(_string);i++)
    {
        if(_string[i] == QUOTE)
        {
            if(betweenQuotes)
                betweenQuotes = false; 
            else
                betweenQuotes = true; 
        }
        
        if(betweenQuotes)
            buff[i] = ' ';
        else
            if((int)_string[i] > 96
            && (int)_string[i] < 123)
            {
                buff[i] = (char)toupper(_string[i]);
            }
            else{
                buff[i] = _string[i];
            }
            
    }

    if(debug)
        printf("\n buff:%s d=%s",buff,_delimiter);

    char *s;
    s = strstr(buff, _delimiter);      // search for string "hassasin" in buff
    if(debug)
        printf("\n s:%s",s);
    if (s != NULL)                     // if successful then s now points at "hassasin"
    {
        long int result = s - buff;
        if(debug)
            printf(" result = %ld",result);
        return result;
    }                                  

    return NEGATIVE;
}
/******************************************************
 * Find Delimiter From List
 ******************************************************/
//Returns the first delimiter to appear in the string
signed long lookup::findDelimiterFromList(char* _string, list<char*> _list)
{
    signed long len = strlen(_string);
    signed long found = len;
    signed long result;
    for(char* delimiter : _list)
    {
        result = findDelimiter(_string,delimiter);
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
      printf("\n determine action");

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
        utilities::lTrim(_token,' ');
        utilities::rTrim(_token);
        utilities::scrub(_token);
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
