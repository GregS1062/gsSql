#pragma once

#include <system_error>
#include <iostream>
#include <fstream>
#include <cstdint>
#include <filesystem>
#include <list>
#include "global.h"
#include "sqlClassLoader.h"
#include "parseSql.h"

using namespace std;

class sqlEngine
{
	/***************************************
	 * Assuming a single table for now
	*******************************************/
	cTable* 		queryTable;
	fstream* 		tableStream;
	list<column*>	queryColumn;
	char* line;

	public:
		sqlParser* 	query;

		sqlEngine(sqlParser*, cTable*);
		ParseResult open();
		ParseResult close();
		ParseResult	getConditionColumns();
		ParseResult queryContitionsMet();
		ParseResult update();
		ParseResult compareLike(Condition*);
		ParseResult compareEqual(Condition*);
		ParseResult insert();
		string 		select();
		char* 		getRecord(long, fstream*, int);
		long		appendRecord(void*, fstream*, int);
		bool 		writeRecord(void*, long, fstream*, int);


};

sqlEngine::sqlEngine(sqlParser* _query, cTable* _table)
{
    /***************************************
	 * Assuming a single table for now
	*******************************************/

		query 		= _query;
		queryTable  = _table;
}
/******************************************************
 * Open
 ******************************************************/
ParseResult sqlEngine::open()
{
		////Open data file
		tableStream 	= queryTable->open();
		if(tableStream == nullptr)
			return ParseResult::FAILURE;
		
		if(tableStream->is_open())
			return ParseResult::SUCCESS;
		
		return ParseResult::FAILURE;
}
/******************************************************
 * Load Close
 ******************************************************/
ParseResult sqlEngine::close()
{
	tableStream->close();
	return ParseResult::SUCCESS;
}
/******************************************************
 * Get Condition Columns
 ******************************************************/
ParseResult sqlEngine::getConditionColumns()
{
	column* col;
	for(Condition* condition : query->conditions)
	{
		if(condition == nullptr)
			return ParseResult::FAILURE;
		
		col = queryTable->getColumn(condition->name);

		if(col == nullptr)
		{
			errText.append(" condition column ");
			errText.append(condition->name);
			errText.append(" not found ");
			query->rowsToReturn = 1;
			return ParseResult::FAILURE;

		}

		condition->col = col;
	}
	return ParseResult::SUCCESS;
}
/******************************************************
 * Compare like
 ******************************************************/
ParseResult sqlEngine::compareLike(Condition* _condition)
{
	//Like condition compares the condition value length to the record
	// so "sch" = "schiller" because only three characters are being compared
	
	char buffRecord[60];
	column* column = _condition->col;
	strncpy(buffRecord, line+column->position, strlen(_condition->value));
	buffRecord[strlen(_condition->value)] = '\0';
		
	if(query->compareCaseInsensitive(_condition->value,buffRecord))
	{
		return ParseResult::SUCCESS;
	}

	return ParseResult::FAILURE;
}
/******************************************************
 * Compare Equal
 ******************************************************/
ParseResult sqlEngine::compareEqual(Condition* _condition)
{
	//Equal condition compares the full column length to the record
	// so "sch" != "schiller" 

	char buffRecord[60];
	column* column = _condition->col;
	strncpy(buffRecord, line+column->position, column->length);
			buffRecord[column->length] = '\0';
		
	if(query->compareCaseInsensitive(_condition->value,buffRecord))
	{
		return ParseResult::SUCCESS;
	}

	return ParseResult::FAILURE;
}
/******************************************************
 * Query Conditions Met
 ******************************************************/
ParseResult sqlEngine::queryContitionsMet()
{
	//Nothing to see, move along
	if(query->conditions.size() == 0)
		return ParseResult::SUCCESS;

	ParseResult queryResult = ParseResult::FAILURE;
	ParseResult queryResults = ParseResult::FAILURE;

	for(Condition* condition : query->conditions)
	{

		if(query->compareCaseInsensitive(condition->op,"like"))
		{
			queryResult = compareLike(condition);
		}
		else
		{
			if(query->compareCaseInsensitive(condition->op,"="))
			{
				queryResult = compareEqual(condition);
			}
		}
		/*errText.append(" ");
		errText.append(std::to_string(queryResult));
		errText.append(" ");
		errText.append(std::to_string( query->conditions.size()));
		errText.append(" ");
		errText.append(condition->colName);
		errText.append(" ");
		errText.append(condition->op);
		errText.append(" ");
		errText.append(condition->value);*/

		if(query->conditions.size() == 1)
			return queryResult;

		if(!query->compareCaseInsensitive(condition->condition,"AND")
		&& !query->compareCaseInsensitive(condition->condition,"OR")
		&& query->conditions.size() == 1
		&& queryResult == ParseResult::FAILURE)
			return ParseResult::FAILURE;

		if(query->compareCaseInsensitive(condition->condition,"AND")
		&& queryResult == ParseResult::FAILURE)
			return ParseResult::FAILURE;
		
		if(query->compareCaseInsensitive(condition->condition,"AND")
		&& queryResult  == ParseResult::SUCCESS
		&& queryResults == ParseResult::SUCCESS)
			return ParseResult::SUCCESS;

		if(query->compareCaseInsensitive(condition->condition,"OR")
		&& (queryResult == ParseResult::SUCCESS
		|| queryResults == ParseResult::SUCCESS))
			return ParseResult::SUCCESS;

		queryResults = queryResult;		
	}

	return ParseResult::FAILURE;
}

/******************************************************
 * Update Record
 ******************************************************/
