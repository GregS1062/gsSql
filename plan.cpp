#pragma once
#include <list>
#include <vector>
#include "defines.h"
#include "interfaces.h"
#include "sqlCommon.h"

/******************************************************
 * Plan
 ******************************************************/
class Plan
{
    /* The lstColumnsInPriorStatements list represent columns that have already been read during execution
            So if a statement has a condition such as order.custid that is a key on order, one would search the scoredColumns
            for customer.custid which would be expected to contain a value
    */

        list<shared_ptr<Column>>            lstColumnsInPriorStatements;
        vector<shared_ptr<Statement>>         orderedStatements;
        vector<shared_ptr<Statement>>       scoredList;
        shared_ptr<iSQLTables>              isqlTables;
        int                                 scoreStatements(shared_ptr<Statement> statement);
        bool                                IsKeySearch(shared_ptr<Column>);
        bool                                IsColumnInPriorStatement(shared_ptr<Column>);
        void                                loadNextStatement();
    public:       
        Plan(shared_ptr<iSQLTables>);
        vector<shared_ptr<Statement>>         determineExecutionOrder(vector<shared_ptr<Statement>>);

};
Plan::Plan(shared_ptr<iSQLTables> _isqlTables)
{
    isqlTables = _isqlTables;
}
/******************************************************
 * Determine Execution Order
 ******************************************************/
vector<shared_ptr<Statement>> Plan::determineExecutionOrder(vector<shared_ptr<Statement>> _scoredList)
{
    try
    {
        if(_scoredList.size() < 2)
            return _scoredList;

        scoredList = _scoredList;
        size_t count = scoredList.size();
        while(count > 0)
        {
            loadNextStatement();
            count--;
        }

        return orderedStatements;
    }
    catch_and_throw_to_caller
}
/******************************************************
 * Load Next Statement
 ******************************************************/
void Plan::loadNextStatement()
{
    try
    {
        int highScore   = 0;
        int pos = -1;
        shared_ptr<Statement> statement;
        for(int i = 0;i < (int)scoredList.size();i++)
        {
            statement = scoredList.at(i);
            int score = scoreStatements(statement);
            if(score > highScore)
            {
                highScore = score;
                pos = i;
            }
        }
        if (pos < 0)
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Cannot determine order of execution ");
            throw exception();
        }
        statement = scoredList.at(pos);
        for(shared_ptr<Column> col : statement->table->columns)
        {
            lstColumnsInPriorStatements.push_back(col);
        };
        orderedStatements.push_back(statement);
        if(pos > 0)
            pos--;
        scoredList.erase(scoredList.begin() + pos);
    }
    catch_and_throw_to_caller
}
/******************************************************
 * Score Statements
 ******************************************************/
int Plan::scoreStatements(shared_ptr<Statement> _statement)
{
      /*
        Sprint one - simple optimization
            Score 1st execute
            - low       Statement with no conditions (table scan)
            - medium    Statement with a col/value condition
            - High      Statement with a col/value condition on a primary key

            Score subsequent joins
            - No        join on column not included in a preceding execution
            - low       join on non-indexed column
            - medium    join on index column including in preceding execution

   */

   /* For now produce a binary result, should this statement be run
        Yes = first pass
        No  = join or key columns not read yet
    */

   int lowScore     = 5;
   int mediumScore  = 10;
   int highScore    = 15;
   try
   {
        int score = 0;

        if(_statement == nullptr)
            return 0;

        if (_statement->action == SQLACTION::SELECT)
            score = lowScore;

        for(shared_ptr<Condition> condition : _statement->table->conditions)
        {
            //a null compareToColumn means this is a col/val condition
            if(condition->compareToColumn == nullptr)
            {
                score = score + mediumScore;
                for(shared_ptr<sIndex> index : _statement->table->indexes)
                {
                    shared_ptr<Column> col = index->columns.front();

                    //key match found
                    if (strcmp(condition->col->name.c_str(),col->name.c_str()) == 0)
                        score = score + highScore;
                }
            }
            else
            {
                //a compareToColumn means this is a col/col condition

                // non-join limiting col/col condition on the same table (example discount > salesTax)  
                if (strcmp(condition->col->tableName.c_str(),condition->compareToColumn->tableName.c_str()) == 0)
                {
                    score = score + lowScore;
                    continue;
                }

                shared_ptr<Column> col;
                /*
                    Since the statement is just now being scored, its columns cannot be compared 
                        to columns already read at join time, therefore select the column not belonging
                        to the statement table.  This can either be condition->col or condition->compareToColumn
                */
                if (strcmp(condition->col->tableName.c_str(),_statement->table->name.c_str()) != 0)
                    col = condition->col;
                
                if (strcmp(condition->compareToColumn->tableName.c_str(),_statement->table->name.c_str()) != 0)
                    col = condition->compareToColumn;
                
                if(col == nullptr)
                {
                    sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Optimizer expecting a join col/col ");
                    throw exception();
                }

                // if NOT column in prior statement
                if(!IsColumnInPriorStatement(col))
                    continue;
                
                score = score + mediumScore;

                if(IsKeySearch(col))
                    score = score + highScore;
            }
        }

        return score;
   }
    catch_and_throw_to_caller
}
/******************************************************
 * Is Column In Prior Statement
 ******************************************************/
bool Plan::IsColumnInPriorStatement(shared_ptr<Column> _col)
{
    try
    {
        for(shared_ptr<Column> col : lstColumnsInPriorStatements)
        {
            if(strcasecmp(col->name.c_str(),_col->name.c_str()) == 0)
                return true;
        }
        return false;
    }
    catch_and_throw_to_caller
}
/******************************************************
 * Is Key Search
 ******************************************************/
bool Plan::IsKeySearch(shared_ptr<Column> _col)
{
    try
    {
        // 3)

        shared_ptr<sTable> tbl = nullptr;
        for(shared_ptr<sTable> table : isqlTables->tables)
        {
            if(strcasecmp(table->name.c_str(), _col->tableName.c_str()) == 0)
            {
                tbl = table;
                break;
            }
        }
        if(tbl == nullptr)
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Optimizer cannot find table in SQL definition: ",_col->tableName);
            throw exception();
        }

        //scroll through table indexes
        for(shared_ptr<sIndex> index : tbl->indexes)
        {
            //TODO Modify for multi-key indexes
            shared_ptr<Column> col = index->columns.front();

            //key match found
            if (strcmp(_col->name.c_str(),col->name.c_str()) == 0)
                return true;   

        }
        return false;
    }
    catch_and_throw_to_caller
}

