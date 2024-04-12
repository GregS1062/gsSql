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


class ColumnNameValue
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
    sqlParser*          sqlDB;

    public:

    list<char*>             lstTables;       // tables
    list<ColumnNameValue*>  lstColNameValue;// used in update to set list of column/values
    list<char*>             lstColName;
    list<char*>             lstValues;
    list<Condition*>        conditions;     // where condition (operator) value

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
    ParseResult         parseColumnNameValueList(char*);
    ParseResult         parseColumnList(char*);
    ParseResult         parseValueList(char*);
    ParseResult         parseConditions(char*);
    ParseResult         addCondition(char*);
    ParseResult         validateSQLString();
    ParseResult         populateQueryTable(sTable* table);
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
    lstTables.clear();
    lstColName.clear();
    lstColNameValue.clear();
    conditions.clear();
    lstColNameValue.clear();
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
    tokenParser* tok  = new tokenParser(_queryString);
    
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

    char workingString[MAXSQLSTRINGSIZE];

    signed long beginColumnList;

    tokenParser* tok = new tokenParser(queryString);

    char* token     = tok->getToken();  //The first token will always be "SELECT", throw away
    
    //Get next token
    token = tok->getToken();
    if(strcasecmp(token,sqlTokenTop) == 0)
    {
        token = tok->getToken();
        if(!utilities::isNumeric(token))
        {
            errText.append(" expecting numeric after top ");
            return ParseResult::FAILURE;
        }
        rowsToReturn = atoi(token);
        beginColumnList = tok->pos;
    }
    else
    {
        // Check for "top", a column was picked up - reverse
        beginColumnList = tok->pos - ((strlen(token) +1));
    }

    //start of column list
    strcpy(workingString,queryString+beginColumnList);


    //Find begining of table list
    signed long posFrom     = lookup::findDelimiter((char*)workingString, (char*)sqlTokenFrom);
  
    if(posFrom == NEGATIVE)
    {
        errText.append(" missing FROM token");
        return ParseResult::FAILURE;
    }

    list<char*> delimiterList;
    delimiterList.push_back((char*)sqlTokenWhere);
    delimiterList.push_back((char*)sqlTokenOn);
    delimiterList.push_back((char*)sqlTokenJoin);
    
    strcpy(workingString, (char*)workingString+(posFrom+1+strlen(sqlTokenFrom)));
    
    long signed found       = lookup::findDelimiterFromList(workingString,delimiterList);
    if(found > NEGATIVE)
    {
        workingString[found] = '\0';
    }

    if(parseTableList(workingString) == ParseResult::FAILURE)
        return ParseResult::FAILURE;

    if(lstTables.size() == 0)
    {
        errText.append(" no selected tables found in string ");
        return ParseResult::FAILURE;
    }

    strcpy(workingString,queryString+beginColumnList);
    if(debug)
        printf("\n working %s",workingString);

    signed long posAsterisk = lookup::findDelimiter((char*)workingString, (char*)sqlTokenAsterisk);
    posFrom = lookup::findDelimiter((char*)workingString, (char*)sqlTokenFrom);

    if(posAsterisk == NEGATIVE
    || posAsterisk > posFrom)
    {
        workingString[posFrom] = '\0';
        if(parseColumnList(workingString) == ParseResult::FAILURE)
            return ParseResult::FAILURE;
    }
    else
    {
        lstColName.push_back((char*)sqlTokenAsterisk);
    }
    
    signed long posWhere =lookup::findDelimiter((char*)queryString, (char*)sqlTokenWhere);

    if(posWhere == NEGATIVE)
        return ParseResult::SUCCESS;
    
    signed long endOfString = queryStringLength - posWhere - strlen((char*)sqlTokenWhere);

    strcpy(workingString,queryString+posWhere + strlen((char*)sqlTokenWhere) + 1);
    workingString[endOfString] = '\0';

    if(parseConditions(workingString) == ParseResult::FAILURE)
        return ParseResult::FAILURE;

    return ParseResult::SUCCESS;
}
/******************************************************
 * Parse Insert
 ******************************************************/
