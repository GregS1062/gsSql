#pragma once

#include <system_error>
#include <iostream>
#include <fstream>
#include <cstdint>
#include <ctime>
#include <cstring>
#include <string.h>
#include <string>
#include <cstdio>
#include <list>
#include <vector>
#include <algorithm>
#include <filesystem>
#include "sqlCommon.h"
#include "queryParser.h"
#include "conditions.h"
#include "utilities.h"
#include "insert.h"


using namespace std;

/******************************************************
 * SQL Engine
 ******************************************************/
class sqlEngine
{
	/***************************************
	 * Assuming a single table for now
	*******************************************/
	cTable* 		queryTable;
	cIndex* cindex;
	fstream* 		tableStream;
	fstream* 		indexStream;
	list<column*>	queryColumn;
	char* line;

	public:
		queryParser* 	query;
		ParseResult 	prepare(queryParser*, cTable*);
		ParseResult 	open();
		ParseResult 	close();
		ParseResult		getConditionColumns();
		ParseResult 	update();
		ParseResult 	insert();
		string 			select();
		char* 			getRecord(long, fstream*, int);
		long			appendRecord(void*, fstream*, int);
		bool 			writeRecord(void*, long, fstream*, int);
		string 			formatOutput(column*);
		ParseResult 	formatInput(char*, column*);
		ParseResult 	updateIndexes(long);

};
/******************************************************
 * SQL Engine Constuctor
 ******************************************************/
ParseResult sqlEngine::prepare(queryParser* _query, cTable* _table)
{
    /***************************************
	 * Assuming a single table for now
	*******************************************/

		query 		= _query;
		queryTable  = _table;
		return ParseResult::FAILURE;
}
/******************************************************
 * Open
 ******************************************************/
ParseResult sqlEngine::open()
{
		////Open data file
        tableStream = new fstream{};
		tableStream->open(queryTable->fileName, ios::in | ios::out | ios::binary);
		if (!tableStream->is_open()) {
            errText.append(queryTable->fileName);
            errText.append(" not opened ");
			return ParseResult::FAILURE;
		}

		indexStream = new fstream{};
		cindex = queryTable->indexes.front();
		cindex->open();
		cindex->index = new Index(cindex->fileStream);

		return ParseResult::SUCCESS;
}
/******************************************************
 * Close
 ******************************************************/
