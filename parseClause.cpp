#pragma once
#include <list>
#include "defines.h"
#include "sqlCommon.h"
#include "interfaces.h"
#include "prepareQuery.cpp"

class ParseClause
{
    public:
        iClauses        iClause;
        list<string>    queries;
        ParseResult     process(string);
        ParseResult     parseSelect(string);
        ParseResult     parseJoin(string);
        size_t          parseOrderByGroup(string);
};
/******************************************************
 * Process Process - Breaks query string into clauses
 ******************************************************/
ParseResult ParseClause::process(string _queryString)
{
    /*
        Case 1: Select * from table
        Case 2: Select * from table IN Select....
        Case 3: Select * from table JOIN table2 ON column = column
        case 4: JOIN table ON column = column
        case 5: variations on join INNER, OUTER, LEFT, RIGHT - NATURAL
    */

    try
    {

        size_t positionJoin = isJoin(_queryString);

        if(positionJoin != std::string::npos)
        {
            positionJoin = findKeyword(_queryString,sqlTokenJoin);
            if(parseJoin(snipString(_queryString,sqlTokenJoin,positionJoin)) == ParseResult::FAILURE)
                return ParseResult::FAILURE;
            return ParseResult::SUCCESS;
        }
        
        if(parseSelect(_queryString) == ParseResult::FAILURE)
            return ParseResult::FAILURE;

        return ParseResult::SUCCESS;
    }
    catch(const std::exception& e)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Exception error",e.what());
        return ParseResult::FAILURE;
    }
}
/******************************************************
 * Parse Select - Parses the select statement
 ******************************************************/
ParseResult ParseClause::parseSelect(string _queryString)
{
    /*
        Logic:
            query string is parsed in reverse by finding and eraseing clauses

        case 1: select from table
        case 2: select from table where (conditions)
    */
   try
   {
        if(debug)
        {
            fprintf(traceFile,"\n\n-------------------------PARSE CLAUSES - SELECT -------------------------------------------");
            fprintf(traceFile,"\nQuery String = %s",_queryString.c_str());
        }

        size_t positionSelect = findKeyword(_queryString,sqlTokenSelect);

        if(positionSelect == std::string::npos)
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Error: expecting the term 'Select' ");
            return ParseResult::FAILURE;
        }

        //remove 'Select' token from queryString
        _queryString = snipString(_queryString,strlen(sqlTokenSelect));

        size_t OrderByGroupBy = parseOrderByGroup(_queryString);
        if(OrderByGroupBy != std::string::npos)
            _queryString = clipString(_queryString,OrderByGroupBy);



        //----------------------------------------------------------
        // Conditions
        //----------------------------------------------------------
        //case 1: select custid from customers
        //case 2: select custid from customers where custid = 123
        size_t positionOfWhere = findKeyword(_queryString,sqlTokenWhere);

        // 2)
        if(positionOfWhere != std::string::npos)
        {
            iClause.strConditions = snipString(_queryString,sqlTokenWhere,positionOfWhere);
            // erase the where clause
            _queryString = clipString(_queryString,positionOfWhere);
        }
        
        // 1) drops through
        size_t positionOfFrom = findKeyword(_queryString,sqlTokenFrom);
        if(positionOfFrom == std::string::npos)
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Missing required key word 'FROM'");
            return ParseResult::FAILURE;
        }

        iClause.strTables = snipString(_queryString,sqlTokenFrom,positionOfFrom);
        _queryString = clipString(_queryString,positionOfFrom);

        size_t positionTop = findKeyword(_queryString,sqlTokenTop);
        if(positionTop == std::string::npos)
        {
            iClause.strColumns = _queryString;
            return ParseResult::SUCCESS;
        }
        
        _queryString = snipString(_queryString,sqlTokenTop,positionTop);
        size_t positionAfterCount = _queryString.find(SPACE);

        char* token = (char*)_queryString.substr(0,positionAfterCount).c_str();
        iClause.topRows = atoi(token);
        iClause.strColumns = snipString(_queryString,positionAfterCount);

        return ParseResult::SUCCESS;
    }
    catch(const std::exception& e)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Exception error",e.what());
        return ParseResult::FAILURE;
    }

}
/******************************************************
 * Parse Join - Parses join statements
 ******************************************************/
