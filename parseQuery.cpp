#pragma once
#include <string>
#include <list>
#include <vector>
#include <algorithm>
#include "interfaces.h"
#include "sqlCommon.h"
#include "tokenParser.cpp"
#include "parseSQL.cpp"
#include "utilities.cpp"
#include "lookup.cpp"
#include "parseClause.cpp"


using namespace std;

/******************************************************
 * 
 * This class provide raw syntax checking only.
 * 
 * Expect no table, column or data validation.
 * 
 ******************************************************/

class ParseQuery
{
    const char*             queryString;
    signed int              queryStringLength   = 0;
    bool join               = true;
    bool notJoin            = false;

    public:

    iElements*              iElement = new iElements();

    Condition*              condition       = nullptr;
    int                     rowsToReturn    = 0;
    bool                    isColumn        = false;
    bool                    isCondition     = false;
    SQLACTION               sqlAction       = SQLACTION::NOACTION;

    ParseResult             parse(const char*);
    ParseResult             parseSelect();
    ParseResult             parseInsert();
    ParseResult             parseUpdate();
    ParseResult             parseDelete();
    ParseResult             parseTableList(char*);
    ParseResult             parseColumnNameValueList(char*);
    ParseResult             parseColumnList(char*);
    columnParts*            parseColumnName(char*);
    ParseResult             parseValueList(char*);
    ParseResult             parseOrderByList(char*);
    ParseResult             parseGroupByList(char*);
    ParseResult             parseConditionList(char*, bool);
    ParseResult             addCondition(char*,bool);
    void                    clear();
};
/******************************************************
 * Clear
 ******************************************************/
void ParseQuery::clear()
{
    iElement->clear();
}

/******************************************************
 * Parse
 ******************************************************/
ParseResult ParseQuery::parse(const char* _queryString)
{
    if(debug)
        fprintf(traceFile,"\n\n-------------------------BEGIN QUERY PARSE-------------------------------------------");
   
    tokenParser* tok = new tokenParser();
    queryString       = tok->cleanString((char*)_queryString);

    iElement = new iElements();
    
    if(debug)
     fprintf(traceFile,"\n query=%s",queryString);;
    
    queryStringLength = (signed int)strlen(queryString);
    
    tok->parse(queryString);
    
    char* token     = tok->getToken();
    if(token == nullptr)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Null returned where ACTION should be.");
        return ParseResult::FAILURE;
    }

    sqlAction = lookup::determineAction(token);
    
    delete tok;
    free(token);
    
    if(sqlAction == SQLACTION::INVALID)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Cannot determine action: select, insert, update?");
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
            return parseDelete();
            break;
        case SQLACTION::JOIN:
            return parseSelect();
            break;
        case SQLACTION::INNER:
            return parseSelect();
            break;
        case SQLACTION::OUTER:
            return parseSelect();
            break;
        case SQLACTION::LEFT:
            return parseSelect();
            break;        
        case SQLACTION::RIGHT:
            return parseSelect();
            break;        
        case SQLACTION::NATURAL:
            return parseSelect();
            break;
        case SQLACTION::FULL:
            return parseSelect();
            break;
        case SQLACTION::CROSS:
            return parseSelect();
            break;
    }

    return ParseResult::FAILURE;

}
/******************************************************
 * Parse Select
 ******************************************************/