ParseResult sqlEngine::close()
{
	tableStream->close();
	cindex->close();
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
		
		if(compareToCondition::queryContitionsMet(query->conditions, line) == ParseResult::SUCCESS)
		{
			for (itr = columns.begin(); itr != columns.end(); ++itr) 
			{
				col = (column*)itr->second;
				if(formatInput(line,col) == ParseResult::FAILURE)
					return ParseResult::FAILURE;
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
			if(col->edit == t_edit::t_date)
			{
				sumOfColumnSize = sumOfColumnSize + 16;
			}
			else
			{
				sumOfColumnSize = sumOfColumnSize + col->length;
			}
    }
	
	double percentage = 0;
	header.append(rowBegin);
	for (itr = columns.begin(); itr != columns.end(); ++itr) 
	{
		col = (column*)itr->second;
		header.append("\n\t");
		header.append(hdrBegin);
		header.append(" style="" width:");
		if(col->edit == t_edit::t_date)
		{
			percentage = 12 / sumOfColumnSize * 100;
		}
		else
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

		if(compareToCondition::queryContitionsMet(query->conditions,line) == ParseResult::SUCCESS)
		{
			resultCount++;
			rowResponse.append("\n\t\t");
			rowResponse.append(rowBegin);
			for (itr = columns.begin(); itr != columns.end(); ++itr) 
			{
				col = (column*)itr->second;
				rowResponse.append(cellBegin);
				rowResponse.append(formatOutput(col));
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
 * Format Ouput
 ******************************************************/
string sqlEngine::formatOutput(column* _col)
{
	char buff[60];
	std::stringstream ss;
	string formatString;


	switch(_col->edit)
	{
		case t_edit::t_char:
		{
			memcpy(&buff, line+_col->position, _col->length);
			buff[_col->length] = '\0';
			return formatString.append(buff);
			break;
		}
		case t_edit::t_bool:
		{
			memcpy(&buff, line+_col->position, _col->length);
			//return formatString.append("error");
			break;
		}
		case t_edit::t_int:
		{
			int icopy;
			memcpy(&icopy, line+_col->position, _col->length);
			return formatString.append(std::to_string(icopy));
			break;
		}
		case t_edit::t_double:
		{
			double dbl;
			memcpy(&dbl, line+_col->position, _col->length);
			if(dbl < 0){
				ss << "-$" << std::fixed << std::setprecision(2) << -dbl; 
				} else {
				ss << "$" << std::fixed << std::setprecision(2) << dbl; 
			}
			return ss.str();
			break;
		}
		case t_edit::t_date:
		{
			t_tm dt;
			memcpy(&dt, line+_col->position, sizeof(t_tm));
			formatString.append(std::to_string(dt.month));
			formatString.append("/");
			formatString.append(std::to_string(dt.day));
			formatString.append("/");
			formatString.append(std::to_string(dt.year));
			return formatString;
		}
		
	}
	return "error";
}
/******************************************************
 * Store Record
 ******************************************************/
ParseResult sqlEngine::insert()
{
	size_t count = 0;
	char* buff = (char*)malloc(queryTable->recordLength);
	column* col;
	map<char*,column*>::iterator itr;
	map<char*,column*>columns = query->queryTable->columns;
	for (itr = columns.begin(); itr != columns.end(); ++itr) {
        col = (column*)itr->second;
		formatInput(buff, col);
		count++;
	}
	
	long recordNumber = appendRecord(buff, tableStream, queryTable->recordLength);
	free(buff);
	if(recordNumber > NEGATIVE)
	{	
		if(updateIndexes(recordNumber) == ParseResult::FAILURE)
		{
			returnResult.message.append("update index failed");
			return ParseResult::FAILURE;
		};
		returnResult.message.append(" 1 record inserted at location ");
		returnResult.message.append(std::to_string(recordNumber));
		return ParseResult::SUCCESS;
	};
	errText.append(" location 0 - insert failed");
	return ParseResult::FAILURE;
}
/******************************************************
 * Format Input
 ******************************************************/
ParseResult sqlEngine::formatInput(char* _buff, column* _col)
{
	if(((size_t)_col->position + strlen(_col->value)) > (size_t)queryTable->recordLength)
	{
		errText.append("buffer overflow on ");
		errText.append(_col->name.c_str());
		return ParseResult::FAILURE;
	}
	switch(_col->edit)
	{
		case t_edit::t_bool:
		{
			memmove(&_buff[_col->position], _col->value, _col->length);
			break;
		}
		case t_edit::t_char:
		{
			//printf("\n %d %s %d",_col->position, _col->value, _col->length);
			memmove(&_buff[_col->position], _col->value, _col->length);
			break;
		}
		case t_edit::t_int:
		{
			int _i = atoi(_col->value);
			memmove(&_buff[_col->position], &_i, _col->length);
			break;
		}
		case t_edit::t_double:
		{
			double _d = atof(_col->value);
			memmove(&_buff[_col->position], &_d, _col->length);
			break;
		}
		case t_edit::t_date:
		{
			t_tm _d = utilities::parseDate(_col->value);
			memmove(&_buff[_col->position], &_d, _col->length);
			break;
		}
	}
	return ParseResult::SUCCESS;
}
/******************************************************
 * Update Indexes
 ******************************************************/
ParseResult sqlEngine::updateIndexes(long _location)
{	
	column* iColumn;
	column* qColumn;
	for(size_t i = 0;i < cindex->columns.size();i++)
	{
		iColumn = query->scrollColumns(cindex->columns,(short)i);
		qColumn = query->getQueryTableColumnByName((char*)iColumn->name.c_str());
		if(!cindex->index->insertIndex->insert(qColumn->value,_location))
		{
			errText.append(" insert on ");
			errText.append(iColumn->name);
			errText.append(" failed ");
			return ParseResult::FAILURE;
		};
	}
	
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