ParseResult queryParser::parseInsert()
{
    char workingString[MAXSQLSTRINGSIZE];
    long signed startColumn;

    signed long startTable = lookup::findDelimiter((char*)queryString, (char*)sqlTokenInto);
    if(startTable == NEGATIVE)
    {
        errText.append(" <p> expecting the literal 'into'");
        return ParseResult::FAILURE;
    }
    startTable = startTable + strlen((char*)sqlTokenInto)+1;
    strcpy((char*)workingString,queryString+startTable);
    
    list<char*> delimiterList;
    delimiterList.push_back((char*)sqlTokenValues);
    delimiterList.push_back((char*)sqlTokenOpenParen);

    long signed endTable = lookup::findDelimiterFromList(workingString,delimiterList);
    
    if(endTable < 1)
    {
        errText.append(" <p> expecting a (colonm list or a values statement");
        return ParseResult::FAILURE;
    }
    workingString[endTable] = '\0';

    if(parseTableList(workingString) == ParseResult::FAILURE)
        return ParseResult::FAILURE;

    // add table end to table start 
    startColumn = startTable + endTable;
    
    // Get column list
    strcpy(workingString,(char*)queryString+startColumn);

    signed long posValues       =lookup::findDelimiter((char*)workingString, (char*)sqlTokenValues);
    signed long posOpenParen    =lookup::findDelimiter((char*)workingString, (char*)sqlTokenOpenParen);
    signed long posCloseParen   =lookup::findDelimiter((char*)workingString, (char*)sqlTokenCloseParen);

    if(posValues > posOpenParen)
    {
        strcpy(workingString, workingString+posOpenParen+1);
        workingString[posCloseParen-1] = '\0';
        if(parseColumnList(workingString) == ParseResult::FAILURE)
            return ParseResult::FAILURE;
    }

    posValues       =lookup::findDelimiter((char*)queryString, (char*)sqlTokenValues);
    memset(&workingString,' ',MAXSQLSTRINGSIZE-1);
    strcpy(workingString,queryString+posValues+strlen((char*)sqlTokenValues)+1);

    posCloseParen    =lookup::findDelimiter((char*)workingString, (char*)sqlTokenCloseParen);
    workingString[posCloseParen-1] = '\0';
    
    printf("\n working string after token values %s",workingString);
    posOpenParen    =lookup::findDelimiter((char*)workingString, (char*)sqlTokenOpenParen);
    printf("\n working string search for ( %s",workingString);
    
    strcpy(workingString,workingString+posOpenParen+1);
     
     printf("\n working string after open paren %s",workingString);

    if(parseValueList(workingString) == ParseResult::FAILURE)
        return ParseResult::FAILURE;

    return ParseResult::SUCCESS;
}
/******************************************************
 * Parse Update
 ******************************************************/
