#pragma once
#include <string>
#include <list>
#include <vector>
#include "sqlClassLoader.h"

using namespace std;

#define sqlTokenSelect      "SELECT"
#define sqlTokenCreate      "CREATE"
#define sqlTokenInsert      "INSERT"
#define sqlTokenDelete      "DELETE"
#define sqlTokenUpdate      "UPDATE"
#define sqlTokenTop         "TOP"
#define sqlTokenAsterisk    "*"
#define sqlTokenOpenParen   "("     //Note difference between OPENPAREN and sqlTokenOpenParen
#define sqlTokenCloseParen   ")"
#define sqlTokenInto        "INTO"
#define sqlTokenFrom        "FROM"
#define sqlTokenWhere       "WHERE"
#define sqlTokenAnd         "AND"
#define sqlTokenOr          "OR"
#define sqlTokenValues      "VALUES"

/******************************************************
 * 
 * This class provide raw syntax checking only.
 * 
 * Expect no table, column or data validation.
 * 
 ******************************************************/

enum class SQLACTION{
    NOACTION,
    INVALID,
    INSERT,
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

    list<char*>     queryTable;
    std::vector<char*>     queryColumn;
    std::vector<char*>     queryValue;
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
    ParseResult     parseSelect();
    ParseResult     parseInsert();
    ParseResult     determineAction(char*);
    ParseResult     parseTableList();
    ParseResult     parseColumnList();
    ParseResult     parseValueList();
    ParseResult     parseConditions();
    ParseResult     addCondition(char*);
    bool            ignoringCaseIsEqual(char*, const char*);
    bool            isNumeric(char*);
    bool            isDelimiter();
};
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
        errText.append(" cannot determine action: select, insert, update? ");
        return ParseResult::FAILURE;
    }

    switch(sqlAction)
    {
        case SQLACTION::NOACTION:
            return ParseResult::FAILURE;
            break;
        case SQLACTION::INVALID:
            return ParseResult::FAILURE;
            break;
        case SQLACTION::SELECT:
            return parseSelect();
            break;
        case SQLACTION::INSERT:
            return parseInsert();
            break;
        case SQLACTION::CREATE:
            return ParseResult::FAILURE;
            break;
        case SQLACTION::UPDATE:
            return ParseResult::FAILURE;
            break;
        case SQLACTION::DELETE:
            return ParseResult::FAILURE;
            break;
    }

    return ParseResult::FAILURE;

}
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

    if(ignoringCaseIsEqual(_token,sqlTokenInsert))
    {
        sqlAction = SQLACTION::INSERT;
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
 * Parse Select
 ******************************************************/
ParseResult sqlParser::parseSelect()
{
    
    char* token     = getToken();
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
        pos = pos - (strlen(token) + 1);
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
 * Parse Insert
 ******************************************************/
ParseResult sqlParser::parseInsert()
{
    
    char* token     = getToken();
    if(!ignoringCaseIsEqual(token,sqlTokenInto))
    {
        errText.append(" <p> expecting the literal 'into'");
        return ParseResult::FAILURE;
    }

    if(parseTableList() == ParseResult::FAILURE)
        return ParseResult::FAILURE;

    if(parseColumnList() == ParseResult::FAILURE)
        return ParseResult::FAILURE;

    if(parseValueList() == ParseResult::FAILURE)
        return ParseResult::FAILURE;

    if(queryColumn.size() > 1
    && queryColumn.size() != queryValue.size())
    {
        errText.append(" columns =");
        errText.append(std::to_string(queryColumn.size())); 
        errText.append(" values =");
        errText.append(std::to_string(queryValue.size())); 
        for (char* value : queryValue)
        {
            errText.append(value);
            errText.append("|");
        }
        return ParseResult::FAILURE;
    }

    return ParseResult::SUCCESS;
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
    int stop = 0;
    while(pos < sqlStringLength)
    {
        stop++;
        if(stop > 20)
        { 
            errText.append("\n parse column list loop");
            return ParseResult::FAILURE;
        }
        token = getToken();

        if(sqlAction == SQLACTION::SELECT)
        {
            if(ignoringCaseIsEqual(token,sqlTokenFrom))
                return ParseResult::SUCCESS;
        }else
        {
            if(sqlAction == SQLACTION::INSERT)
            {
                if(ignoringCaseIsEqual(token,sqlTokenCloseParen)
                || ignoringCaseIsEqual(token,sqlTokenValues))
                {
                    // if no columns specified then all columns are assumed
                    //      an asterisk signifies all columns
                    //if(count == 0)
                     queryColumn.push_back((char*)sqlTokenAsterisk); 

                    return ParseResult::SUCCESS;
                }
            }
        }
        
        queryColumn.push_back(token);
        count++;
    }
    if(count == 0)
        errText.append(" expecting column list");

    if(sqlAction == SQLACTION::SELECT)
        errText.append(" expecting FROM after column list");

    if(sqlAction == SQLACTION::INSERT)
        errText.append(" expecting closing parenthesis after column list");

    return ParseResult::FAILURE;
}
/******************************************************
 * Parse Value List
 ******************************************************/
ParseResult sqlParser::parseValueList()
{
    char* token;
    int count = 0;
    int stop = 0;
    while(pos < sqlStringLength)
    {
        stop++;
        if(stop > 20)
        { 
            errText.append("\n parse value list loop");
            return ParseResult::FAILURE;
        }
        token = getToken();

        if(ignoringCaseIsEqual(token,sqlTokenOpenParen))
            continue;
        
        if(ignoringCaseIsEqual(token,sqlTokenValues))
            continue;

        if(sqlAction == SQLACTION::INSERT)
        {
            if(ignoringCaseIsEqual(token,sqlTokenCloseParen))
            {
                if(count < 1)
                {
                    errText.append("<p> expecting at least one value");
                    return ParseResult::FAILURE;
                }
                return ParseResult::SUCCESS;
            }
        }
        
        queryValue.push_back(token);
        count++;
    }
    if(count == 0)
        errText.append(" expecting value list");

    if(sqlAction == SQLACTION::INSERT)
        errText.append(" expecting closing parenthesis after value list");

    return ParseResult::FAILURE;
}
/******************************************************
 * Parse Table List
 ******************************************************/
ParseResult sqlParser::parseTableList()
{
    char* token;
    int count = 0;

    int stop = 0;
    while(pos < sqlStringLength)
    {
        token = getToken();

        if(strlen(token) == 1
        && strcmp(token,sqlTokenOpenParen) != 0)
            continue;

        stop++;
        if(stop > 20)
        { 
            errText.append(" parse table loop ");
            return ParseResult::FAILURE;
        }

        if(sqlAction == SQLACTION::SELECT)
            if(ignoringCaseIsEqual(token,sqlTokenWhere))
                return ParseResult::SUCCESS;

        if(sqlAction == SQLACTION::INSERT)
        {
            if(ignoringCaseIsEqual(token,sqlTokenValues)
            || strcmp(token,sqlTokenOpenParen) == 0)
            {
                if(count>1)
                {
                    errText.append("<p> insert table count > 1 count=");
                    errText.append(std::to_string(count));
                    return ParseResult::FAILURE;
                }

                //if the token 'Values' read, back up for the column parser
                if(ignoringCaseIsEqual(token,sqlTokenValues))
                    pos = pos - (strlen(token) +1);

                return ParseResult::SUCCESS;
            }
        }
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
 * Get Token
 ******************************************************/
char* sqlParser::getToken()
{
    bool betweenQuotes = false;
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

            if((c == SPACE && !betweenQuotes)  //Elinimate white SPACE but not inside token
            || c == NEWLINE
            || c == TAB)
            {
                if(t > 0)
                {
                    token[t] = '\0';
                    retToken = (char*)malloc(strlen(token));
                    strcpy(retToken,token);
                    return retToken;
                }

                continue;
            }

        if ( c == OPENPAREN      //Note difference between OPENPAREN and sqlTokenOpenParen
        ||   c == CLOSEPAREN)
        {
            if(t > 0)
            {
                pos = pos -1;
                token[t] = '\0';
                retToken = (char*)malloc(strlen(token));
                strcpy(retToken,token);
                return retToken;
            }
            else
            {
                token[0] = c;
                token[1] = '\0';
                retToken = (char*)malloc(strlen(token));
                strcpy(retToken,token);
                return retToken;
            }
         }
        
        if(c == COMMA)
         {
            token[t] = '\0';
            retToken = (char*)malloc(strlen(token));
            strcpy(retToken,token);
            return retToken;
        }
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
bool sqlParser::isDelimiter()
{
    return true;
}
/******************************************************
 * Ignoring Case Is Equal
 ******************************************************/
bool sqlParser::ignoringCaseIsEqual(char* _str1, const char* _str2)
{
    if(_str1 == nullptr)
    {
        return false;
    }

    if(_str2 == nullptr)
    {
        return false;
    }

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
