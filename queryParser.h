#pragma once
#include <string>
#include <list>
#include <vector>
#include <algorithm>
#include "tokenParser.h"
#include "sqlParser.h"
#include "sqlCommon.h"
#include "conditions.h"
#include "utilities.h"
#include "lookup.h"


using namespace std;


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

    list<sTable*>           tables;         // tables
    list<Condition*>        conditions;     // where condition (operator) value
    list<ColumnValue*>      columnValue;    // used in update to set list of column/values

    sTable* dbTable;        //template table with all columns defined
    sTable* queryTable;     //table consisting of only columns defined in the query statement

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
    ParseResult         parseTableList(char*);
    ParseResult         parseColumnValue(signed long, signed long);
    ParseResult         parseColumnList(signed long, signed long);
    ParseResult         parseValueList(signed long, signed long);
    ParseResult         parseConditions(signed long, signed long);
    ParseResult         addCondition(char*);
    ParseResult         validateSQLString();
    ParseResult         populateQueryTable(sTable* table);
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
    tables.clear();
    conditions.clear();
    columnValue.clear();
    return ParseResult::SUCCESS;
}
/******************************************************
 * Parse
 ******************************************************/
ParseResult queryParser::parse(const char* _queryString,sqlParser* _sqlDB)
{
    //debug = true;
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

    sqlAction = lookup::determineAction(token);
    if(sqlAction == SQLACTION::INVALID)
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

    if(debug)
      printf("\n validateSQLString");

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
 * Parse Select
 ******************************************************/
ParseResult queryParser::parseSelect()
{
    if(debug)
        printf("\n parse select");

    char* token     = tok->getToken();
    if(strcasecmp(token,sqlTokenTop) == 0)
    {
        token = tok->getToken();
        if(!utilities::isNumeric(token))
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

    signed long posFrom     = lookup::findDelimiter((char*)queryString, (char*)sqlTokenFrom);
  
    if(posFrom == NEGATIVE)
    {
        errText.append(" missing FROM token");
        return ParseResult::FAILURE;
    }

    list<char*> delimiterList;
    delimiterList.push_back((char*)sqlTokenWhere);
    delimiterList.push_back((char*)sqlTokenOn);
    delimiterList.push_back((char*)sqlTokenJoin);

    char fromClause[MAXSQLSTRINGSIZE];
    strcpy(fromClause, (char*)queryString+(posFrom+1+strlen(sqlTokenFrom)));
    
    long signed found       = lookup::findDelimiterFromList(fromClause,delimiterList);
    if(found > NEGATIVE)
        strncpy(fromClause, queryString+posFrom, found);

    if(parseTableList(fromClause) == ParseResult::FAILURE)
        return ParseResult::FAILURE;

    if(tables.size() == 0)
    {
        errText.append(" no selected tables found in string ");
        return ParseResult::FAILURE;
    }

    /* Populate the query table with the columns that 
        will be used by the engine
    */

    queryTable = tables.front();
    if(queryTable == nullptr)
    {
        errText.append(" cannot find selected table ");
        return ParseResult::FAILURE;
    }

    signed long posColumn   = pos;

    if(parseColumnList(posColumn,posFrom) == ParseResult::FAILURE)
        return ParseResult::FAILURE;
    
    long signed posWhere      = lookup::findDelimiter((char*)queryString,(char*)sqlTokenWhere);
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
    char tableString[MAXSQLSTRINGSIZE];
    signed long posInto = lookup::findDelimiter((char*)queryString, (char*)sqlTokenInto);
    if(posInto == NEGATIVE)
    {
        errText.append(" <p> expecting the literal 'into'");
        return ParseResult::FAILURE;
    }
    strcpy((char*)tableString,queryString+posInto+strlen((char*)sqlTokenInto)+1);
    
    list<char*> delimiterList;
    delimiterList.push_back((char*)sqlTokenValues);
    delimiterList.push_back((char*)sqlTokenOpenParen);
    long signed found       = lookup::findDelimiterFromList(tableString,delimiterList);
    
    printf("\n string:%s found:%ld",tableString,found);
    if(found < 1)
    {
        errText.append(" <p> expecting a (colonm list or a values statement");
        return ParseResult::FAILURE;
    }
    tableString[found] = '\0';


    if(parseTableList(tableString) == ParseResult::FAILURE)
        return ParseResult::FAILURE;

    queryTable = tables.front();
    if(queryTable == nullptr)
    {
        errText.append(" cannot identify query table ");
        return ParseResult::FAILURE;
    }

    // pos = abbreviation for position
    signed long posValues       =lookup::findDelimiter((char*)queryString, (char*)sqlTokenValues);
    signed long posOpenParen    =lookup::findDelimiter((char*)queryString, (char*)sqlTokenOpenParen);
    signed long posCloseParen   =lookup::findDelimiter((char*)queryString, (char*)sqlTokenCloseParen);


    if(posValues > posOpenParen)
    {
        // Signature (col,col,col) Values
        if(parseColumnList(posOpenParen,posCloseParen) == ParseResult::FAILURE)
            return ParseResult::FAILURE;
    }
    else{
        dbTable = lookup::getTableByName(sqlDB->tables,(char*)queryTable->name);
        if(dbTable == nullptr)
        {
            errText.append(" Check your table name. Cannot find ");
            errText.append(queryTable->name);
            errText.append(" in SQL def. ");
            return ParseResult::FAILURE;
        }
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

    
    char tableString[MAXSQLSTRINGSIZE];

    strcpy(tableString, (char*)queryString+strlen((char*)sqlTokenUpdate)+1);
   
    signed long posSet = lookup::findDelimiter((char*)tableString, (char*)sqlTokenSet);
     
    if(posSet < 1)
    {
        errText.append(" could not find the verb 'set' in update string");
    }
    tableString[posSet-1] = '\0';

    printf("\n 1:%s 2:%ld", tableString, posSet);
    

    if(parseTableList(tableString) == ParseResult::FAILURE)
        return ParseResult::FAILURE;

    signed long posWhere =lookup::findDelimiter((char*)queryString, (char*)sqlTokenWhere);

    if(parseColumnValue(posSet+strlen(sqlTokenSet),posWhere) == ParseResult::FAILURE)
        return ParseResult::FAILURE;
 
    //Update will only have one query table, though conditions may have more
    queryTable = tables.front(); 
    if(queryTable == nullptr)
    {
        errText.append(" cannot find selected table ");
        return ParseResult::FAILURE;
    }
    
    // The template table defined by the CREATE syntax)
    dbTable = lookup::getTableByName(sqlDB->tables,(char*)queryTable->name);
    
    posWhere = posWhere + strlen(sqlTokenWhere);

    if(parseConditions(posWhere,queryStringLength) == ParseResult::FAILURE)
        return ParseResult::FAILURE;
    
    column* col;
    for(ColumnValue* colVal : columnValue)
    {
        col = dbTable->getColumn(colVal->name);
        if(col->primary)
        {
            errText.append(" ");
            errText.append(colVal->name);
            errText.append(" is a primary key, cannot update ");
            return ParseResult::FAILURE;
        }
        if(col == nullptr)
        {
            errText.append(" column not found ");
            errText.append(colVal->name);
            return ParseResult::FAILURE;
        }
        if(valueSizeOutofBounds(colVal->value,col))
            return ParseResult::FAILURE;

        col->value = colVal->value;
        queryTable->columns.push_back(col);
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
        if(strcasecmp(_token,sqlTokenLike) != 0
        && strcasecmp(_token,sqlTokenGreater) != 0
        && strcasecmp(_token,sqlTokenLessThan) != 0
        && strcasecmp(_token,sqlTokenEqual) != 0
        && strcasecmp(_token,sqlTokenNotEqual) != 0)
        {
            errText.append(_token);
            errText.append(" condition operator missing or not =, !=, >, > or like");
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

    //TODO Will eventually become a map
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

        switch(col->edit)
        {
            case t_edit::t_bool:    //do nothing
            {
                break; 
            }  
            case t_edit::t_char:    //do nothing
            {
                break; 
            }  
            case t_edit::t_date:    //do nothing
            {
                if(!utilities::isDateValid(con->value))
                    return ParseResult::FAILURE;
                con->dateValue = utilities::parseDate(con->value);
                break; 
            } 
            case t_edit::t_double:
            {
                if(!utilities::isNumeric(con->value))
                {
                    errText.append(" condition column ");
                    errText.append(con->name); 
                    errText.append("  value not numeric |");
                    errText.append(con->value);
                    errText.append("| ");
                    return ParseResult::FAILURE;
                }
                con->doubleValue = atof(con->value);
                break;
            }
            case t_edit::t_int:
            {
                if(!utilities::isNumeric(con->value))
                {
                    errText.append(" condition column ");
                    errText.append(con->name); 
                    errText.append("  value not numeric |");
                    errText.append(con->value);
                    errText.append("| ");
                    return ParseResult::FAILURE;
                }
                con->intValue = atoi(con->value);
                break;
            }
        }
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
    dbTable = lookup::getTableByName(sqlDB->tables,(char*)queryTable->name);
    if(dbTable == nullptr)
    {
        errText.append(" Could not find ");
        errText.append(queryTable->name);
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
        if(strcmp(token,sqlTokenAsterisk) == 0)
        { 
           // if (queryTable->columns.size() == 0)
            return populateQueryTable(dbTable);
        }

        splitToken st = lookup::tokenSplit(token);
         if(strlen(token) == strlen(st.one))
        {
            col = dbTable->getColumn(st.one);
        }
        else
        {
            col = dbTable->getColumn(st.two);
            col->alias = st.one;
        }
        //col = dbTable->getColumn(token);
        if(col == nullptr)
        {
            errText.append(" column not found ");
            errText.append(token);
            return ParseResult::FAILURE;
        }
        queryTable->columns.push_back(col);
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
    size_t count = 0;
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

        col = lookup::scrollColumnList(queryTable->columns,count);
        
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
ParseResult queryParser::parseTableList(char* _tableString)
{
    sTable* table;
    char* token;
    tokenParser* tp = new tokenParser();
    printf("\n table String %s ",_tableString);

    // sample template   
    // table1 t1, table2 t2
    char * split = strtok (_tableString,",");
    while (split != NULL)
    {
        table = new sTable();
        tp->parse(split);
        token = tp->getToken(); 
        sTable* t1 = lookup::getTableByName(sqlDB->tables,token); 
        
        if(t1 != nullptr)
            table->name = token;

        token = tp->getToken();
        if(token == nullptr)
        {
            printf("\n table name %s ",table->name);
            printf(" alias %s ",table->alias);
            tables.push_back(table);
            break;
        }
        //case: no alias -- from customer where
        if (strlen(token) == 0)
        {
            printf("\n table name %s ",table->name);
            printf(" alias %s ",table->alias);
            tables.push_back(table);
            split = strtok (NULL, ",");
            continue;
        } 

        //case: alias -- from customer c where
        sTable* t2 = lookup::getTableByName(sqlDB->tables,token);
        if(t1 != nullptr
        && t2 == nullptr)
        {
            table->alias = token;
            printf("\n table name %s ",table->name);
            printf(" alias %s ",table->alias);
            tables.push_back(table);
            split = strtok (NULL, ",");
            continue;
        }
        
        return ParseResult::FAILURE;    
    }


    if(tables.size() == 0)
    {
        errText.append(" expecting at least one table");
        return ParseResult::FAILURE;
    }
    return ParseResult::SUCCESS;
}

/******************************************************
 * Populate Query Table
 ******************************************************/
ParseResult queryParser::populateQueryTable(sTable* _table)
{

    for (column* col : _table->columns) 
    {
        queryTable->columns.push_back(col);
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
        errText.append(" : value is null");

        return true;
    }

    if(col == nullptr)
    {
        errText.append(" cannot find column for ");
        errText.append(value);
        return true;
    }

    if(strlen(value) > (size_t)col->length
    && col->edit == t_edit::t_char)
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


