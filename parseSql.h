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
        char*   condition       = nullptr;
        char*   name            = nullptr;  // described by user
        column* col             = nullptr;  // actual column loaded by engine
        char*   op              = nullptr;  // operator is a reserved word
        char*   value           = nullptr;
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
    char*   name;
    char*   value;
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
    const char*         sqlString;
    signed long         sqlStringLength = 0;
    signed long         pos             = 0; //pointer to position in string being parsed
    
    sqlClassLoader*     loader;

    public:

    std::map<char*,cTable*> queryTables;
    list<Condition*>        conditions;
    list<ColumnValue*>      columnValue;

    cTable* queryTable;

    Condition*          condition       = nullptr;

    int                 rowsToReturn    = 0;
    bool                isColumn        = false;
    bool                isCondition     = false;
    SQLACTION           sqlAction       = SQLACTION::NOACTION;

    ParseResult         parse(const char*,sqlClassLoader*);
    ParseResult         parseSelect();
    ParseResult         parseInsert();
    ParseResult         parseUpdate();
    ParseResult         determineAction(char*);
    ParseResult         parseTableList(signed long, signed long);
    ParseResult         parseColumnValue(signed long, signed long);
    ParseResult         parseColumnList(signed long, signed long);
    ParseResult         parseValueList(signed long, signed long);
    ParseResult         parseConditions(signed long, signed long);
    ParseResult         addCondition(char*);
    ParseResult         validateSQLString();
    ParseResult         populateQueryTable(cTable* table);
    char*               getToken();
    cTable*             getQueryTable(short);
    column*             getQueryTableColumn(short);
    bool                compareCaseInsensitive(char*, const char*);
    bool                isNumeric(char*);
    signed long         findDelimiter(char*, char*);
};
/******************************************************
 * Parse
 ******************************************************/
ParseResult sqlParser::parse(const char* _sqlString,sqlClassLoader* _loader)
{
    sqlString       = _sqlString;
    sqlStringLength = strlen(_sqlString);
    loader          = _loader;
    
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

    // Do open and close parenthesis match?
    if(std::count(sql.begin(), sql.end(), '(')
    != std::count(sql.begin(), sql.end(), ')'))
    {
        errText.append(" open condition '(' does not match close condition ')'");
        return ParseResult::FAILURE;
    }

    //Do quotes match?
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

    signed long posColumn   = pos;
    signed long posFrom     = findDelimiter((char*)sqlString, (char*)sqlTokenFrom);
    signed long posWhere    = findDelimiter((char*)sqlString, (char*)sqlTokenWhere);
    if(posWhere == NEGATIVE)
        posWhere = sqlStringLength;

    if(parseTableList(posFrom+strlen(sqlTokenFrom),posWhere) == ParseResult::FAILURE)
        return ParseResult::FAILURE;

    if(queryTables.size() == 0)
    {
        errText.append(" no selected tables found in string ");
        return ParseResult::FAILURE;
    }

    /* Populate the query table with the columns that 
        will be used by the engine
    */

    queryTable = getQueryTable(0);
    if(queryTable == nullptr)
    {
        errText.append(" cannot find selected table ");
        return ParseResult::FAILURE;
    }

    if(parseColumnList(posColumn,posFrom) == ParseResult::FAILURE)
        return ParseResult::FAILURE;
    

    if(posWhere < sqlStringLength)
    {
        if(parseConditions(posWhere + strlen(sqlTokenWhere), sqlStringLength) == ParseResult::FAILURE)
            return ParseResult::FAILURE;
    }

    return ParseResult::SUCCESS;
}
/******************************************************
 * Parse Insert
 ******************************************************/