ParseResult sqlEngine::update()
{
	int recordPosition 	= 0;
	int rowCount 		= 0;
	column* col;

	if(getConditionColumns() == ParseResult::FAILURE)
	{
		errText.append( " Query condition failure");
		return ParseResult::FAILURE;
	};

	map<char*,column*>::iterator itr;
	map<char*,column*>columns = query->queryTable->columns;

	//table scan for now
	while(true)
	{
		line = getRecord(recordPosition,tableStream, queryTable->recordLength);
		if(line == nullptr)
			break;

		if(queryContitionsMet() == ParseResult::SUCCESS)
		{
			for (itr = columns.begin(); itr != columns.end(); ++itr) 
			{
				col = (column*)itr->second;
				memmove(&line[col->position], col->value, col->length);	
			}
			if(!writeRecord(line,recordPosition,tableStream,queryTable->recordLength))
			{
				errText.append(" Failed to update record");
				return ParseResult::FAILURE;
			}
			rowCount++;
		}
		recordPosition = recordPosition + queryTable->recordLength;
	}
	returnResult.message.append(std::to_string(rowCount));
	returnResult.message.append(" rows updated");
	return ParseResult::SUCCESS;
}
/******************************************************
 * Fetch Row
 ******************************************************/
string sqlEngine::select()
{
	string rowResponse;
	string header;
	int recordPosition 	= 0;
	int resultCount 	= 0; 
	char buff[60];

	if(getConditionColumns() == ParseResult::FAILURE)
	{
		errText.append( " Query condition failure");
		return "";
	};
	int sumOfColumnSize = 0;

	column* col;
	map<char*,column*>::iterator itr;
	map<char*,column*>columns = query->queryTable->columns;
	for (itr = columns.begin(); itr != columns.end(); ++itr) {
            col = (column*)itr->second;
			sumOfColumnSize = sumOfColumnSize + col->length;
    }
	
	double percentage = 0;

	header.append(rowBegin);
	for (itr = columns.begin(); itr != columns.end(); ++itr) 
	{
		col = (column*)itr->second;
		header.append("\n\t");
		header.append(hdrBegin);
		header.append(" style="" width:");
		percentage = (double)col->length / sumOfColumnSize * 100;
		header.append(to_string((int)percentage));
		header.append("%"">");
		header.append(col->name);
		header.append(hdrEnd);

	}
	header.append(rowEnd);

	rowResponse.append(header);
	int rowCount = 0;

	while(true)
	{
		line = getRecord(recordPosition,tableStream, queryTable->recordLength);
		if(line == nullptr)
			break;

		if(query->rowsToReturn > 0
		&& rowCount >= query->rowsToReturn)
			break;

		if(queryContitionsMet() == ParseResult::SUCCESS)
		{
			resultCount++;
			rowResponse.append("\n\t\t");
			rowResponse.append(rowBegin);
			for (itr = columns.begin(); itr != columns.end(); ++itr) 
			{
				col = (column*)itr->second;
				rowResponse.append(cellBegin);
				strncpy(buff, line+col->position, col->length);
				buff[col->length] = '\0';
				rowResponse.append(buff);
				rowResponse.append(cellEnd);
			}
			rowResponse.append(rowEnd);
			rowCount++;
		}
		recordPosition = recordPosition + queryTable->recordLength;
	}
	returnResult.message.append(std::to_string(resultCount));
	returnResult.message.append(" rows returned ");
	return rowResponse;
}
/******************************************************
 * Store Record
 ******************************************************/
ParseResult sqlEngine::insert()
{
	char* buff = (char*)malloc(queryTable->recordLength);
	size_t count = 0;
	
	column* col;
	map<char*,column*>::iterator itr;
	map<char*,column*>columns = query->queryTable->columns;
	for (itr = columns.begin(); itr != columns.end(); ++itr) {
        col = (column*)itr->second;
		memmove(&buff[col->position], col->value, col->length);
		count++;
	}

	long recordNumber = appendRecord(buff, tableStream, queryTable->recordLength);
	if(recordNumber > 0)
	{
		returnResult.message.append(" 1 record inserted at location ");
		returnResult.message.append(std::to_string(recordNumber));
		return ParseResult::SUCCESS;
	};
	errText.append(" location 0 - insert failed");
	return ParseResult::FAILURE;
}

/******************************************************
 * Get Record
 ******************************************************/
char* sqlEngine::getRecord(long _address, fstream* _file, int _size)
{
	try
	{
		if (_size < 1)
		{
			return nullptr;
		}

		char* ptr = (char*)malloc(_size);

		_file->clear();

		if (!_file->seekg(_address))
		{
			free(ptr);
			return nullptr;
		}

		if (!_file->read((char*)ptr, _size))
			ptr = nullptr;
		
		
		_file->flush();

		return ptr;
	}
    catch(const std::exception& e)
    {
        errText.append( e.what());
        return nullptr;
    } 
}
/******************************************************
 * Append Record
 ******************************************************/
long sqlEngine::appendRecord(void* _ptr, fstream* _file, int _size)
{
	try
	{
		_file->clear();
		_file->seekg(0, _file->end);
		long eof = _file->tellg();
		if (!_file->write((char*)_ptr, _size))
		{
			errText.append("Write to file failed");
			eof = 0;
		}
			

		_file->flush();

		return eof;
	}
	catch(const std::exception& e)
    {
        errText.append( e.what());
        return 0;
    } 
}
/*-----------------------------------------------------------
	Write record
-------------------------------------------------------------*/
bool sqlEngine::writeRecord(void* _ptr, long _address, fstream* _file, int _size)
{
	try
	{ 
		_file->clear();
		if (!_file->seekp(_address))
			return false;

		if (!_file->write((char*)_ptr, _size))
			return false;

		_file->flush();
		return true;
	}
	catch(const std::exception& e)
    {
        errText.append( e.what());
        return 0;
    } 
	
}
