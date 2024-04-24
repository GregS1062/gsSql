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
    const char*             queryString;
    signed long             queryStringLength   = 0;
    sqlParser*              sqlDB;

    public:

    list<char*>             lstTables;       // tables
    list<ColumnNameValue*>  lstColNameValue;// used in update to set list of column/values
    list<char*>             lstColName;
    list<char*>             lstValues;
    list<OrderBy*>          lstOrder;
    list<char*>             lstGroup;
    list<Condition*>        conditions;     // where condition (operator) value

    Condition*              condition       = nullptr;

    int                     rowsToReturn    = 0;
    bool                    isColumn        = false;
    bool                    isCondition     = false;
    SQLACTION               sqlAction       = SQLACTION::NOACTION;

    ParseResult             clear();
    ParseResult             parse(const char*,sqlParser*);
    ParseResult             parseSelect();
    ParseResult             parseInsert();
    ParseResult             parseUpdate();
    ParseResult             parseDelete();
    ParseResult             parseTableList(char*);
    ParseResult             parseColumnNameValueList(char*);
    ParseResult             parseColumnList(char*);
    ParseResult             parseValueList(char*);
    ParseResult             parseOrder(char*);
    ParseResult             parseGroup(char*);
    ParseResult             parseConditions(char*);
    ParseResult             addCondition(char*);
    ParseResult             validateSQLString();
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
    if(debug)
        fprintf(traceFile,"\n\n-------------------------BEGIN QUERY PARSE-------------------------------------------");
   
    tokenParser* tok = new tokenParser();
    queryString       = tok->cleanString((char*)_queryString);
    
    if(debug)
     fprintf(traceFile,"\n query=%s",queryString);
;
    queryStringLength = strlen(queryString);
    sqlDB             = _sqlDB;
    
    tok->parse(queryString);
    
    char* token     = tok->getToken();
    if(token == nullptr)
    {
        utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Null returned where ACTION should be.");
        return ParseResult::FAILURE;
    }

    if(validateSQLString() == ParseResult::FAILURE)
        return ParseResult::FAILURE;

    sqlAction = lookup::determineAction(token);
    if(sqlAction == SQLACTION::INVALID)
    {
        utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Cannot determine action: select, insert, update?");
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
    }

    return ParseResult::FAILURE;

}
/******************************************************
 * Validate SQL String
 ******************************************************/
