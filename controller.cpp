
#pragma once
#include <list>
#include <vector>
#include "defines.h"
#include "interfaces.h"
#include "sqlCommon.h"
#include "prepareQuery.cpp"
#include "parseQuery.cpp"
#include "binding.cpp"
#include "execute.cpp"
#include "printDiagnostics.cpp"

    /*--------------------------------------------------------------------------------------------
        The controller will accept a query and marshal and sequence all of the resources and processes 
            from the begining to the logical end
      --------------------------------------------------------------------------------------------
        
        Accept and validate
        1) Accept request
        2) Remove whitespace outside of quotes: tabs, carriage returns
        3) Normalize: eliminate multiple spaces or insert spaces around keywords and operators
        4) Validate that query is well-formed: quotes and parenthesis must be balanced, 
            and keywords have logical pairs: select/from, update/set, insert/values

        Prepare
        1) Divide query into subqueries: select, join, subquery
        2) Divide subqueries into clauses
        3) Divide clauses into elements
        4) bind elements into tables, columns, functions and values
        5) Bind tables, columns, functions and values into statements 

        Plan
        6) Identify most efficient index or table-scan
        7) Order statements for most efficient processing
        
        Execute
        8) Execute the plan steps

        Output
        9) Determine presentation (html or text)
        10) Print headers
        11) Proportion output
        12) Return response
    */
/******************************************************
 * Controller
 ******************************************************/
class controller
{
    list<string>                    lstDeclaredTables;
    list<shared_ptr<iElements>>     lstElements;
    list<shared_ptr<sTable>>        lstTables;
    list<shared_ptr<Statement>>     lstStatements; 
    list<string>                    queries;
    shared_ptr<iSQLTables>          isqlTables;
    shared_ptr<OrderBy>             orderBy;
    shared_ptr<GroupBy>             groupBy; 
    list<shared_ptr<columnParts>>   reportColumns;
    
    ParseResult                     prepare(string);
    ParseResult                     getElements(string);
    ParseResult                     bindElements();
    ParseResult                     buildTableList(string);
    
    public:
        controller(shared_ptr<iSQLTables>);
        ParseResult                     runQuery(string);
};
/******************************************************
 * Constructor
 ******************************************************/
controller::controller(shared_ptr<iSQLTables> _isqlTables)
{
    isqlTables = _isqlTables;
}
/******************************************************
 * Run Query
 ******************************************************/
ParseResult controller::runQuery(string _queryString)
{
    try
    {
        if(prepare(_queryString) == ParseResult::FAILURE)
            return ParseResult::FAILURE;

        execute execute;
        execute.lstStatements   = lstStatements;
        execute.groupBy         = groupBy;
        execute.orderBy         = orderBy;
        execute.reportColumns   = reportColumns;
        return execute.ExecuteQuery();    
    }
    catch_and_throw_to_caller
}
/******************************************************
 * Prepare
 ******************************************************/
ParseResult controller::prepare(string _queryString)
{
    try
    {
        if(getElements(_queryString) == ParseResult::FAILURE)
            return ParseResult::FAILURE;

        if(bindElements() == ParseResult::FAILURE)
            return ParseResult::FAILURE;

        return ParseResult::SUCCESS;
    }
    catch_and_throw_to_caller
}
/******************************************************
 * Bind Elements
 ******************************************************/
ParseResult controller::bindElements()
{          
    // 5)
    try
    {
        for(shared_ptr<iElements> ielement : lstElements)
        {
            Binding binding             = Binding(isqlTables);
            binding.lstTables           = lstTables;
            
            // 6)

            lstStatements.push_back(binding.bind(ielement));
            
            if(binding.orderBy != nullptr )
                orderBy                 = binding.orderBy;
            
            if(binding.groupBy != nullptr)
                groupBy                 = binding.groupBy;
            
            if(binding.reportColumns.size() > 0)
                reportColumns           = binding.reportColumns;
        }            
        return ParseResult::SUCCESS;
    }
    catch_and_throw_to_caller
}
/******************************************************
 * Get Elements
 ******************************************************/
ParseResult controller::getElements(string _queryString)
{
    try
    {
       // 1)
        string querystr = normalizeQuery(_queryString,MAXSQLSTRINGSIZE);
        if( querystr.empty())
            return ParseResult::FAILURE;

        // 2)
        queries = split(querystr);
        if(queries.size() == 0)
            return ParseResult::FAILURE;

        // 3 And 4)
        for(string query : queries)
        {
            ParseQuery parseQuery;
            if(parseQuery.parse(query) == ParseResult::FAILURE)
                return ParseResult::FAILURE;

            if(!parseQuery.ielements->tableName.empty())
                buildTableList(parseQuery.ielements->tableName);
            
            lstElements.push_back(parseQuery.ielements);
        }
        return ParseResult::SUCCESS;
    }
    catch_and_throw_to_caller
}
/******************************************************
 * Bind Table List
 ******************************************************/
ParseResult controller::buildTableList(string _tableName)
{
    try
    {
        shared_ptr<sTable> table;
        if(_tableName.empty())
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Table name is null");
            return ParseResult::FAILURE;
        }

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
