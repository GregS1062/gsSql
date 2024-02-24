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
    const char*     sqlString;
    size_t          sqlStringLength = 0;
    size_t          pos             = 0; //pointer to position in string being parsed
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

    char*           getToken();
    ParseResult     parse(const char*);
    ParseResult     determineAction(char*);
    ParseResult     parseColumnList();
    ParseResult     parseTableList();
    ParseResult     parseConditions();
    ParseResult     addCondition(char*);
    bool            ignoringCaseIsEqual(char*, const char*);
    bool            isNumeric(char*);
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
 * Parse Conditions
 ******************************************************/
ParseResult sqlParser::parseConditions()
{
    char* token;
    int count = 0;
    
    //No conditions test
    if(pos >= sqlStringLength)
        return ParseResult::SUCCESS;

    while(pos < sqlStringLength)
    {
        token = getToken();
        if(addCondition(token) == ParseResult::FAILURE)
            return ParseResult::FAILURE;
        count++;
    }
    if(count == 0)
    {
        errText.append(" expecting at least one condition column");
        return ParseResult::FAILURE;
    }
    return ParseResult::SUCCESS;
}
/******************************************************
 * Parse Column List
 ******************************************************/
ParseResult sqlParser::parseColumnList()
{
    char* token;
    int count = 0;
    while(pos < sqlStringLength)
    {
        token = getToken();
        if(ignoringCaseIsEqual(token,sqlTokenFrom))
            return ParseResult::SUCCESS;
        queryColumn.push_back(token);
        count++;
    }
    if(count == 0)
        errText.append(" expecting at least one column");
    errText.append(" expecting FROM after column list");
    return ParseResult::FAILURE;
}
/******************************************************
 * Parse Table List
 ******************************************************/
ParseResult sqlParser::parseTableList()
{
    char* token;
    int count = 0;
    while(pos < sqlStringLength)
    {
        token = getToken();
        if(ignoringCaseIsEqual(token,sqlTokenWhere))
            return ParseResult::SUCCESS;
        queryTable.push_back(token);
        count++;
    }
    if(count == 0)
    {
        errText.append(" expecting at least one table");
        return ParseResult::FAILURE;
    }
    return ParseResult::SUCCESS;
}
/******************************************************
 * Parse
 ******************************************************/
ParseResult sqlParser::parse(const char* _sqlString)
{
    sqlString       = _sqlString;
    sqlStringLength = strlen(_sqlString);
    
    char* token     = getToken();
    if(token == nullptr)
    {
        errText.append(" first token is null ");
        return ParseResult::FAILURE;
    }

    if(determineAction(token) == ParseResult::FAILURE)
    {
        errText.append(" cannot determine action: select, update? ");
        return ParseResult::FAILURE;
    }
    
    token = getToken();
    if(ignoringCaseIsEqual(token,sqlTokenTop))
    {
        token = getToken();
        if(!isNumeric(token))
        {
            errText.append(" expecting numeric after top ");
            return ParseResult::FAILURE;
        }
        rowsToReturn = atoi(token);
    }
    else
    {
        // Check for "top", a column was picked up - reverse
        pos = pos - strlen(token);
    }

    if(parseColumnList() == ParseResult::FAILURE)
        return ParseResult::FAILURE;

    if(parseTableList() == ParseResult::FAILURE)
        return ParseResult::FAILURE;

    if(parseConditions() == ParseResult::FAILURE)
        return ParseResult::FAILURE;

    return ParseResult::SUCCESS;
}
/******************************************************
 * Get Token
 ******************************************************/
char* sqlParser::getToken()
{
    bool isToken = false;
    char token[MAXSQLTOKENSIZE];            //token buffer space            
    char c = ' ';                           //character in question
    int  t = 0;                             //token character pointer
    token[0] = '\0';
    char* retToken;

    //read string loop
    while(pos < sqlStringLength)
    {
        c = sqlString[pos];
        pos++;
        
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

                retToken = (char*)malloc(strlen(token));
                strcpy(retToken,token);
                return retToken;
            }
        }
        isToken = true;
        token[t] = c;
        t++;
    }
    if(t > 0)
    {
        token[t] = '\0';
        retToken = (char*)malloc(strlen(token));
        strcpy(retToken,token);
        return retToken;
    }
    return nullptr;
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
/******************************************************
 * Is Numeric
 ******************************************************/
bool sqlParser::isNumeric(char* _token)
{
    for(size_t i=0;i<strlen(_token);i++)
    {
        if(!isdigit(_token[i]))
            return false;
    }
    return true;
}