ParseResult ParseQuery::parseSelect()
{
    
    if(debug)
    {
        fprintf(traceFile,"\n\n-------------------------BEGIN PROCESS SELECT-------------------------------------------");
        fprintf(traceFile,"\nQuery String = %s",queryString);
    }

    ParseClause* parseClause = new ParseClause();
    parseClause->parseSelect((char*)queryString);
    iClauses* iclause = parseClause->iClause;

    iElement = new iElements();
    iElement->sqlAction = sqlAction;
    iElement->rowsToReturn = iclause->topRows;
     
    ParseResult errorState = parseTableList(iclause->strTables);

    if(errorState == ParseResult::SUCCESS)
        errorState = parseColumnList(iclause->strColumns);
    
    if(errorState == ParseResult::SUCCESS) 
        errorState = parseConditionList(iclause->strConditions,notJoin);

    if(errorState == ParseResult::SUCCESS) 
        errorState = parseConditionList(iclause->strJoinConditions,join);

    if(errorState == ParseResult::SUCCESS) 
        errorState = parseOrderByList(iclause->strOrderBy);

    if(errorState == ParseResult::SUCCESS) 
        errorState = parseGroupByList(iclause->strGroupBy);

    if(iclause->topRows > 0)
        rowsToReturn = iclause->topRows;

    if(errorState == ParseResult::FAILURE) 
    {
        delete iclause;
        delete iElement;
        return ParseResult::FAILURE;
    }

    return ParseResult::SUCCESS;

}
/******************************************************
 * Parse Insert
 ******************************************************/
ParseResult ParseQuery::parseInsert()
{

    iElement = new iElements();
    iElement->sqlAction = sqlAction;
    //----------------------------------------------------------
    // Create table list
    //----------------------------------------------------------

    signed int startTable = lookup::findDelimiter((char*)queryString, (char*)sqlTokenInto);
    if(startTable == NEGATIVE
    || startTable == DELIMITERERR)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Expecting the literal 'into'");
        return ParseResult::FAILURE;
    }

    char* strTableList = dupString(queryString+startTable+strlen((char*)sqlTokenInto)+1);
    
    // Distinguishing between  Insert into theTable (..column list..) and Insert into theTable Values(..value list list..)
    signed int posValues       =lookup::findDelimiter(strTableList, (char*)sqlTokenValues);
    signed int posOpenParen    =lookup::findDelimiter(strTableList, (char*)sqlTokenOpenParen);

    if(posValues    == DELIMITERERR
    || posOpenParen == DELIMITERERR)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Expecting a colonm list or a values statement");
        return ParseResult::FAILURE;
    }

    signed int endTable = 0;

    if(posOpenParen < posValues)
    {
        endTable = posOpenParen;
    }
    else
        endTable = posValues;
    
    if(debug)
    {
        fprintf(traceFile,"\n posOpenParen %d",posOpenParen);
        fprintf(traceFile,"\n posValues %d",posValues);
        fprintf(traceFile,"\n endTable %d",endTable);
    }
    
    //Terminate table list
    strTableList[endTable] = '\0';

    if(parseTableList(strTableList) == ParseResult::FAILURE)
        return ParseResult::FAILURE;

    signed int posCloseParen = 0;

    posValues       =lookup::findDelimiter((char*)queryString, (char*)sqlTokenValues);
    posOpenParen    =lookup::findDelimiter((char*)queryString, (char*)sqlTokenOpenParen);

    if(posValues > posOpenParen)
    {
        char* strColumnList = dupString(queryString+posOpenParen+1);
        posCloseParen   =lookup::findDelimiter(strColumnList, (char*)sqlTokenCloseParen);
        strColumnList[posCloseParen] = '\0';
        if(parseColumnList(strColumnList) == ParseResult::FAILURE)
            return ParseResult::FAILURE;
    }

    //----------------------------------------------------------
    // Create value list
    //----------------------------------------------------------
    posValues       =lookup::findDelimiter((char*)queryString, (char*)sqlTokenValues);
    if(posValues == NEGATIVE
    || posValues == DELIMITERERR)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Expecting a colonm list or a values statement");
        return ParseResult::FAILURE;
    }

    char* strValueList = dupString(queryString+posValues+strlen((char*)sqlTokenValues)+1);

    //At this point value list will be enclosed in parenthesis - trim them

    //trim closing parenthesis
    posCloseParen    =lookup::findDelimiter(strValueList, (char*)sqlTokenCloseParen);
    strValueList[posCloseParen] = '\0';

    //trim open parenthesis
    posOpenParen    =lookup::findDelimiter(strValueList, (char*)sqlTokenOpenParen);
    if(posOpenParen == NEGATIVE
    || posOpenParen == DELIMITERERR)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Expecting a colonm list or a values statement");
        return ParseResult::FAILURE;
    }
    
    char* strValues = dupString(strValueList+posOpenParen+1);
    
    free(strValueList);

    if(parseValueList(strValues) == ParseResult::FAILURE)
        return ParseResult::FAILURE;

    return ParseResult::SUCCESS;
}
/******************************************************
 * Parse Update
 ******************************************************/
