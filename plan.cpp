#pragma once
#include <list>
#include <vector>
#include "defines.h"
#include "interfaces.h"
#include "sqlCommon.h"
#include "prepareQuery.cpp"
#include "parseQuery.cpp"
#include "binding.cpp"
#include "sqlEngine.cpp"

/******************************************************
 * Plan
 ******************************************************/
class Plan
{
    list<Statement*>    lstStatements;
    list<char*>         queries;
    iElements*          ielements = new iElements();
    OrderBy*            orderBy     = new OrderBy;
    GroupBy*            groupBy     = new GroupBy; 
    
    public:
        iSQLTables*     isqlTables = new iSQLTables();
        ParseResult     split(char*);
        ParseResult     prepare(char*);
        ParseResult     execute();
        ParseResult     validateSQLString(char*);
};
/******************************************************
 * Prepare
 ******************************************************/
ParseResult Plan::prepare(char* _queryString)
{
    /*---------------------------------------------------
    1) Validates if query is well-formed
    2) Query is divided into subqueries
    3) Subqueries divided into clauses
    4) Clauses divided into elements
    5) Elements are bound into columns and values
 ------------------------------------------------------*/
    char* querystr = sanitizeQuery(_queryString);
    if( querystr == nullptr)
        return ParseResult::FAILURE;

    sendMessage(MESSAGETYPE::ERROR,presentationType,true,querystr); 
    return ParseResult::FAILURE;

    if(split(_queryString) == ParseResult::FAILURE)
        return ParseResult::FAILURE;

    ParseQuery* parseQuery = new ParseQuery();
    for(char* query : queries)
    {
        parseQuery->parse(query);
        ielements = parseQuery->iElement;
        if(ielements->lstColName.size() == 0)
        {
            printf("\n parse query produced no no elements ");
            break;
        }
        Binding* binding = new Binding();
        binding->isqlTables     = isqlTables;
        binding->ielements      = ielements;
        binding->bind();
        lstStatements           = binding->lstStatements;
        orderBy                 = binding->orderBy;
        groupBy                 = binding->groupBy;

    }
        
    return ParseResult::SUCCESS;
}
/******************************************************
 * Prepare
 ******************************************************/
ParseResult Plan::execute()
{
    sqlEngine* engine = new sqlEngine(isqlTables);
    if(lstStatements.size() == 0)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Bind produced no statements");
        return ParseResult::FAILURE;
    }
    for(Statement* statement : lstStatements)
    {
        statement->orderBy  = orderBy;
        statement->groupBy  = groupBy;
        engine->execute(statement);
    }
    return ParseResult::SUCCESS;
}

/******************************************************
 * Process Split
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
    }
    if(strlen(_queryString) > 0)
    {
        queries.push_back(_queryString);
    }
    return ParseResult::SUCCESS;
}

