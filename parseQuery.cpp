#pragma once
#include <string>
#include <list>
#include <vector>
#include <algorithm>
#include "defines.h"
#include "interfaces.h"
#include "sqlCommon.h"
#include "parseClause.cpp"
#include "tokenParser.cpp"


using namespace std;
/******************************************************
 * Determine Actionm
 ******************************************************/
SQLACTION determineAction(string _token)
{
    try{

        if(debug)
            fprintf(traceFile,"\ndetermine action %s",_token.c_str());

        if(strcasecmp(_token.c_str(),sqlTokenSelect) == 0)
        {
            return SQLACTION::SELECT;
        }

        if(strcasecmp(_token.c_str(),sqlTokenInsert) == 0)
        {
            return SQLACTION::INSERT;
        }
        
        if(strcasecmp(_token.c_str(),sqlTokenUpdate) == 0)
        {
            return  SQLACTION::UPDATE;
        }

        if(strcasecmp(_token.c_str(),sqlTokenDelete) == 0)
        {
            return SQLACTION::DELETE;
        }

        if(strcasecmp(_token.c_str(),sqlTokenCreate) == 0)
        {
            return  SQLACTION::CREATE;
        }


        if(strcasecmp(_token.c_str(),sqlTokenJoin) == 0)
        {
            return  SQLACTION::JOIN;
        }

        if(strcasecmp(_token.c_str(),sqlTokenInner) == 0)
        {
            return  SQLACTION::INNER;
        }

        if(strcasecmp(_token.c_str(),sqlTokenOuter) == 0)
        {
            return  SQLACTION::OUTER;
        }

        if(strcasecmp(_token.c_str(),sqlTokenLeft) == 0)
        {
            return  SQLACTION::LEFT;
        }

        if(strcasecmp(_token.c_str(),sqlTokenRight) == 0)
        {
            return  SQLACTION::RIGHT;
        }

        if(strcasecmp(_token.c_str(),sqlTokenNatural) == 0)
        {
            return  SQLACTION::NATURAL;
        }

        if(strcasecmp(_token.c_str(),sqlTokenCross) == 0)
        {
            return  SQLACTION::CROSS;
        }

        if(strcasecmp(_token.c_str(),sqlTokenFull) == 0)
        {
            return  SQLACTION::FULL;
        }


        return  SQLACTION::INVALID;
    }
    catch_and_trace
    return  SQLACTION::INVALID;

}

/******************************************************
 * 
 * This class provide raw syntax checking only.
 * 
 * Expect no table, column or data validation.
 * 
 ******************************************************/

class ParseQuery
{
    string                  queryString{};
    size_t                  queryStringLength   = 0;
    bool join               = true;
    bool notJoin            = false;
    int                     rowsToReturn    = 0;
    bool                    isColumn        = false;
    bool                    isCondition     = false;
    shared_ptr<Condition>   condition       = make_shared<Condition>();
    SQLACTION               sqlAction       = SQLACTION::NOACTION;

    ParseResult             parseSelect();
    ParseResult             parseInsert();
    ParseResult             parseUpdate();
    ParseResult             parseDelete();
    ParseResult             parseTableList(string);
    ParseResult             parseColumnNameValueList(string);
    ParseResult             parseColumnList(string);
    shared_ptr<columnParts> parseColumnName(string);
    ParseResult             parseValueList(string);
    ParseResult             parseOrderByList(string);
    ParseResult             parseGroupByList(string);
    ParseResult             parseConditionList(string, CONDITIONTYPE);
    ParseResult             addCondition(string,CONDITIONTYPE);

    public:
        shared_ptr<iElements>   ielements        = make_shared<iElements>();
        ParseResult             parse(string);
        void                    clear();
};
/******************************************************
 * Clear
 ******************************************************/
void ParseQuery::clear()
{
    try
    {
        ielements->clear();
    }
    catch_and_trace
}

/******************************************************
 * Parse
 ******************************************************/
ParseResult ParseQuery::parse(string _queryString)
{
    try
    {
        if(debug)
        {
            fprintf(traceFile,"\n***************************************************************");
            fprintf(traceFile,"\n                 BEGIN QUERY PARSE");
            fprintf(traceFile,"\n***************************************************************");
        }
    
        queryString = normalizeQuery(_queryString,MAXSQLSTRINGSIZE);
        
        if(debug)
            fprintf(traceFile,"\n query=%s",queryString.c_str());;
        
        queryString.erase(0, queryString.find_first_not_of(SPACE));
        queryStringLength = queryString.length();
        
        size_t pos = queryString.find(SPACE);
        
        if(pos == std::string::npos)
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Cannot determine query action.");
            return ParseResult::FAILURE;
        }

        sqlAction = determineAction(queryString.substr(0,pos));

        
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
    catch_and_trace
    return ParseResult::FAILURE;
}
/******************************************************
 * Parse Select
 ******************************************************/
