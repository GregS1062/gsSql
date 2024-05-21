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
                statement 2 join on orders o where c.custid = o.custid group by c.zipCode
        2) Each statement will have one and only one table
        3) A given statement may contain columns for another statement, these must be resolved
    ----------------------------------------------------------------------------------------------*/
    // This will be a merged list of tables in the query including selects and joins
    //      It is required by bind because column names in the select may belong to one of the join
    //      tables
    list<char*>         lstDeclaredTables;
    //
    ParseQuery*         parseQuery;
    list<Statement*>    lstStatements;
    list<char*>         queries;
    list<iElements*>    lstElements;
    OrderBy*            orderBy     = nullptr;
    GroupBy*            groupBy     = nullptr; 
    ParseResult         printFunctions(resultList* _results);
    
    public:
        iSQLTables*     isqlTables = new iSQLTables();
        ParseResult     split(char*);
        ParseResult     prepare(char*);
        ParseResult     execute();
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

    if(split(querystr) == ParseResult::FAILURE)
        return ParseResult::FAILURE;

    parseQuery = new ParseQuery();

    /*
        All tables and all columns
    */
    for(char* query : queries)
    {
        if(parseQuery->parse(query) == ParseResult::FAILURE)
            return ParseResult::FAILURE;

        if(parseQuery->iElement->tableName != nullptr)
            lstDeclaredTables.push_back(parseQuery->iElement->tableName);
        
        if(parseQuery->iElement->lstColumns.size() == 0)
        {
            fprintf(traceFile,"\n nothing in column list ");
            break;
        }
        
        lstElements.push_back(parseQuery->iElement);
    }
            
    Binding* binding            = new Binding(isqlTables);
    binding->bindTableList(lstDeclaredTables);
    for(iElements* ielement : lstElements)
    {

        Statement* statement = binding->bind(ielement);
        if(statement != nullptr)
            lstStatements.push_back(statement);
        if(binding->orderBy != nullptr)
            orderBy                 = binding->orderBy;
        if(binding->groupBy != nullptr)
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

    resultList*		results;

    for(Statement* statement : lstStatements)
    {
        if(debug)
        {
            if(statement->table != nullptr)
                printTable(statement->table);
            if(orderBy != nullptr)
                printOrderBy(orderBy);
            if(groupBy != nullptr)
                printGroupBy(groupBy);
        }

        //Were any functions asked for?
        bool functions = false;
        for(Column* col : statement->table->columns)
        {
            if(col->functionType != t_function::NONE)
            {
                functions = true;
                break;
            }   
        } 
        
        if(engine->execute(statement) == ParseResult::FAILURE)
            return ParseResult::FAILURE;
        
        results = engine->results;

        if(results == nullptr)
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Results = null");
            return ParseResult::FAILURE;
        }

        if(groupBy == nullptr
        && functions)
        {
            if(debug)
                fprintf(traceFile,"\n------------- print functions ----------------");
            printFunctions(results);
            return ParseResult::SUCCESS;
        }

        
        if(orderBy != nullptr)
			if(orderBy->order.size() > 0)
				results->orderBy(orderBy);

		if(groupBy != nullptr)
			if(groupBy->group.size() > 0)
				results->groupBy(groupBy,functions);

		results->print();
		fprintf(traceFile,"\n %s",returnResult.resultTable.c_str());
    }
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
    functionResults->addRow(reportRow);
    functionResults->print();
    return ParseResult::SUCCESS;
}


/******************************************************
 * Process Split
 ******************************************************/
ParseResult Plan::split(char* _queryString)
{

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
    signed int found = lookup::findDelimiterFromList(_queryString+offset,lstDelimiters);
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