ParseResult sqlParser::parseInsert()
{
    
    signed long posInto = findDelimiter((char*)sqlString, (char*)sqlTokenInto);
    if(posInto == NEGATIVE)
    {
        errText.append(" <p> expecting the literal 'into'");
        return ParseResult::FAILURE;
    }

    // pos = abbreviation for position
    signed long posValues       = findDelimiter((char*)sqlString, (char*)sqlTokenValues);
    signed long posOpenParen    = findDelimiter((char*)sqlString, (char*)sqlTokenOpenParen);
    signed long posCloseParen   = findDelimiter((char*)sqlString, (char*)sqlTokenCloseParen);
    signed long posCloseTable;
    
    if(posValues < posOpenParen)
    {
        posCloseTable = posValues;
    }
    else
    {
        posCloseTable = posOpenParen;
    }
    
    if(parseTableList(posInto + strlen(sqlTokenInto),posCloseTable) == ParseResult::FAILURE)
        return ParseResult::FAILURE;

    queryTable = sqlParser::getQueryTable(0);
    if(queryTable == nullptr)
    {
        errText.append(" cannot identify query table ");
        return ParseResult::FAILURE;
    }

    if(posValues > posOpenParen)
    {
        // Signature (col,col,col) Values
        if(parseColumnList(posOpenParen,posCloseParen) == ParseResult::FAILURE)
            return ParseResult::FAILURE;
    }
    else{
        cTable* loadTable = loader->getTableByName((char*)queryTable->name.c_str());
        populateQueryTable(loadTable);
    }

    if(parseValueList(posValues+strlen(sqlTokenValues),sqlStringLength) == ParseResult::FAILURE)
        return ParseResult::FAILURE;

    return ParseResult::SUCCESS;
}
/******************************************************
 * Parse Update
 ******************************************************/
ParseResult sqlParser::parseUpdate()
{
    pos = strlen(sqlTokenUpdate);
    signed long posSet = findDelimiter((char*)sqlString, (char*)sqlTokenSet);
    if(parseTableList(pos,posSet) == ParseResult::FAILURE)
        return ParseResult::FAILURE;

    signed long posWhere = findDelimiter((char*)sqlString, (char*)sqlTokenWhere);
    if(parseColumnValue(posSet+strlen(sqlTokenSet),posWhere) == ParseResult::FAILURE)
        return ParseResult::FAILURE;

    posWhere = posWhere + strlen(sqlTokenWhere);
    if(parseConditions(posWhere,sqlStringLength) == ParseResult::FAILURE)
        return ParseResult::FAILURE;

    queryTable = getQueryTable(0);
    if(queryTable == nullptr)
    {
        errText.append(" cannot find selected table ");
        return ParseResult::FAILURE;
    }
    
    // The template table defined by the CREATE syntax (jason for now)
    cTable* loadTable = loader->getTableByName((char*)queryTable->name.c_str());
    column* col;
    for(ColumnValue* colVal : columnValue)
    {
        col = loadTable->getColumn(colVal->name);
        if(col == nullptr)
        {
            errText.append(" column not found ");
            errText.append(colVal->name);
            return ParseResult::FAILURE;
        }
        if(strlen(colVal->value) > (size_t)col->length)
        {
            errText.append(colVal->name);
            errText.append(" value > edit length ");
            return ParseResult::FAILURE;
        }
        col->value = colVal->value;
        queryTable->columns.insert({colVal->name, col});
    }

    return ParseResult::SUCCESS;
}
/******************************************************
 * Parse Column List (for Update)
 ******************************************************/
ParseResult sqlParser::parseColumnValue(signed long _begin, signed long _end)
{
    char* token;
    bool  isColumnFlag = true;
    ColumnValue* colVal = new ColumnValue();
    pos = _begin;
    while(pos < _end)
    {
        token = getToken();

        if(strcmp(token,sqlTokenEqual) == 0)
        {
            continue;
        }
        if(isColumnFlag)
        {
            isColumnFlag = false;
            colVal = new ColumnValue();
            colVal->name = token;
            continue;
        }
        isColumnFlag = true;
        colVal->value = token;
        columnValue.push_back(colVal);
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
        condition->condition = _token;
        return ParseResult::SUCCESS;
    }

    if(compareCaseInsensitive(_token,"OR"))
    {
        condition->condition = _token;
        return ParseResult::SUCCESS;
    }

    if(condition->name == nullptr)
    {
        condition->name = _token;
        return ParseResult::SUCCESS;
    }
    if(condition->op == nullptr)
    {
        if(strcmp(_token,"=") != 0
        && !compareCaseInsensitive(_token,"like"))
        {
            errText.append(_token);
            errText.append(" condition operator missing or not = or like");
            return ParseResult::FAILURE;
        }
        condition->op = _token;
        return ParseResult::SUCCESS;
    }
    if(condition->value == nullptr)
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

        condition->value = _token;
        conditions.push_back(condition);

        condition = new Condition();
        return ParseResult::SUCCESS;
    }
    return ParseResult::FAILURE;
}

/******************************************************
 * Parse Conditions
 ******************************************************/