ParseResult ParseQuery::parseSelect()
{
    try
    {
        if(debug)
        {
            fprintf(traceFile,"\n\n-------------------------BEGIN PROCESS SELECT-------------------------------------------");
            fprintf(traceFile,"\nQuery String = %s",queryString.c_str());
        }

        unique_ptr<ParseClause> parseClause = make_unique<ParseClause>();
        parseClause->process(queryString);
        
        iClauses iclause = parseClause->iClause;

        ielements->sqlAction = sqlAction;
        ielements->rowsToReturn = iclause.topRows;
        
        if(parseTableList(iclause.strTables) == ParseResult::FAILURE)
            return ParseResult::FAILURE;

        if(parseColumnList(iclause.strColumns) == ParseResult::FAILURE)
            return ParseResult::FAILURE;
        
        if(parseConditionList(iclause.strConditions,CONDITIONTYPE::SELECT) == ParseResult::FAILURE)
            return ParseResult::FAILURE;

        if(parseConditionList(iclause.strJoinConditions,CONDITIONTYPE::JOIN) == ParseResult::FAILURE)
            return ParseResult::FAILURE;

        if(parseOrderByList(iclause.strOrderBy) == ParseResult::FAILURE)
            return ParseResult::FAILURE;

        if(parseGroupByList(iclause.strGroupBy) == ParseResult::FAILURE)
            return ParseResult::FAILURE;

        if(parseConditionList(iclause.strHavingConditions,CONDITIONTYPE::HAVING) == ParseResult::FAILURE)
            return ParseResult::FAILURE;

        if(iclause.topRows > 0)
            rowsToReturn = iclause.topRows;


        return ParseResult::SUCCESS;
    }
    catch_and_trace
    return ParseResult::FAILURE;
}
/******************************************************
 * Parse Insert
 ******************************************************/
ParseResult ParseQuery::parseInsert()
{

    try
    {
        ielements->sqlAction = sqlAction;
        //----------------------------------------------------------
        // Create table list
        //----------------------------------------------------------

        size_t startTable = findKeyword(queryString,sqlTokenInto);
        if(startTable == std::string::npos)
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Expecting the literal 'into'");
            return ParseResult::FAILURE;
        }
        queryString = snipString(queryString,sqlTokenInto,startTable);
        
        // Distinguishing between  Insert into Table (..column list..) and Insert into theTable Values(..value list list..)
        size_t posValues       = findKeyword(queryString,sqlTokenValues);
        size_t posOpenParen    = findKeyword(queryString,sqlTokenOpenParen);

        if(posValues    == std::string::npos
        || posOpenParen == std::string::npos)
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Expecting a colonm list or a values statement");
            return ParseResult::FAILURE;
        }

        size_t endTable = 0;

        if(posOpenParen < posValues)
        {
            endTable = posOpenParen;
        }
        else
            endTable = posValues;
        
        //Terminate table list
        string strTableList = clipString(queryString,endTable-1);

        if(parseTableList(strTableList) == ParseResult::FAILURE)
            return ParseResult::FAILURE;

        size_t posCloseParen = 0;

        posValues       = findKeyword(queryString,sqlTokenValues);
        posOpenParen    = findKeyword(queryString,sqlTokenOpenParen);

        if(posValues > posOpenParen)
        {
            string strColumnList = snipString(queryString,posOpenParen+1);
            posCloseParen   = findKeyword(strColumnList, sqlTokenCloseParen);
            strColumnList = clipString(strColumnList,posCloseParen);
            if(parseColumnList(strColumnList) == ParseResult::FAILURE)
                return ParseResult::FAILURE;
        }

        //----------------------------------------------------------
        // Create value list
        //----------------------------------------------------------
        posValues       = findKeyword(queryString, sqlTokenValues);
        if(posValues == std::string::npos)
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Expecting a colonm list or a values statement");
            return ParseResult::FAILURE;
        }

        string strValueList = snipString(queryString,sqlTokenValues,posValues);

        //At this point value list will be enclosed in parenthesis - trim them

        //trim closing parenthesis
        posCloseParen   = findKeyword(strValueList, sqlTokenCloseParen);
        strValueList    = clipString(strValueList,posCloseParen);

        //trim open parenthesis
        posOpenParen    = findKeyword(strValueList, sqlTokenOpenParen);
        if(posOpenParen == std::string::npos)
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Expecting a colonm list or a values statement");
            return ParseResult::FAILURE;
        }
        
        string strValues = snipString(strValueList,posOpenParen+1);
        

        if(parseValueList(strValues) == ParseResult::FAILURE)
            return ParseResult::FAILURE;

        return ParseResult::SUCCESS;
    }
    catch_and_trace
    return ParseResult::FAILURE;
}
/******************************************************
 * Parse Update
 ******************************************************/
