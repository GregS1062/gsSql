#pragma once
#include <list>
#include "sqlCommon.h"

class iElements
{
    public:

    SQLACTION                       sqlAction {};               // Statement action
    char*                           tableName = nullptr;        // tables
    list<shared_ptr<columnParts>>   lstColumns;                 // used in update to set list of column/values            // column names
    list<char*>                     lstValues {};               // value lists - found in insert and select IN (...)
    shared_ptr<OrderBy>             orderBy{};                  // order by
    shared_ptr<GroupBy>             groupBy{};                  // group by
    list<Condition*>                lstConditions{};            // where condition (operator) value
    list<Condition*>                lstJoinConditions{};        // join ON conditions
    list<Condition*>                lstHavingConditions{};      // Having conditions
    int                             rowsToReturn = 0;           // max result rows

    ParseResult             clear();
};
/******************************************************
 * Clear
 ******************************************************/
ParseResult iElements::clear()
{
    sqlAction       = SQLACTION::NOACTION;
    rowsToReturn    = 0;
    tableName       = nullptr;
    lstConditions.clear();
    lstJoinConditions.clear();
    lstHavingConditions.clear();
    lstColumns.clear();
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
        char*       strOrderBy          {};
        char*       strTables           {};
        char*       strColumns          {};
        char*       strConditions       {};
        char*       strJoinConditions   {};
        char*       strHavingConditions {};
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