ParseResult ParseQuery::parseUpdate()
{

    iElement = new iElements();
    iElement->sqlAction = sqlAction;
    //---------------------------------------------------------
    // Create table list
    //---------------------------------------------------------
    char* strTableList = dupString(queryString+strlen((char*)sqlTokenUpdate)+1);
   
    signed int posSet = lookup::findDelimiter(strTableList, (char*)sqlTokenSet);
     
    if(posSet < 1)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Could not find the token 'set' in string");
        return ParseResult::FAILURE;
    }

    //Terminate table list
    strTableList[posSet-1] = '\0';
    
    if(parseTableList(strTableList) == ParseResult::FAILURE)
        return ParseResult::FAILURE;

    //---------------------------------------------------------
    // Create column/value list
    //---------------------------------------------------------
    
    //reset position of verb set
    posSet = lookup::findDelimiter((char*)queryString, (char*)sqlTokenSet);

    //create string
    char* strColumnValues = dupString(queryString+posSet+strlen((char*)sqlTokenSet) +1);

    signed int posEndOfColumnValues =lookup::findDelimiter(strColumnValues, (char*)sqlTokenWhere);

    if(posEndOfColumnValues != NEGATIVE)
        strColumnValues[posEndOfColumnValues] = '\0';

    
    if(parseColumnNameValueList(strColumnValues) == ParseResult::FAILURE)
        return ParseResult::FAILURE;

    //---------------------------------------------------------
    // Create condition list
    //---------------------------------------------------------
    
    //find where clause in the original query string
    signed int posWhere =lookup::findDelimiter((char*)queryString, (char*)sqlTokenWhere);
    if(posWhere == DELIMITERERR)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,true,"delimiter error finding the token 'where' in string");
        return ParseResult::FAILURE;
    }

    if(posWhere == NEGATIVE)
        return ParseResult::SUCCESS;

    char* strConditions = dupString(queryString+posWhere + strlen((char*)sqlTokenWhere));

    if(parseConditionList(strConditions,notJoin) == ParseResult::FAILURE)
        return ParseResult::FAILURE;

    return ParseResult::SUCCESS;
}
/******************************************************
 * Parse Column List (for Update)
 ******************************************************/
ParseResult ParseQuery::parseColumnNameValueList(char* _workingString)
{
    if(debug)
    {
        fprintf(traceFile,"\n\n-----------Column Value List----------------");
        fprintf(traceFile,"\ntableString:%s",_workingString);
    }

    if(_workingString == nullptr)
        return ParseResult::SUCCESS;

    char* token;
    bool  isColumnFlag = true;
    columnParts* parts{};
    tokenParser* tok = new tokenParser(_workingString);
 
    while(!tok->eof)
    {
        token = tok->getToken();
        if(tok->eof)
            break;

        if(tok == nullptr)
            break;

        if(strcmp(token,sqlTokenEqual) == 0)
        {
            continue;
        }

        if(isColumnFlag)
        {
            isColumnFlag = false;
            parts = parseColumnName(token);
            continue;
        }

        isColumnFlag = true;
        parts->value = token;
        iElement->lstColumns.push_back(parts);
    }

    return ParseResult::SUCCESS;
}
/******************************************************
 * Parse Delete
 ******************************************************/