ParseResult queryParser::validateSQLString()
{

    if(debug)
      fprintf(traceFile,"\n validateSQLString");

    string sql;
    sql.append(queryString);

    // Do open and close parenthesis match?
    if(std::count(sql.begin(), sql.end(), '(')
    != std::count(sql.begin(), sql.end(), ')'))
    {
        utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Syntax error: mismatch of parenthesis");
        return ParseResult::FAILURE;
    }

    //Do quotes match?
    bool even = std::count(sql.begin(), sql.end(), '"') % 2 == 0;
    if(!even)
    {
        utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Syntax error: too many or missing quotes.");
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
    {
        fprintf(traceFile,"\n\n-------------------------BEGIN PARSE SELECT-------------------------------------------");
        fprintf(traceFile,"\nQuery String = %s",queryString);
    }

    char* selectString = utilities::dupString(queryString);
    
    signed long beginColumnList;

    tokenParser* tok = new tokenParser(selectString);

    char* token     = tok->getToken();  //The first token will always be "SELECT", throw away
    
    //Get next token
    token = tok->getToken();
    if(strcasecmp(token,sqlTokenTop) == 0)
    {
        token = tok->getToken();
        if(!utilities::isNumeric(token))
        {
            utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Expecting a number after TOP.");
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

    //----------------------------------------------------------
    // Parse Table List
    //----------------------------------------------------------
    signed long posFrom     = lookup::findDelimiter(selectString, (char*)sqlTokenFrom);
    
    if(debug)
        fprintf(traceFile,"\nQuery string after top %s",queryString);
    
    char* strTableList = utilities::dupString(selectString+posFrom+1+strlen(sqlTokenFrom));
    
    list<char*> delimiterList;
    delimiterList.push_back((char*)sqlTokenWhere);
    delimiterList.push_back((char*)sqlTokenOn);
    delimiterList.push_back((char*)sqlTokenJoin);
    delimiterList.push_back((char*)sqlTokenOrderBy);
    delimiterList.push_back((char*)sqlTokenGroupBy);
    
    long signed found       = lookup::findDelimiterFromList(strTableList,delimiterList);
    
    if(debug)
        fprintf(traceFile,"\ndelimiter found:%ld", found);

    if(found > NEGATIVE)
    {
        strTableList[found] = '\0';
    }


    if(parseTableList(strTableList) == ParseResult::FAILURE)
        return ParseResult::FAILURE;

    if(lstTables.size() == 0)
    {
        utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Table not recognized.");
        return ParseResult::FAILURE;
    }

    //-----------------------------------------------------
    // Parse column list
    //-----------------------------------------------------
    //start of column list
    char* strColumnList = utilities::dupString(queryString+beginColumnList);

    //Find begining of table list
    posFrom     = lookup::findDelimiter(strColumnList, (char*)sqlTokenFrom);
  
    if(posFrom == NEGATIVE
    || posFrom == DELIMITERERR)
    {
        utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Missing FROM token");
        return ParseResult::FAILURE;
    }

    //Terminate column list
    strColumnList[posFrom-1] = '\0';

    if(parseColumnList(strColumnList) == ParseResult::FAILURE)
        return ParseResult::FAILURE;

    //----------------------------------------------------------
    // Order by / Group by
    //----------------------------------------------------------

    //Order by

    //Case 1: from table
    //Case 2: from table order by col
    //Case 3: from table group by col
    //Case 4: from table where col = "value"
    //Case 5: from table where col = "value" order by col
    //Case 6: from table where col = "value" group by col

    char* strOrder          = nullptr;
    char* strGroup          = nullptr;
    char* strConditionList  = nullptr;
    
    signed long posWhere =lookup::findDelimiter((char*)queryString, (char*)sqlTokenWhere);
    signed long posOrder =lookup::findDelimiter((char*)queryString, (char*)sqlTokenOrderBy);
    signed long posGroup =lookup::findDelimiter((char*)queryString, (char*)sqlTokenGroupBy);

    if(posOrder > 0
    && posGroup > 0)
    {
    utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Error: cannot have both 'order by' and 'group by'");
    return ParseResult::FAILURE;
    }

    if(posWhere == DELIMITERERR
    || posOrder == DELIMITERERR)
    {
    utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Error looking for 'where' or 'order by'");
    return ParseResult::FAILURE;
    }
    
    // Case 1. No conditions
    if(posWhere == NEGATIVE
    && posOrder == NEGATIVE
    && posGroup == NEGATIVE)
        return ParseResult::SUCCESS;
    
    //Case 2.
    if(posOrder > 0)
    {
        strOrder = utilities::dupString(queryString+posOrder + strlen((char*)sqlTokenOrderBy)+1);       
        if(parseOrder(strOrder) == ParseResult::FAILURE)
            return ParseResult::FAILURE;
        return ParseResult::SUCCESS;
    }

    //Case 3.
    if(posGroup > 0)
    {
        strGroup = utilities::dupString(queryString+posGroup + strlen((char*)sqlTokenGroupBy)+1);       
        if(parseGroup(strGroup) == ParseResult::FAILURE)
            return ParseResult::FAILURE;
        return ParseResult::SUCCESS;
    }

    //----------------------------------------------------------
    // Create conditions list
    //----------------------------------------------------------

    //posWhere is positive at this point
    strConditionList = utilities::dupString(queryString+posWhere + strlen((char*)sqlTokenWhere)+1);
    
    //Case 4.
    if(posOrder == NEGATIVE
    && posGroup == NEGATIVE)
    {
        if(parseConditions(strConditionList) == ParseResult::FAILURE)
            return ParseResult::FAILURE;
        return ParseResult::SUCCESS;
    }

    //Case 5.

    //Trim either order by or group by from condition list
    posOrder =lookup::findDelimiter((char*)strConditionList, (char*)sqlTokenOrderBy);
    posGroup =lookup::findDelimiter((char*)strConditionList, (char*)sqlTokenGroupBy);
    if(posOrder == DELIMITERERR
    || posGroup == DELIMITERERR)
    {
        utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Error: finding 'order by' or 'group by'");
        return ParseResult::FAILURE;
    }

    //Terminate condition list at beginning of order by
    if(posOrder > 0)
        strConditionList[posOrder] = '\0';
    
    if(posGroup > 0)
        strConditionList[posGroup] = '\0';

    if(parseConditions(strConditionList) == ParseResult::FAILURE)
        return ParseResult::FAILURE;

    return ParseResult::SUCCESS;
}
/******************************************************
 * Parse Insert
 ******************************************************/
ParseResult queryParser::parseInsert()
{

    //----------------------------------------------------------
    // Create table list
    //----------------------------------------------------------

    signed long startTable = lookup::findDelimiter((char*)queryString, (char*)sqlTokenInto);
    if(startTable == NEGATIVE
    || startTable == DELIMITERERR)
    {
        utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Expecting the literal 'into'");
        return ParseResult::FAILURE;
    }

    char* strTableList = utilities::dupString(queryString+startTable+strlen((char*)sqlTokenInto)+1);
    
    list<char*> delimiterList;
    delimiterList.push_back((char*)sqlTokenValues);
    delimiterList.push_back((char*)sqlTokenOpenParen);

    long signed endTable = lookup::findDelimiterFromList(strTableList,delimiterList);
    
    if(endTable == NEGATIVE
    || endTable == DELIMITERERR)
    {
        utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Expecting a colonm list or a values statement");
        return ParseResult::FAILURE;
    }
    
    //Terminate table list
    strTableList[endTable] = '\0';

    if(parseTableList(strTableList) == ParseResult::FAILURE)
        return ParseResult::FAILURE;

    //----------------------------------------------------------
    // Create table list
    //----------------------------------------------------------

    // Distinguishing between  Insert into theTable (..column list..) and Insert into theTable Values(..value list list..)
    signed long posValues       =lookup::findDelimiter((char*)queryString, (char*)sqlTokenValues);
    signed long posOpenParen    =lookup::findDelimiter((char*)queryString, (char*)sqlTokenOpenParen);
    signed long posCloseParen = 0;

    if(posValues    == DELIMITERERR
    || posOpenParen == DELIMITERERR)
    {
        utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Expecting a colonm list or a values statement");
        return ParseResult::FAILURE;
    }

    if(posValues > posOpenParen)
    {
        char* strColumnList = utilities::dupString(queryString+posOpenParen+1);
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
        utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Expecting a colonm list or a values statement");
        return ParseResult::FAILURE;
    }

    char* strValueList = utilities::dupString(queryString+posValues+strlen((char*)sqlTokenValues)+1);

    //At this point value list will be enclosed in parenthesis - trim them

    //trim closing parenthesis
    posCloseParen    =lookup::findDelimiter(strValueList, (char*)sqlTokenCloseParen);
    strValueList[posCloseParen] = '\0';

    //trim open parenthesis
    posOpenParen    =lookup::findDelimiter(strValueList, (char*)sqlTokenOpenParen);
    if(posOpenParen == NEGATIVE
    || posOpenParen == DELIMITERERR)
    {
        utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Expecting a colonm list or a values statement");
        return ParseResult::FAILURE;
    }
    
    char* strValues = utilities::dupString(strValueList+posOpenParen+1);
    
    free(strValueList);

    if(parseValueList(strValues) == ParseResult::FAILURE)
        return ParseResult::FAILURE;

    return ParseResult::SUCCESS;
}
/******************************************************
 * Parse Update
 ******************************************************/
ParseResult queryParser::parseUpdate()
{

    //---------------------------------------------------------
    // Create table list
    //---------------------------------------------------------
    char* strTableList = utilities::dupString(queryString+strlen((char*)sqlTokenUpdate)+1);
   
    signed long posSet = lookup::findDelimiter(strTableList, (char*)sqlTokenSet);
     
    if(posSet < 1)
    {
        utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Could not find the token 'set' in string");
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
    char* strColumnValues = utilities::dupString(queryString+posSet+strlen((char*)sqlTokenSet) +1);

    signed long posEndOfColumnValues =lookup::findDelimiter(strColumnValues, (char*)sqlTokenWhere);

    if(posEndOfColumnValues != NEGATIVE)
        strColumnValues[posEndOfColumnValues] = '\0';

    
    if(parseColumnNameValueList(strColumnValues) == ParseResult::FAILURE)
        return ParseResult::FAILURE;

    //---------------------------------------------------------
    // Create condition list
    //---------------------------------------------------------
    
    //find where clause in the original query string
    signed long posWhere =lookup::findDelimiter((char*)queryString, (char*)sqlTokenWhere);
    if(posWhere == DELIMITERERR)
    {
        utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,true,"delimiter error finding the token 'where' in string");
        return ParseResult::FAILURE;
    }

    if(posWhere == NEGATIVE)
        return ParseResult::SUCCESS;

    char* strConditions = utilities::dupString(queryString+posWhere + strlen((char*)sqlTokenWhere));

    if(parseConditions(strConditions) == ParseResult::FAILURE)
        return ParseResult::FAILURE;

    return ParseResult::SUCCESS;
}
/******************************************************
 * Parse Column List (for Update)
 ******************************************************/
ParseResult queryParser::parseColumnNameValueList(char* _workingString)
{
    if(debug)
    {
        fprintf(traceFile,"\n\n-----------Column Value List----------------");
        fprintf(traceFile,"\ntableString:%s",_workingString);
    }
    char* token;
    bool  isColumnFlag = true;
    ColumnNameValue* colVal = new ColumnNameValue();
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
 * Parse Delete
 ******************************************************/
ParseResult queryParser::parseDelete()
{

    //----------------------------------------------------------
    // Parse Table List
    //----------------------------------------------------------
    signed long posFrom     = lookup::findDelimiter((char*)queryString, (char*)sqlTokenFrom);
    
    if(debug)
        fprintf(traceFile,"\nQuery string after top %s",queryString);
    
    char* strTableList = utilities::dupString(queryString+posFrom+1+strlen(sqlTokenFrom));

    signed long posWhere =lookup::findDelimiter(strTableList, (char*)sqlTokenWhere);

    if(posWhere == NEGATIVE)
    {
        utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,true,"A 'where' statement is required for a delete");
        return ParseResult::FAILURE;
    }
    
    strTableList[posWhere] = '\0';
    if(parseTableList(strTableList) == ParseResult::FAILURE)
        return ParseResult::FAILURE;

    //--------------------------------------------------------
    // Mark delete column
    //---------------------------------------------------------
    ColumnNameValue* colVal = new ColumnNameValue();
    colVal->name = utilities::dupString("deleted");
    colVal->value = utilities::dupString("T");
    lstColNameValue.push_back(colVal);
        
    //---------------------------------------------------------
    // Create condition list
    //---------------------------------------------------------
    
    //find where clause in the original query string
    posWhere =lookup::findDelimiter((char*)queryString, (char*)sqlTokenWhere);

    if(posWhere == NEGATIVE)
        return ParseResult::SUCCESS;

    char* strConditions = utilities::dupString(queryString+posWhere + strlen((char*)sqlTokenWhere));

    if(parseConditions(strConditions) == ParseResult::FAILURE)
        return ParseResult::FAILURE;

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
        An operator     - Math operators =, <>, >, <, >=, <=

        Initially only looking for column, opeator, value in quotes
    */

   if(debug)
     fprintf(traceFile,"\ntoken=%s",_token);
    

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
        condition->name = _token;
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
            utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Condition operator missing. See ");
            utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false,_token);
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
            utilities::stripQuotesFromToken(_token);
            utilities::rTrim(_token);
            condition->value = _token;
        }
        else
        //case 2
        if(utilities::isNumeric(_token))
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
        //case 4
        if(lookup::findDelimiter(_token,(char*)sqlTokenPeriod) != NEGATIVE)
        {
            condition->compareToName = _token;
        }
        //case 5
        else
            condition->compareToName = _token;

        conditions.push_back(condition);

        condition = new Condition();
        return ParseResult::SUCCESS;
    }
    return ParseResult::FAILURE;
}
/******************************************************
 * Parse Order
 ******************************************************/
