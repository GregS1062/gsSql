#pragma once
#include <list>
#include <vector>
#include "defines.h"
#include "interfaces.h"
#include "sqlCommon.h"
#include "prepareQuery.cpp"
#include "parseQuery.cpp"
#include "binding.cpp"
#include "sqlSelectEngine.cpp"
#include "sqlJoinEngine.cpp"
#include "sqlModifyEngine.cpp"
#include "printDiagnostics.cpp"
#include "functions.cpp"

/******************************************************
 * Plan
 ******************************************************/
class Plan
{
    /*--------------------------------------------------------------------------------------------
        Rules and logic
        1) A query will be broken into statements
            Example: 
                statement 1) Select c.*, o.orderid o.totalAmount from customers c 
                statement 2) join on orders o where c.custid = o.custid group by c.zipCode
        2) Each statement will have one and only one table
        3) A given statement may contain columns for another statement, these must be resolved

        Prepare
        1) Validate that query is well-formed
        2) Divide query into subqueries
        3) Divide subqueries into clauses
        4) Divide clauses into elements
        5) bind elements into tables, columns, functions and values
        6) Bind tables, columns, functions and values into statements 

        Execute
         - Analyze statements to determine order of execution
            Rules:
                - Subqueries go first 
                - Where an indexed join, read the table with the least rows first
                - Where an indexed join, read the table with a "where" condition

    ----------------------------------------------------------------------------------------------*/
    // This will be a merged list of tables in the query including selects and joins
    //      It is required by bind because column names in the select may belong to one of the join
    //      tables
    list<char*>         lstDeclaredTables;
    list<Statement>     lstStatements;
    list<char*>         queries;
    list<iElements*>    lstElements;
    list<Column>        reportColumns;
    iSQLTables*         isqlTables;
    shared_ptr<OrderBy> orderBy;
    shared_ptr<GroupBy> groupBy; 
    ParseResult         printFunctions(resultList* _results);
    
    public:
        Plan(iSQLTables*);
        ParseResult     split(shared_ptr<char[]>);
        ParseResult     prepare(char*);
        ParseResult     execute();
        ParseResult     determineExecutionOrder();
};
Plan::Plan(iSQLTables* _isqlTables)
{
    isqlTables = _isqlTables;
}
/******************************************************
 * Prepare
 ******************************************************/
ParseResult Plan::prepare(char* _queryString)
{
    ParseQuery parseQuery;

    // 1)
    shared_ptr<char[]> querystr = normalizeQuery(_queryString,MAXSQLSTRINGSIZE);
    if( querystr == nullptr)
        return ParseResult::FAILURE;

    // 2)
    if(split(querystr) == ParseResult::FAILURE)
        return ParseResult::FAILURE;

    // 3 And 4)
    for(char* query : queries)
    {
        if(debug)
            printf("\nQuery statements to be parse %s",_queryString);
        if(parseQuery.parse(query) == ParseResult::FAILURE)
            return ParseResult::FAILURE;

        if(parseQuery.iElement->tableName != nullptr)
            lstDeclaredTables.push_back(parseQuery.iElement->tableName);
        
        if(parseQuery.iElement->lstColumns.size() == 0
        && parseQuery.iElement->lstValues.size() == 0
        && parseQuery.iElement->sqlAction == SQLACTION::SELECT)
        {
            printf("\n nothing in column list ");
            break;
        }
        
        lstElements.push_back(parseQuery.iElement);
    }
            
    // 5)
    Binding* binding            = new Binding(isqlTables);
    binding->bindTableList(lstDeclaredTables);
    for(iElements* ielement : lstElements)
    {
        if(debug)
            printf("\n iElement tablename %s",ielement->tableName);
        // 6)
        lstStatements.push_back(*binding->bind(ielement));
        if(binding->orderBy != nullptr )
            orderBy                 = binding->orderBy;
        if(binding->groupBy != nullptr)
            groupBy                 = binding->groupBy;

    } 
        
    return ParseResult::SUCCESS;
}
/******************************************************
 * Execute
 ******************************************************/
