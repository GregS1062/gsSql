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
		ParseResult ValidateQueryValues();
		ParseResult	getConditionColumns();
		ParseResult queryContitionsMet();
		ParseResult storeData();
		string 		fetchData();
		char*		getValue(char*);
		char* 		getRecord(long, fstream*, int);
		long		appendRecord(void*, fstream*, int);


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
	bool syntaxError = false;
	bool match = false;
	for(char* token : query->queryColumn)
	{
		match = false; 
        for(column* qColumn : queryTable->columns)
        {
			if(query->ignoringCaseIsEqual(token,sqlTokenAsterisk))
			{
				match = true;
				queryColumn.push_back(qColumn);
			}
			if (query->ignoringCaseIsEqual(token,qColumn->name.c_str()))
			{
				match = true;
				queryColumn.push_back(qColumn);
				break;
			}
        }
		if(!match)
		{
			errText.append(" ");
			errText.append(token);
			errText.append(" not found ");
			syntaxError = true;
		}
	}
	if(syntaxError)
		return ParseResult::FAILURE;

	return ParseResult::SUCCESS;
}
/******************************************************
 * Get Condition Columns
 ******************************************************/
ParseResult sqlEngine::getConditionColumns()
{

	for(Condition* condition : query->conditions)
	{
		if(condition == nullptr)
			return ParseResult::FAILURE;
		
		for(column* col : queryTable->columns)
		{
			if(query->ignoringCaseIsEqual(condition->ColumnName,col->name.c_str()))
			{
				condition->Column = col;
				break;
			}
		}
		if(condition->Column == nullptr)
		{
			errText.append(" condition column ");
			errText.append(condition->ColumnName);
			errText.append(" not found ");
			query->rowsToReturn = 1;
			return ParseResult::FAILURE;
		}
	}
	return ParseResult::SUCCESS;
}
/******************************************************
 * Query Conditions Met
 ******************************************************/
ParseResult sqlEngine::queryContitionsMet()
{

	char buffRecord[60];
	column* column;
	for(Condition* condition : query->conditions)
	{
		column = condition->Column;

		if(strcmp(condition->Operator,"=") == 0)
		{
			strncpy(buffRecord, line+column->position, column->length);
			buffRecord[column->length] = '\0';

			if (!query->ignoringCaseIsEqual(condition->Value,buffRecord))
				return ParseResult::FAILURE;
			
			continue;
		}

		strncpy(buffRecord, line+column->position, strlen(condition->Value));
		buffRecord[strlen(condition->Value)] = '\0';
		
		if(query->ignoringCaseIsEqual(condition->Operator,"like")
		&& (!query->ignoringCaseIsEqual(condition->Value,buffRecord)))
		{
			return ParseResult::FAILURE;
		}
	}
	return ParseResult::SUCCESS;
}
/******************************************************
 * Select Query Columns
 ******************************************************/
ParseResult sqlEngine::ValidateQueryValues()
{
	bool syntaxError = false;
	bool valid = false;

	if(queryTable->columns.size() != query->queryValue.size())
	{
		errText.append(" Count of table columns not equal count of values ");
		return ParseResult::FAILURE;
	}

	for(char* token : query->queryValue)
	{
		valid = false; 
        for(column* qColumn : queryTable->columns)
        {
			if(query->ignoringCaseIsEqual(token,sqlTokenAsterisk))
			{
				valid = true;
				queryColumn.push_back(qColumn);
			}
			if (query->ignoringCaseIsEqual(token,qColumn->name.c_str()))
			{
				valid = true;
				queryColumn.push_back(qColumn);
				break;
			}
        }
		if(!valid)
		{
			errText.append(" ");
			errText.append(token);
			errText.append(" not found ");
			syntaxError = true;
		}
	}
	if(syntaxError)
		return ParseResult::FAILURE;

	return ParseResult::SUCCESS;
}
/******************************************************
 * Fetch Row
 ******************************************************/
string sqlEngine::fetchData()
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

	//list<int>columnSize;
	for (column* col : queryColumn)
	{
		sumOfColumnSize = sumOfColumnSize + col->length;
	}
	
	double percentage = 0;

	header.append(rowBegin);
	for (column* col : queryColumn)
	{
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
			for (column* col : queryColumn)
			{
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
ParseResult sqlEngine::storeData()
{
	char* buff = (char*)malloc(queryTable->recordLength);
	size_t count = 0;
	char* value;
	for (column* col : queryColumn)
	{
		value = query->queryValue[count];
		memmove(&buff[col->position], value, col->length);
		count++;
	}
	appendRecord(buff, tableStream, queryTable->recordLength);
	return ParseResult::SUCCESS;
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
