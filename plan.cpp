#pragma once
#include <list>
#include <vector>
#include "defines.h"
#include "interfaces.h"
#include "sqlCommon.h"
#include "parseQuery.cpp"
#include "binding.cpp"

/******************************************************
 * Plan
 ******************************************************/
class Plan
{
    vector<Statement*>  statements;
    list<char*>         queries;
    
    public:
        iSQLTables*     isqlTables = new iSQLTables();
        ParseResult     split(char*);
        ParseResult     prepare(char*);
        ParseResult     validateSQLString(char*);
};
/******************************************************
 * Analyze
 ******************************************************/

ParseResult Plan::prepare(char* _queryString)
{
    /*---------------------------------------------------
    1) Validates if query is well-formed
    2) Breaks query into subqueries
    3) Breaks subqueries into clauses
 ------------------------------------------------------*/
    if(validateSQLString(_queryString) == ParseResult::FAILURE)
        return ParseResult::FAILURE;

    if(split(_queryString) == ParseResult::FAILURE)
        return ParseResult::FAILURE;

    ParseQuery* parseQuery;
    for(char* query : queries)
    {

        parseQuery  = new ParseQuery();
        parseQuery->parse(query);
        iElements* elements = parseQuery->iElement;
        for(char* element : elements->lstColName)
        {
            printf("\n element %s",element);
        } 
    }
        
    return ParseResult::SUCCESS;
}
/******************************************************
 * Validate SQL String
 ******************************************************/
ParseResult Plan::validateSQLString(char* _queryString)
{
    //---------------------------------
    //  Is queryString well formed?
    //---------------------------------

    if(debug)
      fprintf(traceFile,"\n validateSQLString");

    string sql;
    sql.append(_queryString);

    // Do open and close parenthesis match?
    if(std::count(sql.begin(), sql.end(), '(')
    != std::count(sql.begin(), sql.end(), ')'))
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Syntax error: mismatch of parenthesis");
        return ParseResult::FAILURE;
    }

    //Do quotes match?
    bool even = std::count(sql.begin(), sql.end(), '"') % 2 == 0;
    if(!even)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Syntax error: too many or missing quotes.");
        return ParseResult::FAILURE;
    }

    return ParseResult::SUCCESS;
}
/******************************************************
 * Process Select
 ******************************************************/
ParseResult Plan::split(char* _queryString)
{
    int tries= 0;
    int offset = 6;
    list<char*>lstDelimiters;
    char* query;
    lstDelimiters.push_back((char*)sqlTokenInsert);
    lstDelimiters.push_back((char*)sqlTokenDelete);
    lstDelimiters.push_back((char*)sqlTokenUpdate);
    lstDelimiters.push_back((char*)sqlTokenSelect);
    lstDelimiters.push_back((char*)sqlTokenInnerJoin);
    lstDelimiters.push_back((char*)sqlTokenOuterJoin);
    lstDelimiters.push_back((char*)sqlTokenLeftJoin);
    lstDelimiters.push_back((char*)sqlTokenRightJoin);
    lstDelimiters.push_back((char*)sqlTokenJoin);
    signed long found = lookup::findDelimiterFromList(_queryString+offset,lstDelimiters);
    if(found == NEGATIVE)
    {
        queries.push_back(_queryString);
        return ParseResult::SUCCESS;
    }
    while(found > 0)
    {
        //select * from orders Join Author 
        query = dupString(_queryString);
        query[found+offset] = '\0';
        queries.push_back(query);
        _queryString = dupString(_queryString+found+offset);
        found = lookup::findDelimiterFromList(_queryString+offset,lstDelimiters);
        if(tries > 4)
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Runaway ParseClause split");
            break;
        }
            
        tries++;
    }
    if(strlen(_queryString) > 0)
    {
        queries.push_back(_queryString);
    }
    return ParseResult::SUCCESS;
}