ParseResult Plan::execute()
{

    if(lstStatements.size() == 0)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Bind produced no statements");
        return ParseResult::FAILURE;
    }

    resultList*		results;

    for(Statement statement : lstStatements)
    {
        if(debug)
        {
            if(statement.table != nullptr)
                printTable(statement.table);
            if(orderBy != nullptr)
                printOrderBy(orderBy);
            if(groupBy != nullptr)
                printGroupBy(groupBy);
        }

        //Were any functions asked for?
        bool functions = false;
        for(Column* col : statement.table->columns)
        {
            if(col->functionType != t_function::NONE)
            {
                functions = true;
                break;
            }   
        } 

        switch(statement.action)
        {
            case SQLACTION::CREATE:
            {
                sendMessage(MESSAGETYPE::ERROR,presentationType,false,"Not implemented at this time");
                return ParseResult::FAILURE;
                break;
            }

            case SQLACTION::NOACTION:
            {
                sendMessage(MESSAGETYPE::ERROR,presentationType,false,"Invalid SQL Action");
                return ParseResult::FAILURE;
                break;
            }
            case SQLACTION::SELECT:
            {
                sqlSelectEngine sqlSelect;;
                if(sqlSelect.execute(statement) == ParseResult::FAILURE)
                {
                    return ParseResult::FAILURE;
                };
                results = sqlSelect.results;
                break;
            }
            case SQLACTION::INSERT:
            {
                sqlModifyEngine sqlModify;
                if(sqlModify.insert(statement) == ParseResult::FAILURE)
                {
                    return ParseResult::FAILURE;
                };
                break;
            }
            case SQLACTION::UPDATE:
            {
                sqlModifyEngine sqlModify;
                if(sqlModify.update(statement) == ParseResult::FAILURE)
                {
                    return ParseResult::FAILURE;
                };
                break;
            }
            case SQLACTION::DELETE:
            {
                /*
                    NOTE: Update and Delete use the same logic
                */
                sqlModifyEngine sqlModify;
                if(sqlModify.update(statement) == ParseResult::FAILURE)
                {
                    return ParseResult::FAILURE;
                };
                break;
            }
            case SQLACTION::INVALID:
            {
                sendMessage(MESSAGETYPE::ERROR,presentationType,false,"Invalid SQL Action");
                return ParseResult::FAILURE;
                break;
            }
            case SQLACTION::JOIN:
            {
                sqlJoinEngine sqlJoin;;
                if(sqlJoin.join(statement, results) == ParseResult::FAILURE)
                {
                    return ParseResult::FAILURE;
                };
                results = sqlJoin.results;
                break;
            }
            case SQLACTION::LEFT:
            {
                sendMessage(MESSAGETYPE::ERROR,presentationType,false,"Joins not implemented at this time");
                return ParseResult::FAILURE;
                break;
            }
            case SQLACTION::RIGHT:
            {
                sendMessage(MESSAGETYPE::ERROR,presentationType,false,"Joins not implemented at this time");
                return ParseResult::FAILURE;
                break;
            }
            case SQLACTION::INNER:
            {
                sendMessage(MESSAGETYPE::ERROR,presentationType,false,"Joins not implemented at this time");
                return ParseResult::FAILURE;
                break;
            }
            case SQLACTION::OUTER:
            {
                sendMessage(MESSAGETYPE::ERROR,presentationType,false,"Joins not implemented at this time");
                return ParseResult::FAILURE;
                break;
            }
            case SQLACTION::FULL:
            {
                sendMessage(MESSAGETYPE::ERROR,presentationType,false,"Joins not implemented at this time");
                return ParseResult::FAILURE;
                break;
            }
            case SQLACTION::NATURAL:
            {
                sendMessage(MESSAGETYPE::ERROR,presentationType,false,"Joins not implemented at this time");
                return ParseResult::FAILURE;
                break;
            }
            case SQLACTION::CROSS:
            {
                sendMessage(MESSAGETYPE::ERROR,presentationType,false,"Joins not implemented at this time");
                return ParseResult::FAILURE;
                break;
            }
        }

        if(results == nullptr)
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Results = null");
            return ParseResult::FAILURE;
        }

        if(groupBy == nullptr
        && functions)
        {
            if(debug)
                printf("\n------------- print functions ----------------");
            printFunctions(results);
            return ParseResult::SUCCESS;
        }

		if(groupBy->group.size() > 0)
			results->groupBy(groupBy,functions);
        
		if(orderBy->order.size() > 0)
			results->orderBy(orderBy);

    }
    results->print();
    return ParseResult::SUCCESS;
}
/******************************************************
 * Print Functions
 ******************************************************/
ParseResult Plan::printFunctions(resultList* _results)
{
    /*
        Logic:
            1) Capture and save an image of the results
                Which will be used for printing, since
                this is not a group by, a single row will be returned.
            
            2) Read through results row by row

            3) Iterate through the columns

            4) Match the result columns to the report columns

            5) Execute the required function
    */
	if(_results->rows.size() == 0)
		return ParseResult::FAILURE;

    // 1)
	vector<TempColumn*> reportRow = _results->rows.front();

    vector<TempColumn*> row;

    int avgCount = 0;

    // 2)
	for (size_t i = 0; i < _results->rows.size(); i++) { 
		row = (vector<TempColumn*>)_results->rows[i];
		if(row.size() < 1)
           continue;

        // 3)
        
        callFunctions(reportRow,row);
        avgCount++;
    }	
    resultList* functionResults = new resultList();
    for (size_t nbr = 0;nbr < reportRow.size();nbr++) 
    {
        if( reportRow.at(nbr)->functionType == t_function::AVG)
        {
            if(reportRow.at(nbr)->edit == t_edit::t_int)
                reportRow.at(nbr)->intValue = reportRow.at(nbr)->intValue / avgCount;
            if(reportRow.at(nbr)->edit == t_edit::t_double)
                reportRow.at(nbr)->doubleValue = reportRow.at(nbr)->doubleValue / avgCount;
        }
    }
    functionResults->addRow(reportRow);
    functionResults->print();
    return ParseResult::SUCCESS;
}

/******************************************************
 * Process Split
 ******************************************************/
ParseResult Plan::split(shared_ptr<char[]> _queryString)
{
    if(debug)
        printf("\n------------- split ----------------");
    int offset = 6;
    list<char*>lstDelimiters;
    char* queryString = _queryString.get();
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
    signed int found = lookup::findDelimiterFromList(queryString+offset,lstDelimiters);
    if(found == NEGATIVE)
    {
        if(debug)
            printf("\nNo delimiters found %s",queryString);
        queries.push_back(queryString);
        return ParseResult::SUCCESS;
    }
    while(found > 0)
    {
        //select * from orders Join Author 
        query = dupString(queryString);
        query[found+offset] = '\0';
        if(debug)
            printf("\n%s",query);
        queries.push_back(query);
        queryString = dupString(queryString+found+offset);
        found = lookup::findDelimiterFromList(queryString+offset,lstDelimiters);
    }
    if(strlen(queryString) > 0)
    {
        if(debug)
            printf("\ndrop though %s",queryString);
        queries.push_back(queryString);
    }
    return ParseResult::SUCCESS;
}
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

