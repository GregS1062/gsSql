#include "defines.h"
#include "sqlCommon.h"
#include "sqlSelectEngine.cpp"
#include "sqlJoinEngine.cpp"
#include "sqlModifyEngine.cpp"
#include "display.cpp"
#include "printDiagnostics.cpp"
/******************************************************
 * Execute
 ******************************************************/
class execute
{
    ParseResult printFunctions(shared_ptr<response>);
    
    public:

    vector<shared_ptr<Statement>>         lstStatements;
    shared_ptr<GroupBy>                 groupBy;
    shared_ptr<OrderBy>                 orderBy;
    shared_ptr<response>		        results;
    list<shared_ptr<columnParts>>       reportColumns;
    
    ParseResult ExecuteQuery();
   
};
/******************************************************
 * Execute Query
 ******************************************************/
ParseResult execute::ExecuteQuery()
{
    try
    {
        
        if(lstStatements.size() == 0)
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Bind produced no statements");
            return ParseResult::FAILURE;
        }

        display display;
        bool functions = false;

        if(debug)
                printStatements(lstStatements);
        
        for(shared_ptr<Statement> statement : lstStatements)
        {
            if(statement == nullptr)
                return ParseResult::FAILURE;

            //Does statement contain a function?
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
                    break;
                }

                case SQLACTION::NOACTION:
                {
                    sendMessage(MESSAGETYPE::ERROR,presentationType,false,"Invalid SQL Action");
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
                    break;
                }
                case SQLACTION::UPDATE:
                {
                    sqlModifyEngine sqlModify;
                    if(sqlModify.update(statement) == ParseResult::FAILURE)
                        return ParseResult::FAILURE;
                    break;
                }
                case SQLACTION::DELETE:
                {

                    //    NOTE: Update and Delete use the same logic

                    sqlModifyEngine sqlModify;
                    if(sqlModify.update(statement) == ParseResult::FAILURE)
                        return ParseResult::FAILURE;
                    break;
                }
                case SQLACTION::INVALID:
                {
                    sendMessage(MESSAGETYPE::ERROR,presentationType,false,"Invalid SQL Action");
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
                    break;
                }
                case SQLACTION::RIGHT:
                {
                    sendMessage(MESSAGETYPE::ERROR,presentationType,false,"Joins not implemented at this time");
                    break;
                }
                case SQLACTION::INNER:
                {
                    sendMessage(MESSAGETYPE::ERROR,presentationType,false,"Joins not implemented at this time");
                    break;
                }
                case SQLACTION::OUTER:
                {
                    sendMessage(MESSAGETYPE::ERROR,presentationType,false,"Joins not implemented at this time");
                    break;
                }
                case SQLACTION::FULL:
                {
                    sendMessage(MESSAGETYPE::ERROR,presentationType,false,"Joins not implemented at this time");
                    break;
                }
                case SQLACTION::NATURAL:
                {
                    sendMessage(MESSAGETYPE::ERROR,presentationType,false,"Joins not implemented at this time");
                    break;
                }
                case SQLACTION::CROSS:
                {
                    sendMessage(MESSAGETYPE::ERROR,presentationType,false,"Joins not implemented at this time");
                    break;
                }
            }

        }
        if(results == nullptr)
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Results = null");
            return ParseResult::FAILURE;
        }

        if(groupBy != nullptr)
        {
            if(groupBy->group.size() == 0
            && functions)
            {
                printFunctions(results);
                return ParseResult::SUCCESS;
            }

            if(groupBy->group.size() > 0)
                results->groupBy(groupBy,functions);
        }
        
        if(orderBy != nullptr)
            if(orderBy->order.size() > 0)
                 results->orderBy(orderBy);
            
        display.displayResponse(results->rows,reportColumns);
        return ParseResult::SUCCESS;
    }
    catch_and_throw_to_caller
    
}
/******************************************************
 * Print Functions
 ******************************************************/
ParseResult execute::printFunctions(shared_ptr<response> _results)
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
    shared_ptr<response> functionResults = make_shared<response>();
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

    display display;
    display.displayResponse(functionResults->rows,reportColumns); 
    return ParseResult::SUCCESS;
}