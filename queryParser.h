#pragma once
#include <string>
#include <list>
#include <vector>
#include <algorithm>
#include "tokenParser.h"
#include "sqlParser.h"

using namespace std;

class Condition
{
    public:
        char*   name            = nullptr;  // described by user
        char*   op              = nullptr;  // operator is a reserved word
        char*   value           = nullptr;
        char*   prefix          = nullptr; //  (
        char*   condition       = nullptr;
        char*   suffix          = nullptr;  // )
        column* col             = nullptr;  // actual column loaded by engine
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

class queryParser
{
    const char*         queryString;
    signed long         queryStringLength   = 0;
    tokenParser*        tok;
    sqlParser*          sqlDB;

    public:

    std::map<char*,cTable*> queryTables;
    list<Condition*>        conditions;
    list<ColumnValue*>      columnValue;

    cTable* dbTable;        //template table with all columns defined
    cTable* queryTable;     //table consisting of only columns defined in the query statement

    Condition*          condition       = nullptr;

    int                 rowsToReturn    = 0;
    bool                isColumn        = false;
    bool                isCondition     = false;
    SQLACTION           sqlAction       = SQLACTION::NOACTION;

    ParseResult         clear();
    ParseResult         parse(const char*,sqlParser*);
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
    cTable*             getQueryTable(short);
    column*             getQueryTableColumn(short);
    signed long         findDelimiter(char*, char*);
    bool                valueSizeOutofBounds(char*, column*);
};

/******************************************************
 * Clear
 ******************************************************/
ParseResult queryParser::clear()
{
    sqlAction       = SQLACTION::NOACTION;
    rowsToReturn    = 0;
    isColumn        = false;
    isCondition     = false;
    condition       = nullptr;
    dbTable         = nullptr;
    queryTable      = nullptr;
    queryTables.clear();
    conditions.clear();
    columnValue.clear();
    return ParseResult::SUCCESS;
}
/******************************************************
 * Parse
 ******************************************************/
ParseResult queryParser::parse(const char* _queryString,sqlParser* _sqlDB)
{
    queryString       = _queryString;
    queryStringLength = strlen(_queryString);
    sqlDB             = _sqlDB;
    tok               = new tokenParser(_queryString);
    
    char* token     = tok->getToken();
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
ParseResult queryParser::validateSQLString()
{

    string sql;
    sql.append(queryString);

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
ParseResult queryParser::determineAction(char* _token)
{
    if(strcasecmp(_token,sqlTokenSelect) == 0)
    {
        sqlAction = SQLACTION::SELECT;
        return ParseResult::SUCCESS;
    }

    if(strcasecmp(_token,sqlTokenInsert) == 0)
    {
        sqlAction = SQLACTION::INSERT;
        return ParseResult::SUCCESS;
    }
    
    if(strcasecmp(_token,sqlTokenUpdate) == 0)
    {
        sqlAction = SQLACTION::UPDATE;
        return ParseResult::SUCCESS;
    }

    if(strcasecmp(_token,sqlTokenDelete) == 0)
    {
        sqlAction = SQLACTION::DELETE;
        return ParseResult::SUCCESS;
    }

    if(strcasecmp(_token,sqlTokenCreate) == 0)
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
ParseResult queryParser::parseSelect()
{
    
    char* token     = tok->getToken();
    if(strcasecmp(token,sqlTokenTop) == 0)
    {
        token = tok->getToken();
        if(!sqlDB->isNumeric(token))
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
    signed long posFrom     = findDelimiter((char*)queryString, (char*)sqlTokenFrom);
    signed long posWhere    = findDelimiter((char*)queryString, (char*)sqlTokenWhere);
    if(posWhere == NEGATIVE)
        posWhere = queryStringLength;

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
    

    if(posWhere < queryStringLength)
    {
        if(parseConditions(posWhere + strlen(sqlTokenWhere), queryStringLength) == ParseResult::FAILURE)
            return ParseResult::FAILURE;
    }

    return ParseResult::SUCCESS;
}
/******************************************************
 * Parse Insert
 ******************************************************/
ParseResult queryParser::parseInsert()
{
    
    signed long posInto = findDelimiter((char*)queryString, (char*)sqlTokenInto);
    if(posInto == NEGATIVE)
    {
        errText.append(" <p> expecting the literal 'into'");
        return ParseResult::FAILURE;
    }

    // pos = abbreviation for position
    signed long posValues       = findDelimiter((char*)queryString, (char*)sqlTokenValues);
    signed long posOpenParen    = findDelimiter((char*)queryString, (char*)sqlTokenOpenParen);
    signed long posCloseParen   = findDelimiter((char*)queryString, (char*)sqlTokenCloseParen);
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

    queryTable = queryParser::getQueryTable(0);
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
        dbTable = sqlDB->getTableByName((char*)queryTable->name.c_str());
        populateQueryTable(dbTable);
    }

    if(parseValueList(posValues+strlen(sqlTokenValues),queryStringLength) == ParseResult::FAILURE)
        return ParseResult::FAILURE;

    return ParseResult::SUCCESS;
}
/******************************************************
 * Parse Update
 ******************************************************/
ParseResult queryParser::parseUpdate()
{
    pos = strlen(sqlTokenUpdate);

    signed long posSet = findDelimiter((char*)queryString, (char*)sqlTokenSet);
    if(parseTableList(pos,posSet) == ParseResult::FAILURE)
        return ParseResult::FAILURE;

    signed long posWhere = findDelimiter((char*)queryString, (char*)sqlTokenWhere);

    if(parseColumnValue(posSet+strlen(sqlTokenSet),posWhere) == ParseResult::FAILURE)
        return ParseResult::FAILURE;
 
    queryTable = getQueryTable(0);
    if(queryTable == nullptr)
    {
        errText.append(" cannot find selected table ");
        return ParseResult::FAILURE;
    }
    
    // The template table defined by the CREATE syntax)
    dbTable = sqlDB->getTableByName((char*)queryTable->name.c_str());
    
    posWhere = posWhere + strlen(sqlTokenWhere);

    if(parseConditions(posWhere,queryStringLength) == ParseResult::FAILURE)
        return ParseResult::FAILURE;
    
    
    column* col;
    for(ColumnValue* colVal : columnValue)
    {
        col = dbTable->getColumn(colVal->name);
        if(col == nullptr)
        {
            errText.append(" column not found ");
            errText.append(colVal->name);
            return ParseResult::FAILURE;
        }
        if(valueSizeOutofBounds(colVal->value,col))
            return ParseResult::FAILURE;

        col->value = colVal->value;
        queryTable->columns.insert({colVal->name, col});
    }

    return ParseResult::SUCCESS;
}
/******************************************************
 * Parse Column List (for Update)
 ******************************************************/
ParseResult queryParser::parseColumnValue(signed long _begin, signed long _end)
{
    char* token;
    bool  isColumnFlag = true;
    ColumnValue* colVal = new ColumnValue();

    pos = _begin;
    while(pos < _end)
    {
        token = tok->getToken();

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
ParseResult queryParser::addCondition(char* _token)
{
    /* First sprint: Looking for 4 things:
        A column name   - text not enclosed in quotes
        A value         - enclosed in quotes
        A numeric       - no quotes, but numeric
        An operator     - Math operators =, !=, <>, >, <, >=, <=

        Initially only looking for column, opeator, value in quotes
    */

    if(condition == nullptr)
    {
        condition = new Condition();
        condition->prefix = (char*)" ";
        condition->suffix = (char*)" ";
        condition->condition = (char*)" ";
    }
    
    if(strcasecmp(_token,(char*)sqlTokenOpenParen) == 0)
    {
        condition->prefix = _token;
        return ParseResult::SUCCESS;
    }

    if(strcasecmp(_token,(char*)sqlTokenCloseParen) == 0)
    {
        condition->suffix = _token;
        return ParseResult::SUCCESS;
    }

    if(strcasecmp(_token,(char*)sqlTokenAnd) == 0)
    {
        condition->condition = _token;
        return ParseResult::SUCCESS;
    }

    if(strcasecmp(_token,(char*)sqlTokenOr) == 0)
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
        && strcasecmp(_token,"like") != 0)
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
ParseResult queryParser::parseConditions(signed long _begin, signed long _end)
{
    char* token;
    column* col;

    pos = _begin;
    //No conditions test
    if(pos >= queryStringLength)
        return ParseResult::SUCCESS;

    if(dbTable == nullptr)
    {
        errText.append(" null table ");
        return ParseResult::FAILURE;
    }


    while(pos < _end)
    {
        token = tok->getToken();

        if(addCondition(token) == ParseResult::FAILURE)
            return ParseResult::FAILURE;
    }
    if(conditions.size() == 0)
    {
        errText.append(" expecting at least one condition column");
        return ParseResult::FAILURE;
    }
    for(Condition* con : conditions)
    {
        col = dbTable->getColumn(con->name);
        if(col == nullptr)
        {

            errText.append(" condition column |");
            errText.append(con->name);
            errText.append("| not found Value=");
            errText.append(con->value);
            return ParseResult::FAILURE;
        }

        if(valueSizeOutofBounds(con->value,col))
            return ParseResult::FAILURE;
    }
    return ParseResult::SUCCESS;
}
/******************************************************
 * Parse Column List
 ******************************************************/
ParseResult queryParser::parseColumnList(signed long _begin,signed long _end)
{
    char*   token;
    pos     = _begin;
    column* col;

    // The template table defined by the CREATE syntax
    dbTable = sqlDB->getTableByName((char*)queryTable->name.c_str());
    if(dbTable == nullptr)
    {
        errText.append(" Could not find ");
        errText.append(queryTable->name.c_str());
        errText.append(" in SQL ");
        return ParseResult::FAILURE;
    }
    //populate selected columns
    while(pos < _end)
    {
        token = tok->getToken(); 

        if(strcmp(token,(char*)sqlTokenOpenParen) == 0)
            continue;

        //populate all columns
        if(strcmp(token,sqlTokenAsterisk) == 0 
        && queryTable->columns.size() == 0)
        return populateQueryTable(dbTable);
        

        col = dbTable->getColumn(token);
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
ParseResult queryParser::parseValueList(signed long _begin,signed long _end)
{
    short int count = 0;
    column* col;
    char* token;
    pos = _begin;
    while(pos < _end)
    {

        token = tok->getToken();

        if(strcasecmp(token,sqlTokenOpenParen) == 0)
            continue;
        
        if(strcasecmp(token,sqlTokenValues) == 0)
            continue;

        if(strcasecmp(token,sqlTokenCloseParen) == 0)
        {    
            if(count == 0)
            {
                errText.append("<p> expecting at least one value");
                return ParseResult::FAILURE;
            }

            if(queryTable->columns.size() != (long unsigned int)count)
            {
                errText.append(" value count does not match number of columns in table");
                return ParseResult::FAILURE;
            }
            return ParseResult::SUCCESS;
        }

        col = getQueryTableColumn(count);

        if(valueSizeOutofBounds(token,col))
            return ParseResult::FAILURE;

        col->value = token;
        count++;
    }

    return ParseResult::SUCCESS;
}
/******************************************************
 * Parse Table List
 ******************************************************/
ParseResult queryParser::parseTableList(signed long _begin, signed long _end)
{
    cTable* table;
    char* token;
    pos = _begin;

    while(pos < _end)
    {
        token = tok->getToken();       
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
 * Find Delimiter
 ******************************************************/
signed long queryParser::findDelimiter(char* _string, char* _delimiter)
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
cTable* queryParser::getQueryTable(short index)
{
    auto it = queryTables.begin();
    std::advance(it, index);
    return (cTable*)it->second;
}
/******************************************************
 * Get Query Table Column
 ******************************************************/
column* queryParser::getQueryTableColumn(short index)
{
    auto it = queryTable->columns.begin();
    std::advance(it, index);
    return (column*)it->second;
}
/******************************************************
 * Populate Query Table
 ******************************************************/
ParseResult queryParser::populateQueryTable(cTable* table)
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
/******************************************************
 * Value Size Out Of Bounds
 ******************************************************/
bool queryParser::valueSizeOutofBounds(char* value, column* col)
{
    if(value == nullptr)
    {
        errText.append(col->name);
        errText.append(" value is missing or null ");
        return true;
    }

    if(col == nullptr)
    {
        errText.append(" value is missing or null ");
        return true;
    }

    if(strlen(value) > (size_t)col->length)
    {
        errText.append(col->name);
        errText.append(" value length ");
        errText.append(std::to_string(strlen(value)));
        errText.append(" > edit length ");
        errText.append(std::to_string(col->length));
        return true;
    }

    //value is okay
    return false;
}


