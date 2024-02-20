#pragma once
#include <string>
#include <list>
#include "sqlClassLoader.h"

using namespace std;

#define sqlTokenSelect      "SELECT"
#define sqlTokenTop         "TOP"
#define sqlTokenAsterisk    "*"
#define sqlTokenFrom        "FROM"
#define sqlTokenWhere       "WHERE"
#define sqlTokenAnd         "AND"
#define sqlTokenOr          "OR"
#define sqlTokenCreate      "CREATE"
#define sqlTokenDelete      "DELETE"
#define sqlTokenUpdate      "UPDATE"

enum class SQLACTION{
    NOACTION,
    INVALID,
    CREATE,
    SELECT,
    UPDATE,
    DELETE
};

class Condition
{
    public:
        char*   Condition       = nullptr;
        char*   ColumnName      = nullptr;  // described by user
        column* Column          = nullptr;  // actual column loaded by engine
        char*   Operator        = nullptr;
        char*   Value           = nullptr;
};

class sqlParser
{
    public:

    list<char*>     queryColumn;
    list<char*>     queryTable;
    list<Condition*> conditions;

    Condition*      condition       = nullptr;

    int             rowsToReturn    = 0;
    bool            isColumn        = false;
    bool            isTable         = false;
    bool            isTop           = false;
    bool            isCondition     = false;
    SQLACTION       sqlAction       = SQLACTION::NOACTION;

    ParseResult     parse(const char*);
    ParseResult     addToken(char*);
    ParseResult     addCondition(char*);
    bool            isToken(char*);
    ParseResult     determineAction(char*);
    bool            ignoringCaseIsEqual(char*, const char*);
};
/******************************************************
 * Determine Actopm
 ******************************************************/
ParseResult sqlParser::determineAction(char* _token)
{
    if(ignoringCaseIsEqual(_token,sqlTokenSelect))
    {
        sqlAction = SQLACTION::SELECT;
        return ParseResult::SUCCESS;
    }
    
    if(ignoringCaseIsEqual(_token,sqlTokenUpdate))
    {
        sqlAction = SQLACTION::UPDATE;
        return ParseResult::SUCCESS;
    }

    if(ignoringCaseIsEqual(_token,sqlTokenDelete))
    {
        sqlAction = SQLACTION::DELETE;
        return ParseResult::SUCCESS;
    }

    if(ignoringCaseIsEqual(_token,sqlTokenCreate))
    {
        sqlAction = SQLACTION::CREATE;
        return ParseResult::SUCCESS;
    }

    sqlAction = SQLACTION::INVALID;
    return ParseResult::FAILURE;
}
/******************************************************
 * Add Condition
 ******************************************************/
ParseResult sqlParser::addCondition(char* _token)
{
    /* First sprint: Looking for 4 things:
        A column name   - text not enclosed in quotes
        A value         - enclosed in quotes
        A numeric       - no quotes, but numeric
        An operator     - Math operators =, !=, <>, >, <, >=, <=

        Initially only looking for column, opeator, value in quotes
    */

    if(condition == nullptr)
        condition = new Condition();

    if(ignoringCaseIsEqual(_token,"AND"))
    {
        condition->Condition = _token;
        return ParseResult::SUCCESS;
    }

    if(condition->ColumnName == nullptr)
    {
        condition->ColumnName = _token;
        return ParseResult::SUCCESS;
    }
    if(condition->Operator == nullptr)
    {
        condition->Operator = _token;
        return ParseResult::SUCCESS;
    }
    if(condition->Value == nullptr)
    {
        //strip quotes from value
        size_t s = 0;
        size_t len = strlen(_token);
        for(size_t i = 0;i< len; i++)
        {
            if(_token[i] == QUOTE)
                s++;

            if(_token[s] == QUOTE)
            {
                _token[i] = '\0';
            }
            else{
                _token[i] = _token[s];
            }
            s++;  
        }
        condition->Value = _token;
        conditions.push_back(condition);
        condition = new Condition();
        return ParseResult::SUCCESS;
    }
    return ParseResult::FAILURE;
}
/******************************************************
 * Add Token
 ******************************************************/
