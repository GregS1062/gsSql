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
#include "functions.cpp"
#include "printDiagnostics.cpp"

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
    list<string>                lstDeclaredTables;
    list<string>                queries;
    list<shared_ptr<iElements>> lstElements;
    list<Column>                reportColumns;
    shared_ptr<iSQLTables>      isqlTables;

    ParseResult                 printFunctions(shared_ptr<tempFiles> _results);
    ParseResult                 getElements(string);
    ParseResult                 bindElements();
    
    public:
        Plan(shared_ptr<iSQLTables>);
        ParseResult             split(string);
        ParseResult             prepare(string);
        ParseResult             execute();
        ParseResult             determineExecutionOrder();
        ParseResult             buildTableList(string);

        //public for diagnostic purposes
        list<shared_ptr<sTable>>            lstTables;
        list<shared_ptr<Statement>>         lstStatements; 
        shared_ptr<OrderBy>                 orderBy;
        shared_ptr<GroupBy>                 groupBy; 
};
Plan::Plan(shared_ptr<iSQLTables> _isqlTables)
{
    isqlTables = _isqlTables;
}
/******************************************************
 * Prepare
 ******************************************************/
ParseResult Plan::prepare(string _queryString)
{
    if(getElements(_queryString) == ParseResult::FAILURE)
        return ParseResult::FAILURE;

    if(bindElements() == ParseResult::FAILURE)
        return ParseResult::FAILURE;

    return ParseResult::SUCCESS;
}
/******************************************************
 * Bind Elements
 ******************************************************/
ParseResult Plan::bindElements()
{          
    // 5)
    for(shared_ptr<iElements> ielement : lstElements)
    {
        Binding binding             = Binding(isqlTables);
        binding.lstTables           = lstTables;
        
        if(debug)
            fprintf(traceFile,"\n iElement tablename %s",ielement->tableName.c_str());
        // 6)

        lstStatements.push_back(binding.bind(ielement));
        if(binding.orderBy != nullptr )
            orderBy                 = binding.orderBy;
        if(binding.groupBy != nullptr)
            groupBy                 = binding.groupBy;

    } 
        
    return ParseResult::SUCCESS;
}
/******************************************************
 * Get Elements
 ******************************************************/
