#pragma once
#include <string>
#include <list>
#include <vector>
#include <algorithm>
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
#define sqlTokenEqual       "="
#define sqlTokenInto        "INTO"
#define sqlTokenFrom        "FROM"
#define sqlTokenWhere       "WHERE"
#define sqlTokenSet         "SET"
#define sqlTokenAnd         "AND"
#define sqlTokenOr          "OR"
#define sqlTokenValues      "VALUES"

class Condition
{
    public:
        char*   prefix          = nullptr; //  (
        char*   Condition       = nullptr;
        char*   ColumnName      = nullptr;  // described by user
        column* Column          = nullptr;  // actual column loaded by engine
        char*   Operator        = nullptr;
        char*   Value           = nullptr;
        char*   suffix          = nullptr;  // )
};

enum class SQLACTION{
    NOACTION,
    INVALID,
    INSERT,
    CREATE,
    SELECT,
    UPDATE,
    DELETE
};

class ColumnValue
{
    //Note: all that is needed for update parsing is the column name
    //      the column and all of its edits are added in the sqlEngine
    public:
    char*   ColumnName;
    char*   ColumnValue;
    column* Column;
};

/******************************************************
 * 
 * This class provide raw syntax checking only.
 * 
 * Expect no table, column or data validation.
 * 
 ******************************************************/

class sqlParser
{
    const char*     sqlString;
    size_t          sqlStringLength = 0;
    size_t          pos             = 0; //pointer to position in string being parsed
    public:

    list<char*>             queryTable;
    std::vector<char*>      queryColumn;
    std::vector<char*>      queryValue;
    list<Condition*>        conditions;
    list<ColumnValue*>      columnValue;

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
    ParseResult     parseUpdate();
    ParseResult     determineAction(char*);
    ParseResult     parseTableList(char*);
    ParseResult     parseColumnValue();
    ParseResult     parseColumnList();
    ParseResult     parseValueList();
    ParseResult     parseConditions();
    ParseResult     addCondition(char*);
    ParseResult     validateSQLString();
    bool            compareCaseInsensitive(char*, const char*);
    bool            isNumeric(char*);
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
        errText.append("The first token is null ");
        return ParseResult::FAILURE;
    }

    if(validateSQLString() == ParseResult::FAILURE)
        return ParseResult::FAILURE;

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
            return parseUpdate();
            break;
        case SQLACTION::DELETE:
            return ParseResult::FAILURE;
            break;
    }

    return ParseResult::FAILURE;

}
/******************************************************
 * Validate SQL String
 ******************************************************/
ParseResult sqlParser::validateSQLString()
{

    string sql;
    sql.append(sqlString);
    if(std::count(sql.begin(), sql.end(), '(')
    != std::count(sql.begin(), sql.end(), ')'))
    {
        errText.append(" open condition '(' does not match close condition ')'");
        return ParseResult::FAILURE;
    }

    bool even = std::count(sql.begin(), sql.end(), '"') % 2 == 0;
    if(!even)
    {
        errText.append(" extra or missing quote.");
        return ParseResult::FAILURE;
    }

    return ParseResult::SUCCESS;
}
/******************************************************
 * Determine Actionm
 ******************************************************/