ParseResult queryParser::parseOrder(char* _workingString)
{
    if(debug)
    {
        fprintf(traceFile,"\n\n-----------Parse Order by----------------");
        fprintf(traceFile,"\nOrderBy string:%s",_workingString);
    }

    char* token;
    tokenParser* tok = new tokenParser();
    tok->parse(_workingString,true);
    OrderBy*    orderBy    {};

    bool asc = true;
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
                orderBy = new OrderBy();
                orderBy->name   = token;
                lstOrder.push_back(orderBy);
            }
            else
            {
                if(strcasecmp(token,(char*)sqlTokenOrderDescending) == 0)
                    asc = false;
            }
        }
    }

    
    for(OrderBy* order : lstOrder)
    {
        order->asc = asc;
    }
    
    return ParseResult::SUCCESS;
}
/******************************************************
 * Parse Order
 ******************************************************/
ParseResult queryParser::parseGroup(char* _workingString)
{
    if(debug)
    {
        fprintf(traceFile,"\n\n-----------Parse Group by----------------");
        fprintf(traceFile,"\nGroup By string:%s",_workingString);
    }
    
    char* token;
    tokenParser* tok = new tokenParser();
    tok->parse(_workingString,true);

    while(!tok->eof)
    {
        token = tok->getToken();
        if(token != nullptr)
        {
            if(debug)
                fprintf(traceFile,"\n order by tokens %s", token);
            lstGroup.push_back(token);
        }
    }
    return ParseResult::SUCCESS;
}
/******************************************************
 * Parse Conditions
 ******************************************************/