ParseResult ParseQuery::parseDelete()
{
    //----------------------------------------------------------
    // Parse Table List
    //----------------------------------------------------------
    signed int posFrom     = lookup::findDelimiter((char*)queryString, (char*)sqlTokenFrom);
    
    if(debug)
        fprintf(traceFile,"\nQuery string after top %s",queryString);
    
    char* strTableList = dupString(queryString+posFrom+1+strlen(sqlTokenFrom));

    signed int posWhere =lookup::findDelimiter(strTableList, (char*)sqlTokenWhere);

    if(posWhere == NEGATIVE)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,true,"A 'where' statement is required for a delete");
        return ParseResult::FAILURE;
    }
    
    strTableList[posWhere] = '\0';
    if(parseTableList(strTableList) == ParseResult::FAILURE)
        return ParseResult::FAILURE;

    //--------------------------------------------------------
    // Mark delete column
    //---------------------------------------------------------
    columnParts* parts = new columnParts();
    parts->columnName = dupString("deleted");
    parts->value = dupString("T");
    iElement->lstColumns.push_back(parts);
        
    //---------------------------------------------------------
    // Create condition list
    //---------------------------------------------------------
    
    //find where clause in the original query string
    posWhere =lookup::findDelimiter((char*)queryString, (char*)sqlTokenWhere);

    if(posWhere == NEGATIVE)
        return ParseResult::SUCCESS;

    char* strConditions = dupString(queryString+posWhere + strlen((char*)sqlTokenWhere));

    if(parseConditionList(strConditions,notJoin) == ParseResult::FAILURE)
        return ParseResult::FAILURE;

    return ParseResult::SUCCESS;
}

/******************************************************
 * Add Condition
 ******************************************************/
ParseResult ParseQuery::addCondition(char* _token, bool isJoin)
{
    /* First sprint: Looking for 4 things:
        A column name   - text not enclosed in quotes
        A value         - enclosed in quotes
        A numeric       - no quotes, but numeric
        An operator     - Math operators =, <>, >, <, >=, <=

        Initially only looking for column, opeator, value in quotes
    */

   if(debug)
     fprintf(traceFile,"\ntoken=%s",_token);
    
    if(_token == nullptr)
        return ParseResult::SUCCESS;

    //--------------------------------------------------------
    // Initialize condition and condition predicates
    //-------------------------------------------------------
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

    //--------------------------------------------------------
    // Column name
    //-------------------------------------------------------

    //If name is null, it assumed to be a column name
    if(condition->name == nullptr)
    {
        condition->name = parseColumnName(_token);
        return ParseResult::SUCCESS;
    }

    //--------------------------------------------------------
    // Operation
    //-------------------------------------------------------
    if(condition->op == nullptr)
    {
        if(strcasecmp(_token,sqlTokenLike) != 0
        && strcasecmp(_token,sqlTokenGreater) != 0
        && strcasecmp(_token,sqlTokenLessThan) != 0
        && strcasecmp(_token,sqlTokenLessOrEqual) != 0
        && strcasecmp(_token,sqlTokenGreaterOrEqual) != 0
        && strcasecmp(_token,sqlTokenEqual) != 0
        && strcasecmp(_token,sqlTokenNotEqual) != 0)
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Condition operator missing. See ");
            sendMessage(MESSAGETYPE::ERROR,presentationType,false,_token);
            return ParseResult::FAILURE;
        }
        condition->op = _token;
        return ParseResult::SUCCESS;
    }

    //--------------------------------------------------------
    // Value or column to be compared to
    //-------------------------------------------------------
    // case 1: token  = "value"
    // case 2: token  = 99.9
    // case 3: token  in (1,1,2,3,4,5)
    // case 4: token  = alias.column
    // case 5: token  = column
    if(condition->value == nullptr)
    {
        //case 1
        if(lookup::findDelimiter(_token,(char*)sqlTokenQuote) != NEGATIVE)
        {
            stripQuotesFromToken(_token);
            rTrim(_token);
            condition->value = _token;
        }
        else
        //case 2
        if(isNumeric(_token))
        {
            condition->value = _token;
        }
        else
        //case 3
        if(lookup::findDelimiter(_token,(char*)sqlTokenOpenParen) != NEGATIVE)
        {
            //TODO processList();
        }
        else
            condition->compareToName = parseColumnName(_token);

        if(isJoin)
            iElement->lstJoinConditions.push_back(condition);
        else
            iElement->lstConditions.push_back(condition);

        condition = new Condition();
        return ParseResult::SUCCESS;
    }
    return ParseResult::FAILURE;
}
/******************************************************
 * Parse Order
 ******************************************************/
