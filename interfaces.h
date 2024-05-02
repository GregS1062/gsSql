#pragma once
#include <list>
#include "sqlCommon.h"
class iElements
{
    public:

    SQLACTION               sqlAction {};               // Statement action
    list<char*>             lstTables {};               // tables
    list<ColumnNameValue*>  lstColNameValue {};         // used in update to set list of column/values
    list<char*>             lstColName{};               // column names
    list<char*>             lstValues{};                // value lists - found in insert and select IN (...)
    OrderBy*                orderBy         = nullptr;  // order by
    GroupBy*                groupBy         = nullptr;  // group by
    list<Condition*>        lstConditions{};            // where condition (operator) value
    list<Condition*>        lstJoinConditions{};        // join ON conditions
    int                     rowsToReturn = 0;           // max result rows

    ParseResult             clear();
};
/******************************************************
 * Clear
 ******************************************************/
ParseResult iElements::clear()
{
    sqlAction       = SQLACTION::NOACTION;
    rowsToReturn    = 0;
    lstTables.clear();
    lstColName.clear();
    lstColNameValue.clear();
    lstConditions.clear();
    lstJoinConditions.clear();
    lstColNameValue.clear();
    orderBy->order.clear();
    groupBy->group.clear();
    groupBy->having.clear();
    return ParseResult::SUCCESS;
}
/******************************************************
 * iClauses - splits queries into clauses
 ******************************************************/
class iClauses
{
    public:
        iClauses(){};
        char*       strGroupBy          {};
        char*       strHaving           {};
        char*       strOrderBy          {};
        char*       strTables           {};
        char*       strColumns          {};
        char*       strConditions       {};
        char*       strJoinConditions   {};
        int         topRows             = 0;
        void        clear();
};

/******************************************************
 * Clear - clears iClause
 ******************************************************/
void iClauses::clear()
{
        if(strColumns != nullptr)
            free(strColumns);

        if(strColumns != nullptr)
            free(strColumns);

        if(strGroupBy != nullptr)
            free(strGroupBy);

        if(strOrderBy != nullptr)
            free(strOrderBy);

        if(strTables != nullptr)
            free(strTables);

        if(strColumns != nullptr)
            free(strColumns);

        if(strConditions != nullptr)
            free(strConditions);

        if(strJoinConditions != nullptr)
            free(strJoinConditions);
            
        topRows             = 0;
}

class iSQLTables
{
    public:
    
    list<sTable*>           tables;
};