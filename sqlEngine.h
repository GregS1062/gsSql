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
	//assuming one column
	column* conditionColumn = nullptr;

	public:
    	sqlEngine(sqlParser*, ctable*);
		ParseResult open();
		ParseResult close();
		ParseResult selectQueryColumns();
		string 		fetchRow();
		char* 		getRecord(long, fstream*, int);
		bool		queryContitionsMet();
};

sqlEngine::sqlEngine(sqlParser* _query, ctable* _table)
{
    /***************************************
	 * Assuming a single table for now
	*******************************************/

		query 		= _query;
		queryTable  = _table;
}
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
ParseResult sqlEngine::close()
{
	tableStream->close();
	return ParseResult::SUCCESS;
}
bool sqlEngine::queryContitionsMet()
{
	if(query->conditions == nullptr)
		return true;
		
	if(conditionColumn == nullptr)
	{
		for(column* col : queryTable->columns)
		{
			if(query->ignoringCaseIsEqual(query->conditions->conColumn,col->name.c_str()))
			{
				conditionColumn = col;
				break;
			}
		}
	}
	if(conditionColumn == nullptr)
	{
		errText.append(" condition column ");
		errText.append(query->conditions->conColumn);
		errText.append(" condition operator ");
		errText.append(query->conditions->conOperator);
		errText.append(" condition value ");
		errText.append(query->conditions->conValue);
		errText.append(" not found ");
		query->rowsToReturn = 0;
		return false;
	}
	
	char buff[60];
	strncpy(buff, line+conditionColumn->position, conditionColumn->length);
	buff[conditionColumn->length] = '\0';
	if(strcmp(query->conditions->conValue,buff) == 0)
	{
		return true;
	}
	return false;
}
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
string sqlEngine::fetchRow()
{
	string rowResponse;
	string header;
	int recordPosition = 0;
	char buff[60];


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
		&& rowCount > query->rowsToReturn)
			break;

		if(queryContitionsMet())
		{
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
	

	return rowResponse;
}

/*-----------------------------------------------------------
	Get (read) record
-------------------------------------------------------------*/
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