ParseResult queryParser::parseUpdate()
{

    char workingString[MAXSQLSTRINGSIZE];

    strcpy(workingString, (char*)queryString+strlen((char*)sqlTokenUpdate)+1);
   
    signed long posSet = lookup::findDelimiter((char*)workingString, (char*)sqlTokenSet);
     
    if(posSet < 1)
    {
        errText.append(" could not find the verb 'set' in update string");
    }
    workingString[posSet-1] = '\0';
    
    if(parseTableList(workingString) == ParseResult::FAILURE)
        return ParseResult::FAILURE;

    //reset position of verb set
    posSet = lookup::findDelimiter((char*)queryString, (char*)sqlTokenSet);
    strcpy(workingString,queryString+posSet+strlen((char*)sqlTokenSet) +1);

    signed long endOfColumnValues =lookup::findDelimiter((char*)workingString, (char*)sqlTokenWhere);

    if(endOfColumnValues == NEGATIVE)
        endOfColumnValues = strlen(workingString);
    
    workingString[endOfColumnValues] = '\0';
    if(parseColumnNameValueList(workingString) == ParseResult::FAILURE)
        return ParseResult::FAILURE;
    
    signed long posWhere =lookup::findDelimiter((char*)queryString, (char*)sqlTokenWhere);

    if(posWhere == NEGATIVE)
        return ParseResult::SUCCESS;
    signed long endOfString = queryStringLength - posWhere - strlen((char*)sqlTokenWhere);

    strcpy(workingString,queryString+posWhere + strlen((char*)sqlTokenWhere) + 1);
    workingString[endOfString] = '\0';

    if(parseConditions(workingString) == ParseResult::FAILURE)
        return ParseResult::FAILURE;

    return ParseResult::SUCCESS;
}
/******************************************************
 * Parse Column List (for Update)
 ******************************************************/
ParseResult queryParser::parseColumnNameValueList(char* workingString)
{
    char* token;
    bool  isColumnFlag = true;
    ColumnNameValue* colVal = new ColumnNameValue();
    tokenParser* tok = new tokenParser(workingString);

    if(debug)
        printf("\n working string %s",workingString);
    while(!tok->eof)
    {
        token = tok->getToken();
        if(tok->eof)
            break;

        if(strcmp(token,sqlTokenEqual) == 0)
        {
            continue;
        }
        if(isColumnFlag)
        {
            isColumnFlag = false;
            colVal = new ColumnNameValue();
            colVal->name = token;
            continue;
        }
        isColumnFlag = true;
        colVal->value = token;
        lstColNameValue.push_back(colVal);
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
ParseResult queryParser::parseConditions(char* _workString)
{
    char* token;
    tokenParser* tok = new tokenParser(_workString);

    while(!tok->eof)
    {
        token = tok->getToken();
        if(tok->eof)
            break;
        if(addCondition(token) == ParseResult::FAILURE)
            return ParseResult::FAILURE;
    }
    return ParseResult::SUCCESS;
}
/******************************************************
 * Parse Column List
 ******************************************************/
ParseResult queryParser::parseColumnList(char* _workingString)
{
    // sample template   
    // t.col1, t.col2
    char* token;
    tokenParser* tok = new tokenParser(_workingString);
    
    while(!tok->eof)
    {
        token = tok->getToken();
        lstColName.push_back(token);
    }

    return ParseResult::SUCCESS;
}
/******************************************************
 * Parse Value List
 ******************************************************/
ParseResult queryParser::parseValueList(char* _workingString)
{
    char* token;
    tokenParser* tok = new tokenParser(_workingString);
    printf("\n working string %s",_workingString);
    while(!tok->eof)
    {
        token = tok->getToken();
        if(token != nullptr)
            lstValues.push_back(token);
    }

    return ParseResult::SUCCESS;
}
/******************************************************
 * Parse Table List
 ******************************************************/
ParseResult queryParser::parseTableList(char* _tableString)
{
    char* token;
    char* retToken;
    size_t len;
    if(debug)
        printf("\n tableString %s \n",_tableString);
    token = strtok (_tableString,",");
	while(token != NULL)
    {
        if(debug)
            printf("\n token %s \n",token);
        len = strlen(token);
        retToken = (char*)malloc(len+1);
        strcpy(retToken,token);
        retToken[len] = '\0';
        lstTables.push_back(retToken);
        token = strtok (NULL, ",");
    }
    // sample template   
    // table1 t1, table2 t2
   /*  while(!tok->eof)
    {
        token = tok->getToken();
        if(tok->eof)
            break;
        lstTables.push_back(token);
    } */
    if(lstTables.size() == 0)
    {
        errText.append(" expecting at least one table");
        return ParseResult::FAILURE;
    }
    return ParseResult::SUCCESS;
}



