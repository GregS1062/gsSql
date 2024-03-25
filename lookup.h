#pragma once
#include "sqlParser.h"
#include "sqlCommon.h"

class lookup
{
    public:

    static sTable*     scrollTableList(map<char*,sTable*>, size_t);
    static column*     scrollColumnList(list<column*>, size_t);
    static sTable*     getTableByName(list<sTable*>, char*);
    static splitToken  tokenSplit(char*);
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
 * ScrollColumns (returns nth column)
 ******************************************************/
column* lookup::scrollColumnList(list<column*> _columns, size_t _index)
{
    size_t count = 0;
    for(column* col : _columns)
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
    char buff[2000];
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
splitToken lookup::tokenSplit(char* _token)
{
    splitToken st;
    size_t len = strlen(_token);
    char *s;
    s = strstr(_token, ".");      // search for string "hassasin" in buff
    if (s == NULL)                     // if successful then s now points at "hassasin"
    {
        st.one = _token;
    } 
    size_t position = s - _token;
    st.one = (char*)malloc(position);
    strncpy(st.one, _token, position);
    st.one[position] = '\0';
    
    position++;
    st.two = (char*)malloc(len-position);

    strncpy(st.two, _token+position, len-position);
    st.two[len-position] = '\0';
    return st;
}