ParseResult ParseClause::parseJoin(string _queryString)
{

    //----------------------------------------------------------
    // Join Conditions
    //----------------------------------------------------------
    /* case : 
            SELECT *
              FROM Orders
              LEFT JOIN OrderLines ON OrderLines.OrderID=Orders.ID
              WHERE Orders.ID = 12345

        NOTE: presence of both WHERE condition and JOIN condition
 */

    try{
        if(debug)
            fprintf(traceFile,"\n********************* parseJoin *******************");
        

       size_t OrderByGroupBy = parseOrderByGroup(_queryString);
       if(OrderByGroupBy != std::string::npos)
            _queryString = clipString(_queryString,OrderByGroupBy);
    
        size_t positionOfWhere = findKeyword(_queryString, sqlTokenWhere);
        
        // Testing the presence of WHERE and ON
        // 1. Was there a parsing error
        // 2. Are neither present
        // 3. Are both present

        if(positionOfWhere != std::string::npos)
        {
            iClause.strConditions = snipString(_queryString,sqlTokenWhere,positionOfWhere);
            // erase the where clause
            _queryString = clipString(_queryString,positionOfWhere);
        }

        size_t positionOfOn   = findKeyword(_queryString, sqlTokenOn);
        if(positionOfOn != std::string::npos)
        {
            iClause.strJoinConditions = snipString(_queryString,sqlTokenOn,positionOfOn);
            // erase the condition clause
            _queryString = clipString(_queryString,positionOfOn-1);
        }


        size_t positionEqual = findKeyword(_queryString, sqlTokenEqual);
        size_t positionQuote = findKeyword(_queryString, sqlTokenQuote);

        if(positionEqual != std::string::npos
        || positionQuote != std::string::npos)
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Error: found \" or =. missing 'where' ");
            return ParseResult::FAILURE;
        }

        iClause.strTables = _queryString;
    
        return ParseResult::SUCCESS;
    }
    catch(const std::exception& e)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Exception error",e.what());
        return ParseResult::FAILURE;
    }
}

/******************************************************
 * Order By / Group By - Parses order and group by
 ******************************************************/
size_t ParseClause::parseOrderByGroup(string _queryString)
{

    //----------------------------------------------------------
    // Order by / Group by
    //----------------------------------------------------------

    size_t positionOfOrderBy = findKeyword(_queryString,sqlTokenOrderBy);
    size_t positionOfGroupBy = findKeyword(_queryString,sqlTokenGroupBy);
    size_t positionOfHaving  = findKeyword(_queryString,sqlTokenHaving);

    //No order by or group by clause
    if(positionOfGroupBy == std::string::npos
    && positionOfHaving  == std::string::npos
    && positionOfOrderBy == std::string::npos)
    {
        return std::string::npos;
    }

    list<size_t> stack;
    stack.push_back(positionOfGroupBy);
    stack.push_back(positionOfOrderBy);
    stack.push_back(positionOfHaving);

    stack.sort();
    stack.reverse();

    size_t lowest = std::string::npos;

    for(size_t found : stack)
    {
        if(found == std::string::npos)
            continue;

        lowest = found;

        if(positionOfOrderBy == found)
        {
            iClause.strOrderBy = snipString(_queryString,sqlTokenOrderBy,positionOfOrderBy);
            _queryString = clipString(_queryString,positionOfOrderBy);
        }

        if(positionOfGroupBy == found)
        {
            iClause.strGroupBy = snipString(_queryString,sqlTokenGroupBy,positionOfGroupBy);
            
            if(debug)
                fprintf(traceFile,"\nGroup by clause: %s",iClause.strGroupBy.c_str());
                
            _queryString = clipString(_queryString,positionOfGroupBy);

        }

        if(positionOfHaving == found)
        {
            iClause.strHavingConditions = snipString(_queryString,sqlTokenHaving,positionOfHaving);
            if(debug)
                fprintf(traceFile,"\nHaving clause: %s",iClause.strHavingConditions.c_str());
            _queryString = clipString(_queryString,positionOfHaving);
        }
    }
    return lowest;
}

