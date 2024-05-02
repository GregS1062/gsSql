#pragma once
#include <list>
#include "defines.h"
#include "sqlCommon.h"
#include "lookup.cpp"
#include "utilities.cpp"
class ParseClause
{
    public:
        iClauses* iClause = new iClauses();
        list<char*> queries;
        ParseResult process(char*);
        ParseResult parseSelect(char*);
        ParseResult parseJoin(char*);
        ParseResult parseOrderByGroup(char*);
        signed long isJoin(char*);
};
/******************************************************
 * Process Select
 ******************************************************/
ParseResult ParseClause::process(char* _queryString)
{
    signed long positionJoin = isJoin(_queryString);
    if(positionJoin == DELIMITERERR)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Error: delimiter error searching for JOIN ");
        return ParseResult::FAILURE;
    }

    if(positionJoin != NEGATIVE)
    {
        positionJoin = lookup::findDelimiter(_queryString, (char*)sqlTokenJoin);
        _queryString = dupString(_queryString+positionJoin+strlen(sqlTokenJoin));
        if(parseJoin(_queryString) == ParseResult::FAILURE)
            return ParseResult::FAILURE;
        return ParseResult::SUCCESS;
    }
    
    if(parseSelect(_queryString) == ParseResult::FAILURE)
        return ParseResult::FAILURE;

    return ParseResult::SUCCESS;
}
/******************************************************
 * Parse Select
 ******************************************************/
ParseResult ParseClause::parseSelect(char* _queryString)
{

    char* queryString {};
    /*
        Case 1: Select * from table
        Case 2: Select * from table IN Select....
        Case 3: Select * from table JOIN table2 ON column = column
        case 4: JOIN table ON column = column
        case 5: variations on join INNER, OUTER, LEFT, RIGHT - NATURAL
    */

    if(debug)
    {
        fprintf(traceFile,"\n\n-------------------------PARSE CLAUSES - SELECT -------------------------------------------");
        fprintf(traceFile,"\nQuery String = %s",_queryString);
    }

    signed long positionSelect = lookup::findDelimiter(_queryString, (char*)sqlTokenSelect);

    if(positionSelect == DELIMITERERR)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Error: delimiter error searching for SELECT ");
        return ParseResult::FAILURE;
    }

    if(positionSelect == NEGATIVE)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Error: expecting the term 'Select' ");
        return ParseResult::FAILURE;
    }
    else
    {
        //remove 'Select' token from queryString
        queryString = dupString(_queryString+positionSelect+strlen(sqlTokenSelect));
    }


    if(parseOrderByGroup(queryString) == ParseResult::FAILURE)
        return ParseResult::FAILURE;

    //----------------------------------------------------------
    // Conditions
    //----------------------------------------------------------
    //case 5: select custid from customers where custid = 123
    //case 6: select custid from customers
    signed long positionOfWhere = lookup::findDelimiter(queryString, (char*)sqlTokenWhere);
    
    // Testing the presence of WHERE and ON
    // 1. Was there a parsing error
    // 2. Are neither present
    // 3. Are both present
    if(positionOfWhere  == DELIMITERERR
    && positionOfWhere  == NEGATIVE)
    {   
        sendMessage(MESSAGETYPE::ERROR,presentationType,true,queryString);
        sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Error: looking for WHERE");
        sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Error: position where ");
        sendMessage(MESSAGETYPE::ERROR,presentationType,true,std::to_string(positionOfWhere).c_str());

        return ParseResult::FAILURE;
    }


    if(positionOfWhere  > 0)
    {
        iClause->strConditions = dupString(queryString+positionOfWhere+strlen(sqlTokenWhere));
        queryString[positionOfWhere] = '\0';
    }
    
    signed long positionOfFrom = lookup::findDelimiter(queryString, (char*)sqlTokenFrom);
    if( positionOfFrom == DELIMITERERR
    && positionOfFrom == NEGATIVE)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Error: missing required 'FROM' or 'ON' ");
        return ParseResult::FAILURE;
    }

    if(positionOfFrom > 0)
    {
        iClause->strTables = dupString(queryString+positionOfFrom+strlen(sqlTokenFrom));
        queryString[positionOfFrom] = '\0';
    }

    signed long positionEqual = lookup::findDelimiter(iClause->strTables, (char*)sqlTokenEqual);
    signed long positionQuote = lookup::findDelimiter(iClause->strTables, (char*)sqlTokenQuote);

    if(positionEqual > 0
    || positionQuote > 0)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Error: found \" or =. missing 'where' ");
        return ParseResult::FAILURE;
    }

    signed long positionTop = lookup::findDelimiter(queryString, (char*)sqlTokenTop);
    if(positionTop == NEGATIVE)
        iClause->strColumns = dupString(queryString);
    
    if(positionTop > 0)
    {
        tokenParser* tok = new tokenParser(queryString);

        //Get next token
        char* token = tok->getToken();
        if(strcasecmp(token,sqlTokenTop) == 0)
        {
            token = tok->getToken();
            if(!isNumeric(token))
            {
                sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Expecting a number after TOP.");
                return ParseResult::FAILURE;
            }
            iClause->topRows = atoi(token);
            iClause->strColumns = dupString(queryString+tok->pos);
        }
    }


    free (queryString);

    return ParseResult::SUCCESS;

}
/******************************************************
 * Parse Join
 ******************************************************/