ParseResult ParseQuery::parseOrderByList(char* _workingString)
{
    if(debug)
    {
        fprintf(traceFile,"\n\n-----------Parse Order by----------------");
        fprintf(traceFile,"\nOrderBy string:%s",_workingString);
    }

    if(_workingString == nullptr)
        return ParseResult::SUCCESS;

    char* token;
    tokenParser*    tok = new tokenParser();
    OrderBy*        orderBy = new OrderBy();
    tok->parse(_workingString,true);

    while(!tok->eof)
    {
        token = tok->getToken();
        if(token != nullptr)
        {
            if(debug)
                fprintf(traceFile,"\n order by tokens %s", token);
                
            if(strcasecmp(token,(char*)sqlTokenOrderAcending) != 0
            && strcasecmp(token,(char*)sqlTokenOrderDescending) != 0)
            {
                OrderOrGroup    order;
                order.name      = parseColumnName(token);
                orderBy->order.push_back(order);
            }
            else
            {
                if(strcasecmp(token,(char*)sqlTokenOrderDescending) == 0)
                    orderBy->asc = false;
            }
        }
    }
    if(orderBy->order.size() > 0)
        iElement->orderBy = orderBy;
    else    
        iElement->orderBy = nullptr;
    return ParseResult::SUCCESS;
}
/******************************************************
 * Parse Order
 ******************************************************/
ParseResult ParseQuery::parseGroupByList(char* _workingString)
{
    if(debug)
    {
        fprintf(traceFile,"\n\n-----------Parse Group by----------------");
        fprintf(traceFile,"\nGroup By string:%s",_workingString);
    }

    if(_workingString == nullptr)
        return ParseResult::SUCCESS;
    
    char* token;
    tokenParser* tok = new tokenParser();
    GroupBy* groupBy = new GroupBy();
    tok->parse(_workingString,true);

    while(!tok->eof)
    {
        token = tok->getToken();
        if(token != nullptr)
        {
            if(debug)
                fprintf(traceFile,"\n order by tokens %s", token);
                
            OrderOrGroup    order;
            order.name      = parseColumnName(token);
            groupBy->group.push_back(order);
        }
    }
    if(groupBy->group.size() > 0)
        iElement->groupBy = groupBy;
    else    
        iElement->groupBy = nullptr;

    return ParseResult::SUCCESS;
}
/******************************************************
 * Parse Conditions
 ******************************************************/
ParseResult ParseQuery::parseConditionList(char* _workingString,bool _isJoin)
{
    if(debug)
    {
        fprintf(traceFile,"\n\n-----------Parse Conditions----------------");
        if(_isJoin)
            fprintf(traceFile,"\nJoin Condition:%s",_workingString);
        else
            fprintf(traceFile,"\nCondition:%s",_workingString);
    }
    if(_workingString == nullptr)
        return ParseResult::SUCCESS;

    char* token;
    tokenParser* tok = new tokenParser();
    tok->parse(_workingString,true);
    
    while(!tok->eof)
    {
        token = tok->getToken();
        if(token != nullptr)
            if(addCondition(token,_isJoin) == ParseResult::FAILURE)
                return ParseResult::FAILURE;
    }
    return ParseResult::SUCCESS;
}
/******************************************************
 * Parse Column List
 ******************************************************/
ParseResult ParseQuery::parseColumnList(char* _workingString)
{
    if(debug)
    {
        fprintf(traceFile,"\n\n-----------Parse Column List----------------");
        fprintf(traceFile,"\nColumn string:%s",_workingString);
    }

    if(_workingString == nullptr)
        return ParseResult::SUCCESS;

    char * token;
    token = strtok (_workingString,",");
    while (token != NULL)
    {
        lTrim(token,' ');
        iElement->lstColumns.push_back(parseColumnName(token));
        token = strtok (NULL, ",");
    }

    return ParseResult::SUCCESS;
}
/******************************************************
 * Parse Column Name
 ******************************************************/