ParseResult ParseQuery::parseUpdate()
{
    try
    {
        ielements->sqlAction = sqlAction;

        //---------------------------------------------------------
        // Create table list
        //---------------------------------------------------------
        string strTableList = snipString(queryString,sqlTokenUpdate,0);
    
        size_t posSet = findKeyword(strTableList, sqlTokenSet);
        
        if(posSet == std::string::npos)
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Could not find the token 'set' in string");
            return ParseResult::FAILURE;
        }
        
        if(parseTableList(clipString(strTableList,posSet)) == ParseResult::FAILURE)
            return ParseResult::FAILURE;

        //---------------------------------------------------------
        // Create column/value list
        //---------------------------------------------------------
        
        //reset position of verb set
        posSet = findKeyword(queryString, sqlTokenSet);

        //create string
        string strColumnValues = snipString(queryString,sqlTokenSet, posSet);

        size_t posEndOfColumnValues = findKeyword(strColumnValues, sqlTokenWhere);

        if(posEndOfColumnValues != std::string::npos)
            strColumnValues = clipString(strColumnValues,posEndOfColumnValues);

        
        if(parseColumnNameValueList(strColumnValues) == ParseResult::FAILURE)
            return ParseResult::FAILURE;

        //---------------------------------------------------------
        // Create condition list
        //---------------------------------------------------------
        
        //find where clause in the original query string
        size_t posWhere = findKeyword(queryString, sqlTokenWhere);

        if(posWhere == std::string::npos)
            return ParseResult::SUCCESS;

        if(parseConditionList(snipString(queryString,sqlTokenWhere,posWhere),CONDITIONTYPE::SELECT) == ParseResult::FAILURE)
            return ParseResult::FAILURE;

        return ParseResult::SUCCESS;

    }
    catch_and_trace
    return ParseResult::FAILURE;
}
/******************************************************
 * Parse Column List (for Update)
 ******************************************************/
ParseResult ParseQuery::parseColumnNameValueList(string _workingString)
{
    try{
        if(debug)
        {
            fprintf(traceFile,"\n\n-----------Column Value List----------------");
            fprintf(traceFile,"\ntableString:%s",_workingString.c_str());
        }

        if(_workingString.empty())
            return ParseResult::SUCCESS;

        string token;
        bool  isColumnFlag = true;
        shared_ptr<columnParts>parts{};
        unique_ptr<tokenParser> tok = make_unique<tokenParser>(_workingString);
    
        while(!tok->eof)
        {
            token = tok->getToken();
            if(tok->eof)
                break;

            if(tok == nullptr)
                break;

            if(strcmp(token.c_str(),sqlTokenEqual) == 0)
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
            ielements->lstColumns.push_back(parts);
        }

        return ParseResult::SUCCESS;
    }
    catch_and_trace
    return ParseResult::FAILURE;
}
/******************************************************
 * Parse Delete
 ******************************************************/
ParseResult ParseQuery::parseDelete()
{
    try{

        if(debug)
            fprintf(traceFile,"\nQuery string %s",queryString.c_str());
        //----------------------------------------------------------
        // Parse Delete
        //----------------------------------------------------------
        size_t posFrom     = findKeyword(queryString, sqlTokenFrom);
        
        ielements->sqlAction = sqlAction;
        
        string strTableList = snipString(queryString, sqlTokenFrom, posFrom);

        size_t posWhere = findKeyword(strTableList, sqlTokenWhere);

        if(posWhere == std::string::npos)
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"A 'where' statement is required for a delete");
            return ParseResult::FAILURE;
        }
        
        strTableList = clipString(strTableList,posWhere);
        if(parseTableList(strTableList) == ParseResult::FAILURE)
            return ParseResult::FAILURE;

        //--------------------------------------------------------
        // Mark delete column
        //---------------------------------------------------------
        shared_ptr<columnParts>parts = make_shared<columnParts>();
        parts->columnName = "deleted";
        parts->value = "T";
        ielements->lstColumns.push_back(parts);
            
        //---------------------------------------------------------
        // Create condition list
        //---------------------------------------------------------
        
        //find where clause in the original query string
        posWhere = findKeyword(queryString, sqlTokenWhere);

        if(posWhere == std::string::npos)
            return ParseResult::SUCCESS;

        string strConditions = snipString(queryString, sqlTokenWhere,posWhere);

        if(parseConditionList(strConditions,CONDITIONTYPE::SELECT) == ParseResult::FAILURE)
            return ParseResult::FAILURE;

        return ParseResult::SUCCESS;
    }
    catch_and_trace
    return ParseResult::FAILURE;
}