ParseResult ParseClause::parseJoin(char* _queryString)
{
     if(parseOrderByGroup(_queryString) == ParseResult::FAILURE)
        return ParseResult::FAILURE;
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
    signed long positionOfWhere = lookup::findDelimiter(_queryString, (char*)sqlTokenWhere);
    
    // Testing the presence of WHERE and ON
    // 1. Was there a parsing error
    // 2. Are neither present
    // 3. Are both present
    if(positionOfWhere  == DELIMITERERR)
    {   
        sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Error: looking for WHERE");
        return ParseResult::FAILURE;
    }

    if(positionOfWhere > 0)
    {
        
        iClause->strConditions = dupString(_queryString+positionOfWhere+strlen(sqlTokenWhere));
        _queryString[positionOfWhere] = '\0';  // truncate string for next operation  - this one leave tables in the string
    }

    signed long positionOfOn   = lookup::findDelimiter(_queryString, (char*)sqlTokenOn);
    if(positionOfOn > 0)
    {
        iClause->strJoinConditions = dupString(_queryString+positionOfOn+strlen(sqlTokenOn));
        _queryString[positionOfOn] = '\0'; // truncate string for next operation  - this one leave columns in the string
    }


    signed long positionEqual = lookup::findDelimiter(iClause->strTables, (char*)sqlTokenEqual);
    signed long positionQuote = lookup::findDelimiter(iClause->strTables, (char*)sqlTokenQuote);

    if(positionEqual > 0
    || positionQuote > 0)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Error: found \" or =. missing 'where' ");
        return ParseResult::FAILURE;
    }

    iClause->strTables = dupString(_queryString);
   
   return ParseResult::SUCCESS;
}
/******************************************************
 * Is Join
 ******************************************************/
signed long ParseClause::isJoin(char* _string)
{
    list<char*> lstJoin;
    lstJoin.push_back((char*)sqlTokenJoin);
    lstJoin.push_back((char*)sqlTokenInnerJoin);
    lstJoin.push_back((char*)sqlTokenOuterJoin);
    lstJoin.push_back((char*)sqlTokenLeftJoin);
    lstJoin.push_back((char*)sqlTokenRightJoin);
    return lookup::findDelimiterFromList(_string,lstJoin);
}
/******************************************************
 * Order By / Group By
 ******************************************************/
ParseResult ParseClause::parseOrderByGroup(char* _queryString)
{

    //----------------------------------------------------------
    // Order by / Group by
    //----------------------------------------------------------

    signed long positionOfOrderBy = lookup::findDelimiter(_queryString, (char*)sqlTokenOrderBy);
    signed long positionOfGroupBy = lookup::findDelimiter(_queryString, (char*)sqlTokenGroupBy);
    signed long positionOfHaving  = lookup::findDelimiter(_queryString, (char*)sqlTokenHaving);

    //No order by or group by clause
    if(positionOfGroupBy == NEGATIVE
    && positionOfHaving  == NEGATIVE
    && positionOfOrderBy == NEGATIVE)
    {
        return ParseResult::SUCCESS;
    }

    if(positionOfGroupBy == DELIMITERERR
    || positionOfHaving  == DELIMITERERR
    || positionOfOrderBy == DELIMITERERR)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Error: looking for Group By, OrderBy or Having");
        return ParseResult::FAILURE;
    }

    list<signed long> stack;
    stack.push_back(positionOfGroupBy);
    stack.push_back(positionOfOrderBy);
    stack.push_back(positionOfHaving);

    stack.sort();
    stack.reverse();

    for(long signed found : stack)
    {
        if(found == NEGATIVE)
            continue;

        if(debug)
            fprintf(traceFile,"\nStack: %ld",found);

        if(positionOfOrderBy == found)
        {
            iClause->strOrderBy = dupString(_queryString+positionOfOrderBy+strlen(sqlTokenOrderBy));
            if(debug)
                fprintf(traceFile,"\nOrder by clause: %s",iClause->strOrderBy);
            _queryString[positionOfOrderBy] = '\0';
        }

        if(positionOfGroupBy == found)
        {
            iClause->strGroupBy = dupString(_queryString+positionOfGroupBy+strlen(sqlTokenGroupBy));
            if(debug)
                fprintf(traceFile,"\nGroup by clause: %s",iClause->strGroupBy);
            _queryString[positionOfGroupBy] = '\0';
        }

        if(positionOfHaving == found)
        {
            iClause->strHaving = dupString(_queryString+positionOfHaving+strlen(sqlTokenHaving));
            if(debug)
                fprintf(traceFile,"\nHaving clause: %s",iClause->strHaving);
            _queryString[positionOfHaving] = '\0';
        }
    }
    return ParseResult::SUCCESS;
}

