#pragma once
#include <list>
#include "sqlCommon.h"

class iElements
{
    public:

    SQLACTION                       sqlAction {};               // Statement action
    string                          tableName{};        // tables
    list<shared_ptr<columnParts>>   lstColumns;                 // used in update to set list of column/values            // column names
    list<string>                    lstValues {};               // value lists - found in insert and select IN (...)
    shared_ptr<OrderBy>             orderBy{};                  // order by
    shared_ptr<GroupBy>             groupBy{};                  // group by
    list<shared_ptr<Condition>>     lstConditions{};            // where condition (operator) value
    list<shared_ptr<Condition>>     lstJoinConditions{};        // join ON conditions
    list<shared_ptr<Condition>>     lstHavingConditions{};      // Having conditions
    int                             rowsToReturn = 0;           // max result rows

    ParseResult                     clear();
};
/******************************************************
 * Clear
 ******************************************************/
ParseResult iElements::clear()
{
    sqlAction       = SQLACTION::NOACTION;
    rowsToReturn    = 0;
    tableName       = {};
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
struct iClauses
{
    public:
        string       strGroupBy          {};
        string       strOrderBy          {};
        string       strTables           {};
        string       strColumns          {};
        string       strConditions       {};
        string       strJoinConditions   {};
        string       strHavingConditions {};
        int          topRows             = 0;
};

struct iSQLTables
{
    list<shared_ptr<sTable>> tables;
};