ParseResult Plan::getElements(string _queryString)
{
       // 1)
    string querystr = normalizeQuery(_queryString,MAXSQLSTRINGSIZE);
    if( querystr.empty())
        return ParseResult::FAILURE;

    // 2)
    if(split(querystr) == ParseResult::FAILURE)
        return ParseResult::FAILURE;

    // 3 And 4)
    for(string query : queries)
    {
        if(debug)
        {
            fprintf(traceFile,"\nSplit %s",query.c_str());
            fprintf(traceFile,"\n-----------------------------------------------------");
        }
        ParseQuery parseQuery;
        if(parseQuery.parse(query) == ParseResult::FAILURE)
            return ParseResult::FAILURE;

        if(!parseQuery.ielements->tableName.empty())
            buildTableList(parseQuery.ielements->tableName);
        
       /*  if(parseQuery.ielements->lstColumns.size() == 0
        && parseQuery.ielements->lstValues.size() == 0
        && parseQuery.ielements->sqlAction == SQLACTION::SELECT)
        {
            fprintf(traceFile,"\n nothing in column list ");
            break;
        } */
        lstElements.push_back(parseQuery.ielements);
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

     shared_ptr<tempFiles>		results;

    for(shared_ptr<Statement> statement : lstStatements)
    {
        if(statement == nullptr)
            return ParseResult::FAILURE;
            
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
        for(shared_ptr<Column> col : statement->table->columns)
        {
            if(col->functionType != t_function::NONE)
            {
                functions = true;
                break;
            }   
        } 

        switch(statement->action)
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
                    return ParseResult::FAILURE;
                return ParseResult::SUCCESS;
                break;
            }
            case SQLACTION::UPDATE:
            {
                sqlModifyEngine sqlModify;
                if(sqlModify.update(statement) == ParseResult::FAILURE)
                    return ParseResult::FAILURE;
                return ParseResult::SUCCESS;
                break;
            }
            case SQLACTION::DELETE:
            {

                //    NOTE: Update and Delete use the same logic

                sqlModifyEngine sqlModify;
                if(sqlModify.update(statement) == ParseResult::FAILURE)
                    return ParseResult::FAILURE;
                return ParseResult::SUCCESS;
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

      if(groupBy->group.size() == 0
        && functions)
      if(functions)
        {
            if(debug)
                fprintf(traceFile,"\n------------- print functions ----------------");
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
ParseResult Plan::printFunctions(shared_ptr<tempFiles> _results)
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
	vector<shared_ptr<TempColumn>> reportRow = _results->rows.front();

    vector<shared_ptr<TempColumn>> row;

    int avgCount = 0;

    // 2)
	for (size_t i = 0; i < _results->rows.size(); i++) { 
		row = (vector<shared_ptr<TempColumn>>)_results->rows[i];
		if(row.size() < 1)
           continue;

        // 3)
        
        callFunctions(reportRow,row);
        avgCount++;
    }	
    tempFiles* functionResults = new tempFiles();
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
ParseResult Plan::split(string _queryString)
{
    try
    {
        
        if(debug)
            fprintf(traceFile,"\n------------- split ----------------");
        
        string queryString = _queryString;

        size_t joinFound = isJoin(queryString);

        if(joinFound == std::string::npos)
        {
            queries.push_back(queryString);
            return ParseResult::SUCCESS;
        }
        else
        {
            queries.push_back(queryString.substr(0,joinFound));
            //return portion of string AFTER join
            queryString = snipString(queryString,joinFound);
        }

        size_t posJoin = findKeyword(queryString,sqlTokenJoin);
        size_t posInnerJoin = findKeyword(queryString,sqlTokenInnerJoin);
        size_t posOuterJoin = findKeyword(queryString,sqlTokenOuterJoin);
        size_t posLeftJoin = findKeyword(queryString,sqlTokenLeftJoin);
        size_t posRightJoin = findKeyword(queryString,sqlTokenRightJoin);

        list<size_t> stack;
        if(posJoin != std::string::npos)
            stack.push_back(posJoin);
        if(posInnerJoin != std::string::npos)
            stack.push_back(posInnerJoin);
        if(posOuterJoin != std::string::npos)
            stack.push_back(posOuterJoin);
        if(posLeftJoin != std::string::npos)
            stack.push_back(posLeftJoin);
        if(posRightJoin != std::string::npos)
            stack.push_back(posRightJoin);

        stack.sort();
        stack.reverse();

        for(size_t join : stack)
        {
            //return portion of string AFTER join
            queryString = snipString(queryString,join);
            queries.push_back(queryString);
            //erase portion of string AFTER join
            queryString = clipString(queryString,join);
        }

        return ParseResult::SUCCESS;
    }
    catch_and_trace
    return ParseResult::FAILURE;
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
/******************************************************
 * Bind Table List
 ******************************************************/
ParseResult Plan::buildTableList(string _tableName)
{
    try
    {
        shared_ptr<sTable> table;
        if(_tableName.empty())
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Table name is null");
            return ParseResult::FAILURE;
        }
        if(debug)
            fprintf(traceFile,"\n table name %s|| \n",_tableName.c_str());

        table = make_shared<sTable>();

        _tableName = trim(_tableName);
        string tableName{};
        string aliasTableName{};

        //Note: table name pattern  NANE ALIAS (customers c)
        size_t posSpace = _tableName.find(SPACE);
        if(posSpace == std::string::npos)
        {
            // case 3) simple name         - surname
            tableName = _tableName;
        }
        else
        {
            tableName = clipString(_tableName,posSpace);
            aliasTableName = snipString(_tableName,posSpace+1);
        }


        shared_ptr<sTable> temp =  make_shared<sTable>();
        temp = getTableByName(isqlTables->tables,tableName);
        if(temp != nullptr)
        {
            table->name         = tableName;
            table->fileName     = temp->fileName;
            table->alias        = aliasTableName;
            table->recordLength = temp->recordLength;
            for(shared_ptr<sIndex> idx : temp->indexes)
            {
                table->indexes.push_back(idx);
            }
            lstTables.push_back(table);
        }


        if(lstTables.size() == 0)
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true," Binding expecting at least one table");
            return ParseResult::FAILURE;
        }

        return ParseResult::SUCCESS;
    }
    catch_and_trace
    return ParseResult::FAILURE;
}