/******************************************************
 * Add Condition
 ******************************************************/
ParseResult ParseQuery::addCondition(string _token, CONDITIONTYPE _conditionType)
{
    /* First sprint: Looking for 4 things:
        A column name   - text not enclosed in quotes
        A value         - enclosed in quotes
        A numeric       - no quotes, but numeric
        An operator     - Math operators =, <>, >, <, >=, <=

        Initially only looking for column, opeator, value in quotes
    */
   try
   {

        if(debug)
            fprintf(traceFile,"\ntoken=%s",_token.c_str());
        
        if(_token.empty())
            return ParseResult::SUCCESS;

        _token.erase(0,_token.find_first_not_of(SPACE));
        _token.erase(_token.find_last_not_of(SPACE)+1);

        //--------------------------------------------------------
        // Initialize condition and condition predicates
        //-------------------------------------------------------
        if(condition == nullptr)
        {
            condition = make_shared<Condition>();
            condition->condition    = " ";
            if(_conditionType == CONDITIONTYPE::JOIN)
                condition->isJoin   = true;
        }
        
        if(strcasecmp(_token.c_str(),sqlTokenOpenParen) == 0)
        {
            return ParseResult::SUCCESS;
        }

        if(strcasecmp(_token.c_str(),sqlTokenCloseParen) == 0)
        {
            return ParseResult::SUCCESS;
        }

        if(strcasecmp(_token.c_str(),sqlTokenAnd) == 0)
        {
            condition->condition = _token;
            return ParseResult::SUCCESS;
        }

        if(strcasecmp(_token.c_str(),sqlTokenOr) == 0)
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
        if(condition->op.empty())
        {
            _token = trim(_token);
            if(strcasecmp(_token.c_str(),sqlTokenLike) != 0
            && strcasecmp(_token.c_str(),sqlTokenGreater) != 0
            && strcasecmp(_token.c_str(),sqlTokenLessThan) != 0
            && strcasecmp(_token.c_str(),sqlTokenLessOrEqual) != 0
            && strcasecmp(_token.c_str(),sqlTokenGreaterOrEqual) != 0
            && strcasecmp(_token.c_str(),sqlTokenEqual) != 0
            && strcasecmp(_token.c_str(),sqlTokenNotEqual) != 0)
            {
                sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Condition operator missing. See ",_token);
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

        //if value NOT empty, process value
        if(condition->value.empty())
        {
            //case 1
            size_t posQuote = _token.find(QUOTE);
            if(posQuote != std::string::npos)
            {
                _token.erase(0,_token.find_first_not_of(QUOTE));
                _token.erase(_token.find_last_not_of(QUOTE)+1);
                //may be replaced downstream
                condition->value = _token;
            }
            else
                //case 2
                if(isNumeric(_token))
                {
                    condition->value = _token;
                }
                else
                    if(findKeyword(_token,sqlTokenOpenParen) != std::string::npos)
                    {
                        //case 3
                        //TODO processList();
                    }
                    else
                        condition->compareToName = parseColumnName(_token);  
        }

        switch(_conditionType)
        {
            case CONDITIONTYPE::SELECT:
                ielements->lstConditions.push_back(condition);
                break;
            case CONDITIONTYPE::JOIN:
                ielements->lstJoinConditions.push_back(condition);
                break;
            case CONDITIONTYPE::HAVING:
                ielements->lstHavingConditions.push_back(condition);
                break;
        }

        condition = make_shared<Condition>();
        return ParseResult::SUCCESS;
   }
   catch_and_trace
   return ParseResult::FAILURE;
}
/******************************************************
 * Parse Order
 ******************************************************/
ParseResult ParseQuery::parseOrderByList(string _workingString)
{
    try
    {

        if(_workingString.empty())
            return ParseResult::SUCCESS;
        
        if(debug)
        {
            fprintf(traceFile,"\n\n-----------Parse Order by----------------");
            fprintf(traceFile,"\nOrderBy string:%s",_workingString.c_str());
        }

        string token;
        unique_ptr<tokenParser>    tok = make_unique<tokenParser>();
        shared_ptr<OrderBy> orderBy (new OrderBy);
        tok->parse(_workingString,true);

        while(!tok->eof)
        {
            token = tok->getToken();
            if(!token.empty())
            {
                if(debug)
                    fprintf(traceFile,"\n order by tokens %s", token.c_str());
                    
                if(strcasecmp(token.c_str(),sqlTokenOrderAcending) != 0
                && strcasecmp(token.c_str(),sqlTokenOrderDescending) != 0)
                {
                    OrderOrGroup    order;
                    order.name      = parseColumnName(token);
                    orderBy->order.push_back(order);
                }
                else
                {
                    if(strcasecmp(token.c_str(),sqlTokenOrderDescending) == 0)
                        orderBy->asc = false;
                }
            }
        }

        if(orderBy == nullptr)
            ielements->orderBy = nullptr;
        else
        ielements->orderBy = orderBy;

        return ParseResult::SUCCESS;
    }
    catch_and_trace
    return ParseResult::FAILURE;
}
/******************************************************
 * Parse Group
 ******************************************************/
ParseResult ParseQuery::parseGroupByList(string _workingString)
{
    try
    {
        if(_workingString.empty())
            return ParseResult::SUCCESS;

        if(debug)
        {
            fprintf(traceFile,"\n\n-----------Parse Group by----------------");
            fprintf(traceFile,"\nGroup By string:%s",_workingString.c_str());
        }
        
        string token;
        unique_ptr<tokenParser> tok = make_unique<tokenParser>();
        shared_ptr<GroupBy> groupBy (new GroupBy);
        tok->parse(_workingString,true);

        while(!tok->eof)
        {
            token = tok->getToken();
            if(!token.empty())
            {
                if(debug)
                    fprintf(traceFile,"\n group by tokens %s", token.c_str());
                    
                OrderOrGroup    group;
                group.name      = parseColumnName(token);
                groupBy->group.push_back(group);
            }
        }

        if(groupBy == nullptr)
        ielements->groupBy = nullptr;
        else
            ielements->groupBy = groupBy;

        return ParseResult::SUCCESS;
    }
    catch_and_trace
    return ParseResult::FAILURE;
}

/******************************************************
 * Parse Conditions
 ******************************************************/
ParseResult ParseQuery::parseConditionList(string _workingString,CONDITIONTYPE _conditionType)
{
    try
    {
        _workingString = trim(_workingString);
        if(_workingString.empty())
            return ParseResult::SUCCESS;
        
        if(debug)
        {
            fprintf(traceFile,"\n\n-----------Parse Conditions----------------");
            if(_conditionType == CONDITIONTYPE::JOIN)
                fprintf(traceFile,"\nJoin Condition:%s",_workingString.c_str());
            else
            if(_conditionType == CONDITIONTYPE::HAVING)
                fprintf(traceFile,"\nHaving Condition:%s",_workingString.c_str());
            else
            fprintf(traceFile,"\nCondition:%s",_workingString.c_str());
        }
        
        _workingString.erase(0,_workingString.find_first_not_of(SPACE));
        _workingString.erase(_workingString.find_last_not_of(SPACE)+1);

        string token;
        unique_ptr<tokenParser> tok = make_unique<tokenParser>();
        tok->parse(_workingString,true);
        
        while(!tok->eof)
        {
            token = tok->getToken();
            if(!token.empty())
                if(addCondition(token,_conditionType) == ParseResult::FAILURE)
                    return ParseResult::FAILURE;
        }
        return ParseResult::SUCCESS;
    }
    catch_and_trace
    return ParseResult::FAILURE;
}
/******************************************************
 * Parse Column List
 ******************************************************/
ParseResult ParseQuery::parseColumnList(string _workingString)
{
    try
    {
        if(debug)
        {
            fprintf(traceFile,"\n\n-----------Parse Column List----------------");
            fprintf(traceFile,"\nColumn string:%s",_workingString.c_str());
        }

        if(_workingString.empty())
            return ParseResult::SUCCESS;

        char * token;
        token = strtok ((char*)_workingString.c_str(),",");
        while (token != NULL)
        {
            string strToken = token;
            shared_ptr<columnParts> parts = parseColumnName(strToken);
            if(parts == nullptr)
            {
                sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Failed to parse column name  ",strToken);
                return ParseResult::FAILURE;
            }
            ielements->lstColumns.push_back(parts);
            token = strtok (NULL, ",");
        }

        return ParseResult::SUCCESS;
    }
    catch_and_trace
    return ParseResult::FAILURE;
}
/******************************************************
 * Parse Column Name
 ******************************************************/
shared_ptr<columnParts> ParseQuery::parseColumnName(string _str)
{
    /*
        Column name have the following forms

        case 1) column alias        - surname AS last
        case 2) functions           - COUNT(), SUM(), AVG(), MAX(), MIN()
        case 3) simple name         - surname
        case 4) table/column        - customers.surname
        case 5) table alias column  - c.surname from customers c
    */
   try{

         _str = trim(_str);
        shared_ptr<columnParts> parts = make_shared<columnParts>(); 
        parts->fullName =_str;

        size_t columnAliasPosition = findKeyword(_str,sqlTokenAs);

        //case 1) column alias        - surname AS last
        string columnName  = _str;
        if(columnAliasPosition != std::string::npos)
        {
            parts->columnAlias = snipString(_str,columnAliasPosition+3);
            _str = _str.substr(0,columnAliasPosition);
        }

        // case 2) functions          - COUNT(), SUM(), AVG(), MAX(), MIN()
        size_t posParen = _str.find(sqlTokenOpenParen);
        if(posParen != std::string::npos)
        {
            parts->function = trim(clipString(_str,posParen));
            columnName = snipString(_str,posParen+1); 
            columnName.erase(columnName.find_last_of(CLOSEPAREN));
            columnName = trim(columnName);
        }
        else
            columnName = trim(_str);

        if(columnName.empty())
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Column name is empty");
            return nullptr;
        }
        size_t posPeriod = columnName.find(sqlTokenPeriod);
        if(posPeriod == std::string::npos)
        {
             // case 3) simple name         - surname
            parts->columnName = columnName;
        }
        else
        {
            // case 4/5)
            parts->tableAlias = columnName.substr(0,posPeriod);
            parts->columnName = snipString(columnName,posPeriod+1);
        }

        if(debug)    
            fprintf(traceFile,"\nColumn parts: function:%s name:%s alias:%s table Alias %s",parts->function.c_str(), parts->columnName.c_str(), parts->columnAlias.c_str(), parts->tableAlias.c_str());

        return parts;
   }
   catch_and_trace
   return nullptr;
};
/******************************************************
 * Parse Value List
 ******************************************************/
ParseResult ParseQuery::parseValueList(string _workingString)
{
    try
    {
    
        if(debug)
        {
            fprintf(traceFile,"\n\n-----------Parse Value List----------------");
            fprintf(traceFile,"\ntableString:%s",_workingString.c_str());
        }
        
        _workingString.erase(0,_workingString.find_first_not_of(SPACE));
        _workingString.erase(_workingString.find_last_not_of(SPACE)+1);

        if(_workingString.empty())
            return ParseResult::SUCCESS;
    
        string token;
        unique_ptr<tokenParser> tok = make_unique<tokenParser>(_workingString);
        while(!tok->eof)
        {
            token = tok->getToken();
            if(!token.empty())
                ielements->lstValues.push_back(token);
        }

        return ParseResult::SUCCESS;
    }
    catch_and_trace
    return ParseResult::FAILURE;
}
/******************************************************
 * Parse Table List
 ******************************************************/
ParseResult ParseQuery::parseTableList(string _workingString)
{
    try
    {
    
        if(debug)
        {
            fprintf(traceFile,"\n\n-----------Parse Table List----------------");
            fprintf(traceFile,"\ntableString:%s",_workingString.c_str());
        }

        if(_workingString.empty())
            return ParseResult::SUCCESS;

        char* token;

        token = strtok ((char*)_workingString.c_str(),",");
        while(token != NULL)
        {
            if(debug)
                fprintf(traceFile,"\ntoken %s",token);
            if(!ielements->tableName.empty())
            {
                sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Expecting only one table");
                return ParseResult::FAILURE;
            }
            ielements->tableName = trim(token);
            token = strtok (NULL, ",");
        }

        if(ielements->tableName.empty())
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Expecting at least one table");
            return ParseResult::FAILURE;
        }
        return ParseResult::SUCCESS;
    }
    catch_and_trace
    return ParseResult::FAILURE;
}



