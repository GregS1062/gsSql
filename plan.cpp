#pragma once
#include <list>
#include <vector>
#include "defines.h"
#include "sqlCommon.h"


/******************************************************
 * Plan
 ******************************************************/
class Plan
{
        //public for diagnostic purposes
        list<shared_ptr<sTable>>            lstTables;
        list<shared_ptr<Statement>>         lstStatements; 

        ParseResult             determineExecutionOrder();
};

ParseResult Plan::determineExecutionOrder()
{
    /*
      - Subqueries will always be executed first
    - Joins 
        - Analyze the indexes
        - Execution order does not matter if both tables are joined on indexed keys.
        - if a table is not indexed on join key, execute if first
    */
    if(lstStatements.size() < 2)
        return ParseResult::SUCCESS;

    return ParseResult::SUCCESS;
}