ParseResult queryParser::parseConditions(char* _workingString)
{
    if(debug)
    {
        fprintf(traceFile,"\n\n-----------Parse Conditions----------------");
        fprintf(traceFile,"\ntableString:%s",_workingString);
    }
    char* token;
    tokenParser* tok = new tokenParser();
    tok->parse(_workingString,true);
    
    while(!tok->eof)
    {
        token = tok->getToken();
        if(token != nullptr)
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
    if(debug)
    {
        fprintf(traceFile,"\n\n-----------Parse Column List----------------");
        fprintf(traceFile,"\ntableString:%s",_workingString);
    }
    // sample template   
    // t.col1, t.col2
    char* token;
    tokenParser* tok = new tokenParser(_workingString);
    
    while(!tok->eof)
    {
        token = tok->getToken();
        if(token != nullptr)
            lstColName.push_back(token);
    }

    return ParseResult::SUCCESS;
}
/******************************************************
 * Parse Value List
 ******************************************************/
ParseResult queryParser::parseValueList(char* _workingString)
{
    if(debug)
    {
        fprintf(traceFile,"\n\n-----------Parse Value List----------------");
        fprintf(traceFile,"\ntableString:%s",_workingString);
    }
    
    char* token;
    tokenParser* tok = new tokenParser(_workingString);
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
    if(debug)
    {
        fprintf(traceFile,"\n\n-----------Parse Table List----------------");
        fprintf(traceFile,"\ntableString:%s",_tableString);
    }

    char* token;
    char* tableName;

    token = strtok (_tableString,",");
	while(token != NULL)
    {
        if(debug)
            fprintf(traceFile,"\ntoken %s",token);

        tableName = utilities::dupString(token);
        lstTables.push_back(tableName);
        token = strtok (NULL, ",");
    }

    if(lstTables.size() == 0)
    {
        utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Expecting at least one table");
        return ParseResult::FAILURE;
    }
    return ParseResult::SUCCESS;
}



