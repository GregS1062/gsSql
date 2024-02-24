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
	ctable* 		queryTable;
	fstream* 		tableStream;
	sqlParser* 		query;
	list<column*>	queryColumn;
	char* line;

	public:
    	sqlEngine(sqlParser*, ctable*);
		ParseResult open();
		ParseResult close();
		ParseResult selectQueryColumns();
		ParseResult	getConditionColumns();
		ParseResult queryContitionsMet();
		string 		fetchData();
		char* 		getRecord(long, fstream*, int);
};

sqlEngine::sqlEngine(sqlParser* _query, ctable* _table)
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
		strncpy(buffRecord, line+column->position, column->length);
		buffRecord[column->length] = '\0';

		if(strcmp(condition->Operator,"=") == 0
		&& (!query->ignoringCaseIsEqual(condition->Value,buffRecord)))
		{
			return ParseResult::FAILURE;
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
ParseResult sqlEngine::selectQueryColumns()
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
