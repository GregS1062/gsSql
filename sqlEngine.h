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
		ParseResult compareLike(Condition*);
		ParseResult compareEqual(Condition*);
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
		
	if(query->ignoringCaseIsEqual(_condition->Value,buffRecord))
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
		
	if(query->ignoringCaseIsEqual(_condition->Value,buffRecord))
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


		if(query->ignoringCaseIsEqual(condition->Operator,"like"))
		{
			queryResult = compareLike(condition);
		}
		else
		{
			if(query->ignoringCaseIsEqual(condition->Operator,"="))
			{
				queryResult = compareEqual(condition);
			}
			else
			{
				// Neither = nor like
				errText.append("condition neither like nor =");
			}
		}

		if(query->conditions.size() == 1)
			return queryResult;

		if(!query->ignoringCaseIsEqual(condition->Condition,"AND")
		&& !query->ignoringCaseIsEqual(condition->Condition,"OR")
		&& query->conditions.size() == 1
		&& queryResult == ParseResult::FAILURE)
			return ParseResult::FAILURE;

		if(query->ignoringCaseIsEqual(condition->Condition,"AND")
		&& queryResult == ParseResult::FAILURE)
			return ParseResult::FAILURE;
		
		if(query->ignoringCaseIsEqual(condition->Condition,"AND")
		&& queryResult  == ParseResult::SUCCESS
		&& queryResults == ParseResult::SUCCESS)
			return ParseResult::SUCCESS;

		if(query->ignoringCaseIsEqual(condition->Condition,"OR")
		&& (queryResult == ParseResult::SUCCESS
		|| queryResults == ParseResult::SUCCESS))
			return ParseResult::SUCCESS;

		queryResults = queryResult;		
	}

	return ParseResult::FAILURE;
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