ParseResult sqlParser::parseConditions(signed long _begin, signed long _end)
{
    char* token;
    
    //No conditions test
    if(pos >= sqlStringLength)
        return ParseResult::SUCCESS;

    pos = _begin;
    while(pos < _end)
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
ParseResult sqlParser::parseColumnList(signed long _begin,signed long _end)
{
    char*       token;

    pos = _begin;

        // The template table defined by the CREATE syntax (jason for now)
    cTable* loadTable = loader->getTableByName((char*)queryTable->name.c_str());
    column* col;

    token = getToken();

    if(strcmp(token,sqlTokenAsterisk) == 0 )
        return populateQueryTable(loadTable);

    while(pos < _end)
    {
        token = getToken();  
        col = loadTable->getColumn(token);
        if(col == nullptr)
        {
            errText.append(" column not found ");
            errText.append(token);
            return ParseResult::FAILURE;
        }
        queryTable->columns.insert({token, col});
    }
    if(queryTable->columns.size() == 0)
    {
        errText.append(" expecting column list");
        return ParseResult::FAILURE;
    }

    return ParseResult::SUCCESS;
}
/******************************************************
 * Parse Value List
 ******************************************************/
ParseResult sqlParser::parseValueList(signed long _begin,signed long _end)
{
    short int count = 0;
    column* col;
    char* token;
    pos = _begin;
    while(pos < _end)
    {

        token = getToken();

        if(compareCaseInsensitive(token,sqlTokenOpenParen))
            continue;
        
        if(compareCaseInsensitive(token,sqlTokenValues))
            continue;

        if(compareCaseInsensitive(token,sqlTokenCloseParen))
        {    
            if(count == 0)
            {
                errText.append("<p> expecting at least one value");
                return ParseResult::FAILURE;
            }
            return ParseResult::SUCCESS;
        }
        /*
            DEBUG
            errText.append(" ");
            errText.append(token);
            errText.append(" ");
        */
        col = getQueryTableColumn(count);
        if((size_t)col->length < strlen(token))
        {
            errText.append(col->name);
            errText.append(" value length ");
            errText.append(std::to_string(strlen(token)));
            errText.append(" > edit length ");
            errText.append(std::to_string(col->length));
            return ParseResult::FAILURE;
        }
        col->value = token;
        count++;
    }
    return ParseResult::SUCCESS;
}
/******************************************************
 * Parse Table List
 ******************************************************/
ParseResult sqlParser::parseTableList(signed long _begin, signed long _end)
{
    char* token;
    pos = _begin;
    cTable* table;
    pos = _begin;
    while(pos < _end)
    {
        token = getToken();       
        table = new cTable();
        table->name = (char*)token;
        queryTables.insert({token,table});
    }

    if(queryTables.size() == 0)
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
    while(pos <= sqlStringLength)
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

 #include <stdlib.h>
 #include <string.h>

/******************************************************
 * Find Delimiter
 ******************************************************/
signed long sqlParser::findDelimiter(char* _string, char* _delimiter)
{
    char buff[2000];
    for(size_t i = 0;i<strlen(_string);i++)
    {
        buff[i] = (char)toupper(_string[i]);
    }
    char *s;

    s = strstr(buff, _delimiter);      // search for string "hassasin" in buff
    if (s != NULL)                     // if successful then s now points at "hassasin"
    {
        return s - buff;
    }                                  

    return NEGATIVE;
}
/******************************************************
 * Get Query Table
 ******************************************************/
cTable* sqlParser::getQueryTable(short index)
{
    auto it = queryTables.begin();
    std::advance(it, index);
    return (cTable*)it->second;
}
/******************************************************
 * Get Query Table Column
 ******************************************************/
column* sqlParser::getQueryTableColumn(short index)
{
    auto it = queryTable->columns.begin();
    std::advance(it, index);
    return (column*)it->second;
}
/******************************************************
 * Populate Query Table
 ******************************************************/
ParseResult sqlParser::populateQueryTable(cTable* table)
{
    column* col;
    map<char*,column*>::iterator itr;
    map<char*,column*>columns = table->columns;
    for (itr = columns.begin(); itr != columns.end(); ++itr) 
    {
        col = (column*)itr->second;
        queryTable->columns.insert({(char*)col->name.c_str(), col});
    }
    return ParseResult::SUCCESS;
}


