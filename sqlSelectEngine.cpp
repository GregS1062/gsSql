#pragma once
#include "defines.h"
#include "sqlCommon.h"
#include "sqlEngine.cpp"
#include "tempFiles.cpp"
/******************************************************
 * SQL SELECT ENGINE
 ******************************************************/
class sqlSelectEngine : public sqlEngine
{
    /*
        Engine to select records
    */

   public:

   ParseResult		execute			(shared_ptr<Statement>);
   ParseResult		tableScan		(list<shared_ptr<Column>>);
   ParseResult		searchForward	(Search*, string, size_t , SEARCH);
   ParseResult 		searchBack		(Search*, string, size_t);
};
/******************************************************
 * Select
 ******************************************************/
ParseResult sqlSelectEngine::execute(shared_ptr<Statement> _statement)
{

	if(debug)
        fprintf(traceFile,"\n\n-------------------------BEGIN SELECT ENGINE-------------------------------------------");

	statement = _statement;
	
	if(statement == nullptr)
	{
		sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Statement is null");
		return ParseResult::FAILURE;
	}
	
	if(open() == ParseResult::FAILURE)
	{
		return ParseResult::FAILURE;
	}

	//checks the index columns against the queryColumns
	shared_ptr<searchIndexes> searchOn = determineIndex();

	//query condition but not on an indexed column
	if(searchOn == nullptr)
	{
		if(tableScan(statement->table->columns) == ParseResult::FAILURE)
			return ParseResult::FAILURE;

		return ParseResult::SUCCESS;
	}
	
	SEARCH searchPath = indexRead(searchOn);
	if(searchPath == SEARCH::FAILED)
		return ParseResult::FAILURE;

	//TODO assuming single column searches for this sprint
	shared_ptr<Column> col = searchOn->col.front();

	if(searchPath == SEARCH::BACK)
		return searchBack(search,col->value, statement->rowsToReturn);
	
	return searchForward(search,col->value, statement->rowsToReturn,searchPath);

}
/******************************************************
 * Table Scan
 ******************************************************/
ParseResult sqlSelectEngine::tableScan(list<shared_ptr<Column>> _columns)
{
/*
	Input: list of columns to be read
	Ouput: Columns of data added to rows list
*/

	int rowCount = 0;
	int recordPosition 	= 0;

	while(true)
	{
		
		line = getRecord(recordPosition,tableStream, statement->table->recordLength);

		//End of File
		if(line == nullptr)
			break;

		if(isRecordDeleted(false))
		{
			recordPosition = recordPosition + statement->table->recordLength;
			continue;
		}

		//select top n
		if(statement->rowsToReturn > 0
		&& rowCount >= statement->rowsToReturn)
			break;

		if(queryContitionsMet(statement->table->conditions, line) == ParseResult::SUCCESS)
		{

			results->addRow(outputLine(_columns));
			rowCount++;

		}

		recordPosition = recordPosition + statement->table->recordLength;
	}
	sendMessage(MESSAGETYPE::INFORMATION,presentationType,true,"Table scan: rows scanned ");
	sendMessage(MESSAGETYPE::INFORMATION,presentationType,false,std::to_string(rowCount).c_str());
	rowsReturnedMsg();

	//Close File
	close();

	return ParseResult::SUCCESS;
}

/******************************************************
 * Search Forward
 ******************************************************/
ParseResult sqlSelectEngine::searchForward(Search* _search, string _valueStr, size_t _rowsToReturn, SEARCH _op)
{

	if(_rowsToReturn == 0)
	{
		sendMessage(MESSAGETYPE::INFORMATION,presentationType,true,"TOP N rows not defined. Default to ",std::to_string(MAXROWSRETURNED));
		_rowsToReturn = MAXROWSRETURNED;
	}

	char* _value = (char*)_valueStr.c_str();
	QueryResultList* searchResults = _search->findRange(_value, (int)_rowsToReturn, _op);
	
	while(searchResults != nullptr)
	{
		line = getRecord(searchResults->location,tableStream, statement->table->recordLength);
		
		//End of File
		if(line == nullptr)
			break;

		//select top n
		if(_rowsToReturn > 0
		&& results->rows.size() >= _rowsToReturn)
			break;
		
		if(queryContitionsMet(statement->table->conditions, line) == ParseResult::SUCCESS)
			results->addRow(outputLine(statement->table->columns));

		searchResults = searchResults->next;
	}

	rowsReturnedMsg();

	//Close File
	close();

	return ParseResult::SUCCESS;
}
/******************************************************
 * Search Back
 ******************************************************/
ParseResult sqlSelectEngine::searchBack(Search* _search, string _value, size_t _rowsToReturn)
{
	char* __value = (char*)_value.c_str();
	long location		= 0;
	char* key;
	Node* _leaf = _search->findLeafBase(__value);
	ScrollNode* scrollNode = _search->scrollIndexBackward(_leaf,__value);
	key = __value;

	while(true)
	{
		scrollNode = _search->scrollIndexBackward(scrollNode->leaf,key);
		
		if(scrollNode == nullptr)
			break;
		
		location = _search->getLocation(scrollNode->leaf,scrollNode->position);
		_search->getKeyFromPosition(scrollNode->leaf, key, scrollNode->position);
		
		line = getRecord(location,tableStream, statement->table->recordLength);
		
		//End of File
		if(line == nullptr)
			break;

		//select top n
		if(_rowsToReturn > 0
		&& results->rows.size() >= _rowsToReturn)
			break;

		if(queryContitionsMet(statement->table->conditions, line) == ParseResult::SUCCESS)
		{
			results->addRow(outputLine(statement->table->columns));
		}
	}

	rowsReturnedMsg();

	//Close File
	close();

	return ParseResult::SUCCESS;
}