columnParts* ParseQuery::parseColumnName(char* _str)
{
    /*
        Column name have the following forms

        case 1) column alias        - surname AS last
        case 2) functions           - COUNT(), SUM(), AVG(), MAX(), MIN()
        case 3) simple name         - surname
        case 4) table/column        - customers.surname
        case 5) table alias column  - c.surname from customers c
    */
    columnParts* parts = new columnParts();
    parts->fullName = dupString(_str);

    signed int columnAliasPosition = lookup::findDelimiter(_str,(char*)sqlTokenAs);
    if(columnAliasPosition == DELIMITERERR)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Failure to resolve column name");
        sendMessage(MESSAGETYPE::ERROR,presentationType,false,_str);
        return nullptr;
    }

    //case 1) column alias        - surname AS last
    char* columnName  = _str;
    if(columnAliasPosition > NEGATIVE)
    {
        parts->columnAlias = dupString(_str+columnAliasPosition+3);
        _str[columnAliasPosition] = '\0';
    }

    // case 2) functions          - COUNT(), SUM(), AVG(), MAX(), MIN()
    signed int posParen = lookup::findDelimiter(_str,(char*)sqlTokenOpenParen);
    if(posParen > NEGATIVE)
    {
        parts->fuction = dupString(_str);
        parts->fuction[posParen-1] = '\0';
    }
    
    char* colStr = dupString(_str+posParen+1); 
    int ii = 0;
    for(size_t i=0;i<strlen(_str);i++)
    {
        if(colStr[i] != CLOSEPAREN)
        {
            colStr[ii] = colStr[i];
            ii++;
        }
    }
    ii++;
    
    colStr[ii] = '\0';

    TokenPair* tp = lookup::tokenSplit(colStr,(char*)sqlTokenPeriod);
    if(tp == nullptr)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Failure to resolve column name: ");
        sendMessage(MESSAGETYPE::ERROR,presentationType,false,columnName);
        return nullptr; 
    }

    // case 3) simple name         - surname
    if(tp->two == nullptr)
    {
        parts->columnName = tp->one;
    }
    else
    {
    // case 4/5)
        parts->tableAlias = tp->one;
        parts->columnName = tp->two;
    }

    delete tp;

    if(debug)
    {
        fprintf(traceFile,"\n Column parts: function:%s name:%s alias:%s table Alias %s",parts->fuction, parts->columnName, parts->columnAlias, parts->tableAlias);
    }

    return parts;
};
/******************************************************
 * Parse Value List
 ******************************************************/
ParseResult ParseQuery::parseValueList(char* _workingString)
{
    if(debug)
    {
        fprintf(traceFile,"\n\n-----------Parse Value List----------------");
        fprintf(traceFile,"\ntableString:%s",_workingString);
    }
    
    if(_workingString == nullptr)
        return ParseResult::SUCCESS;
   
    char* token;
    tokenParser* tok = new tokenParser(_workingString);
    while(!tok->eof)
    {
        token = tok->getToken();
        if(token != nullptr)
            iElement->lstValues.push_back(token);
    }

    return ParseResult::SUCCESS;
}
/******************************************************
 * Parse Table List
 ******************************************************/
ParseResult ParseQuery::parseTableList(char* _workingString)
{
    if(debug)
    {
        fprintf(traceFile,"\n\n-----------Parse Table List----------------");
        fprintf(traceFile,"\ntableString:%s",_workingString);
    }

    if(_workingString == nullptr)
        return ParseResult::SUCCESS;

    char* token;

    token = strtok (_workingString,",");
	while(token != NULL)
    {
        if(debug)
            fprintf(traceFile,"\ntoken %s",token);
        if(iElement->tableName != nullptr)
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Expecting only one table");
            return ParseResult::FAILURE;
        }
        iElement->tableName = dupString(token);
        token = strtok (NULL, ",");
    }

    if(iElement->tableName == nullptr)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Expecting at least one table");
        return ParseResult::FAILURE;
    }
    return ParseResult::SUCCESS;
}