ParseResult sqlParser::determineAction(char* _token)
{
    if(compareCaseInsensitive(_token,sqlTokenSelect))
    {
        sqlAction = SQLACTION::SELECT;
        return ParseResult::SUCCESS;
    }

    if(compareCaseInsensitive(_token,sqlTokenInsert))
    {
        sqlAction = SQLACTION::INSERT;
        return ParseResult::SUCCESS;
    }
    
    if(compareCaseInsensitive(_token,sqlTokenUpdate))
    {
        sqlAction = SQLACTION::UPDATE;
        return ParseResult::SUCCESS;
    }

    if(compareCaseInsensitive(_token,sqlTokenDelete))
    {
        sqlAction = SQLACTION::DELETE;
        return ParseResult::SUCCESS;
    }

    if(compareCaseInsensitive(_token,sqlTokenCreate))
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
    if(compareCaseInsensitive(token,sqlTokenTop))
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

    if(parseTableList((char*)sqlTokenWhere) == ParseResult::FAILURE)
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
    if(!compareCaseInsensitive(token,sqlTokenInto))
    {
        errText.append(" <p> expecting the literal 'into'");
        return ParseResult::FAILURE;
    }

    if(parseTableList((char*)sqlTokenValues) == ParseResult::FAILURE)
        return ParseResult::FAILURE;

    if(parseColumnList() == ParseResult::FAILURE)
        return ParseResult::FAILURE;

    if(parseValueList() == ParseResult::FAILURE)
        return ParseResult::FAILURE;

    if(queryColumn.size() > 1
    && queryColumn.size() != queryValue.size())
    {
        errText.append(" columns = ");
        errText.append(std::to_string(queryColumn.size())); 
        errText.append(" ");
        for (char* value : queryColumn)
        {
            errText.append(value);
            errText.append("|");
        }
        errText.append(" values = ");
        errText.append(std::to_string(queryValue.size())); 
        errText.append(" ");
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
 * Parse Update
 ******************************************************/
ParseResult sqlParser::parseUpdate()
{
    if(parseTableList((char*)sqlTokenSet) == ParseResult::FAILURE)
        return ParseResult::FAILURE;

    if(parseColumnList() == ParseResult::FAILURE)
        return ParseResult::FAILURE;

    if(parseConditions() == ParseResult::FAILURE)
        return ParseResult::FAILURE;

    return ParseResult::SUCCESS;
}
/******************************************************
 * Parse Column List (for Update)
 ******************************************************/
ParseResult sqlParser::parseColumnValue()
{
    char* token;
    bool  isColumnFlag = true;
    ColumnValue* colVal = new ColumnValue();
    while(pos < sqlStringLength)
    {
        token = getToken();
        if(compareCaseInsensitive(token,sqlTokenWhere))
            return ParseResult::SUCCESS;

        if(strcmp(token,sqlTokenEqual) == 0)
        {
            continue;
        }
        if(isColumnFlag)
        {
            isColumnFlag = false;
            colVal->ColumnName = token;
            continue;
        }
        isColumnFlag = true;
        colVal->ColumnValue = token;
        columnValue.push_back(colVal);
    }
    errText.append("Expecting a 'where' clause");
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
    
    if(compareCaseInsensitive(_token,"("))
    {
        condition->prefix = _token;
        return ParseResult::SUCCESS;
    }

    if(compareCaseInsensitive(_token,")"))
    {
        condition->suffix = _token;
        return ParseResult::SUCCESS;
    }

    if(compareCaseInsensitive(_token,"AND"))
    {
        condition->Condition = _token;
        return ParseResult::SUCCESS;
    }

    if(compareCaseInsensitive(_token,"OR"))
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
        if(strcmp(_token,"=") != 0
        && !compareCaseInsensitive(_token,"like"))
        {
            errText.append(_token);
            errText.append(" condition operator missing or not = or like");
            return ParseResult::FAILURE;
        }
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
    
    //No conditions test
    if(pos >= sqlStringLength)
        return ParseResult::SUCCESS;

    while(pos < sqlStringLength)
    {
        token = getToken();

        if(addCondition(token) == ParseResult::FAILURE)
            return ParseResult::FAILURE;
    }
    if(conditions.size() == 0)
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

        if(sqlAction == SQLACTION::SELECT)
        {
            if(compareCaseInsensitive(token,sqlTokenFrom))
                return ParseResult::SUCCESS;
        }
        else
        {
            if(sqlAction == SQLACTION::INSERT)
            {
                if(compareCaseInsensitive(token,sqlTokenCloseParen)
                || compareCaseInsensitive(token,sqlTokenValues))
                {
                    // if no columns specified then all columns are assumed
                    //      an asterisk signifies all columns
                    if(queryColumn.size() == 0)
                      queryColumn.push_back((char*)sqlTokenAsterisk); 

                    return ParseResult::SUCCESS;
                }
            }
        }
        
        if(strcmp(token,")") == 0
        || strcmp(token,"(") == 0)
            continue;

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
    while(pos < sqlStringLength)
    {

        token = getToken();

        if(compareCaseInsensitive(token,sqlTokenOpenParen))
            continue;
        
        if(compareCaseInsensitive(token,sqlTokenCloseParen))
            continue;
        
        if(compareCaseInsensitive(token,sqlTokenValues))
            continue;

        if(sqlAction == SQLACTION::INSERT)
        {
            if(compareCaseInsensitive(token,sqlTokenCloseParen))
            {
                if(queryValue.size() == 0)
                {
                    errText.append("<p> expecting at least one value");
                    return ParseResult::FAILURE;
                }
                return ParseResult::SUCCESS;
            }
        }
        
        queryValue.push_back(token);
    }
    if(queryValue.size() == 0)
        errText.append(" expecting value list");

    return ParseResult::FAILURE;
}
/******************************************************
 * Parse Table List
 ******************************************************/
ParseResult sqlParser::parseTableList(char* stopToken)
{
    char* token;

    while(pos < sqlStringLength)
    {
        token = getToken();

        if (compareCaseInsensitive(token,stopToken)
        || strcmp(token,sqlTokenOpenParen) == 0)
        {
            if((sqlAction == SQLACTION::INSERT
            || sqlAction == SQLACTION::UPDATE)
            && queryTable.size() > 1)
            {
                errText.append("<p> insert table count > 1 count=");
                errText.append(std::to_string(queryTable.size()));
                return ParseResult::FAILURE;
            }
            break;
        }
        queryTable.push_back(token);
    }
    if(queryTable.size() == 0)
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
            || c == RETURN
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
        ||   c == CLOSEPAREN
        ||   c == EQUAL)
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

/******************************************************
 * Ignoring Case Is Equal
 ******************************************************/
bool sqlParser::compareCaseInsensitive(char* _str1, const char* _str2)
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

