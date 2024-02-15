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
	ctable* 			queryTable;
	fstream* 		tableStream;
	sqlParser* 		query;
	list<column*>	queryColumn;

	public:
    	sqlEngine(sqlParser*, ctable*);
		ParseResult open();
		ParseResult selectQueryColumns();
		string 		fetchRow();
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
ParseResult sqlEngine::selectQueryColumns()
{
	tokens* col = query->getNextColumnToken(nullptr);
	while(col != nullptr)
	{
		column* qColumn          = queryTable->columnHead;
        while(qColumn != nullptr)
        {
            if(strcmp(col->token,qColumn->name.c_str()) == 0)
			{
				queryColumn.push_back(qColumn);
			}
            qColumn = qColumn->next;
        }
		col = col->next;
	}
	return ParseResult::SUCCESS;
}
string sqlEngine::fetchRow()
{
	string rowResponse;
	int recordPosition = 0;
	char* line;
	char buff[60];

	rowResponse.append(rowBegin);
	for (column* col : queryColumn)
	{
		rowResponse.append("\n");
		rowResponse.append(hdrBegin);
		rowResponse.append(col->name);
		rowResponse.append(hdrEnd);
	}
	rowResponse.append(rowEnd);

	for(int i=0;i<15;i++)
	{
		line = getRecord(recordPosition,tableStream, queryTable->recordLength);
		rowResponse.append(rowBegin);
		for (column* col : queryColumn)
		{
			rowResponse.append("\n");
			rowResponse.append(cellBegin);
			strncpy(buff, line+col->position, col->length);
			buff[col->length] = '\0';
			rowResponse.append(buff);
			rowResponse.append(cellEnd);
		}
		rowResponse.append(rowEnd);
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