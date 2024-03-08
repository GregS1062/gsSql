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
		ParseResult ValidateQueryColumns();
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
 * Select Query Columns
 ******************************************************/
ParseResult sqlEngine::ValidateQueryColumns()
{
	if(query->queryColumn.size() == 0)
	{
		errText.append(" No columns detected ");
		return ParseResult::FAILURE;
	}

	column* col;
	 if(strcmp(query->queryColumn[0],sqlTokenAsterisk) == 0)
	{
		for (queryTable->columnItr = queryTable->columns.begin(); queryTable->columnItr != queryTable->columns.end(); ++queryTable->columnItr) 
		{
        	col = (column*)queryTable->columnItr->second;
            queryColumn.push_back(col);
    	}
		return ParseResult::SUCCESS;
	}

	for(char* value : query->queryColumn)
	{
		col = queryTable->getColumn( value);
		if(col == nullptr)
		{
			errText.append(" ");
			errText.append(value);
			errText.append(" not found ");
			return ParseResult::FAILURE;
		}
		queryColumn.push_back(col);
	}
        
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
		
		col = queryTable->getColumn(condition->ColumnName);

		if(col == nullptr)
		{
			errText.append(" condition column ");
			errText.append(condition->ColumnName);
			errText.append(" not found ");
			query->rowsToReturn = 1;
			return ParseResult::FAILURE;

		}

		condition->Column = col;
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
	column* column = _condition->Column;
	strncpy(buffRecord, line+column->position, strlen(_condition->Value));
	buffRecord[strlen(_condition->Value)] = '\0';
		
	if(query->compareCaseInsensitive(_condition->Value,buffRecord))
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
	column* column = _condition->Column;
	strncpy(buffRecord, line+column->position, column->length);
			buffRecord[column->length] = '\0';
		
	if(query->compareCaseInsensitive(_condition->Value,buffRecord))
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

		if(query->compareCaseInsensitive(condition->Operator,"like"))
		{
			queryResult = compareLike(condition);
		}
		else
		{
			if(query->compareCaseInsensitive(condition->Operator,"="))
			{
				queryResult = compareEqual(condition);
			}
		}
		/*errText.append(" ");
		errText.append(std::to_string(queryResult));
		errText.append(" ");
		errText.append(std::to_string( query->conditions.size()));
		errText.append(" ");
		errText.append(condition->ColumnName);
		errText.append(" ");
		errText.append(condition->Operator);
		errText.append(" ");
		errText.append(condition->Value);*/

		if(query->conditions.size() == 1)
			return queryResult;

		if(!query->compareCaseInsensitive(condition->Condition,"AND")
		&& !query->compareCaseInsensitive(condition->Condition,"OR")
		&& query->conditions.size() == 1
		&& queryResult == ParseResult::FAILURE)
			return ParseResult::FAILURE;

		if(query->compareCaseInsensitive(condition->Condition,"AND")
		&& queryResult == ParseResult::FAILURE)
			return ParseResult::FAILURE;
		
		if(query->compareCaseInsensitive(condition->Condition,"AND")
		&& queryResult  == ParseResult::SUCCESS
		&& queryResults == ParseResult::SUCCESS)
			return ParseResult::SUCCESS;

		if(query->compareCaseInsensitive(condition->Condition,"OR")
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

	for(ColumnValue* colVal : query->columnValue)
	{
		col = queryTable->getColumn(colVal->ColumnName);
		if(col == nullptr)
		{
			errText.append(colVal->ColumnName);
			errText.append(" not found in table.");
			return ParseResult::FAILURE;
		}

		colVal->Column = col;
		if(strlen(colVal->ColumnValue) > (size_t)col->length)
		{
			errText.append(colVal->ColumnName);
			errText.append(" value length > column edit size.");
			return ParseResult::FAILURE;
		}

	}

	//table scan for now
	while(true)
	{
		line = getRecord(recordPosition,tableStream, queryTable->recordLength);
		if(line == nullptr)
			break;

		if(queryContitionsMet() == ParseResult::SUCCESS)
		{
			for(ColumnValue* colVal : query->columnValue)
			{
				memmove(&line[colVal->Column->position], colVal->ColumnValue, colVal->Column->length);	
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
	auto it = query->queryTables.begin();
    std::advance(it, 0);
    cTable* table = (cTable*)it->second;
	map<char*,column*>columns = table->columns;
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
	char* value;
	
	if(queryColumn.size() == 1)
	{
		if(query->queryValue.size() != queryTable->columns.size())
		{
			errText.append(" value count != column count - insert failed");
			return ParseResult::FAILURE;
		}

		for (queryTable->columnItr = queryTable->columns.begin(); 
			queryTable->columnItr != queryTable->columns.end(); ++queryTable->columnItr) 
		{
            column* col = (column*)queryTable->columnItr->second;
			value = query->queryValue[count];
			memmove(&buff[col->position], value, col->length);
			count++;
    	}
	}
	else
	{
		for (column* col : queryColumn)
		{
			value = query->queryValue[count];
			memmove(&buff[col->position], value, col->length);
			count++;
		}
	}
	if(appendRecord(buff, tableStream, queryTable->recordLength) > 0)
	{
		returnResult.message.append(" 1 record inserted");
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