ParseResult sqlParser::addToken(char* _token)
{
    char* token = (char*)malloc(strlen(_token));
    strcpy(token,_token);

    //This is the first token and must be the sql verb
    if(sqlAction == SQLACTION::NOACTION)
    {
        if(determineAction(token) != ParseResult::SUCCESS)
                return ParseResult::FAILURE;

        isColumn = true;
        return ParseResult::SUCCESS;
    }

    //Conditions are everything after the WHERE clause
    //  It is best that the columns,values and compares be isolated
    //  from the Action segment of the statment.

    if(isCondition)
    {
        return addCondition(token);
    }

    if(isTop)
    {
        rowsToReturn = atoi(token);
        isTop = false;
        isColumn = true;
        return ParseResult::SUCCESS;
    }
    
    if(ignoringCaseIsEqual(_token,sqlTokenAsterisk))
    {
        isColumn = false;
        isTable = false;
        queryColumn.push_back(token);
        return ParseResult::SUCCESS;
    }

    if(ignoringCaseIsEqual(_token,sqlTokenFrom))
    {
        isColumn = false;
        isTable = true;
        return ParseResult::SUCCESS;
    }

    if(ignoringCaseIsEqual(token,sqlTokenTop))
    {
        isColumn = false;
        isTable = false;
        isTop   = true;
        return ParseResult::SUCCESS;
    }

    if(ignoringCaseIsEqual(token,sqlTokenWhere))
    {
        isColumn    = false;
        isTable     = false;
        isTop       = false;
        isCondition = true;
        return ParseResult::SUCCESS;
    }

    if(isColumn)
    {
        queryColumn.push_back(token);
        return ParseResult::SUCCESS;
    }

    if(isTable)
    {
        queryTable.push_back(token);
        return ParseResult::SUCCESS;
    }

    return ParseResult::FAILURE;
}
/******************************************************
 * Is Token
 ******************************************************/
bool sqlParser::isToken(char* _token)
{
    for(char* token : queryColumn)
    {
        if(strcmp(token,_token) == 0)
            return true;
    }
    return false;
}
/******************************************************
 * Parse
 ******************************************************/
ParseResult sqlParser::parse(const char* _sql)
{
    bool isToken = false;
    char token[MAXSQLTOKENSIZE];            //token buffer space            
    char c = ' ';                           //character in question
    int  t = 0;                             //token character pointer
    size_t sqlStringLength = strlen(_sql);

    //read string loop
    for (size_t i = 0;i< sqlStringLength;i++)
    {
        c = _sql[i];
        //whitespace
        if( c == SPACE
         || c == COMMA
         || c == NEWLINE
         || c == TAB)
         {
            if(!isToken)
                continue;

            if(isToken)
            {
               isToken = false;
               token[t] = '\0';

               if(addToken(token) != ParseResult::SUCCESS)
                    return ParseResult::FAILURE;

               token[0] = '\0';
               t = 0;
               continue;
            }
        }
        isToken = true;
        token[t] = c;
        t++;
    }
    if(t > 0)
    {
        token[t] = '\0';

        if(addToken(token) != ParseResult::SUCCESS)
            return ParseResult::FAILURE;
    }
    return ParseResult::SUCCESS;
}
/******************************************************
 * Ignoring Case Is Equal
 ******************************************************/
bool sqlParser::ignoringCaseIsEqual(char* _str1, const char* _str2)
{
	if(strlen(_str1) != strlen(_str2))
		return false;

	for(size_t i = 0;i<strlen(_str1); i++)
	{
		if(tolower(_str1[i]) != tolower(_str2[i]))
			return false;
	}
	return true;
}
