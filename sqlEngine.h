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
#include "binding.h"
#include "conditions.h"
#include "utilities.h"
#include "insert.h"
#include "search.h"
#include "lookup.h"
#include "defines.h"


using namespace std;

class searchIndexes
{
	public:
		sIndex* index;
		char*	op;
		list<Column*> col;
};

/******************************************************
 * SQL Engine
 ******************************************************/
class sqlEngine
{
	/***************************************
	 * Assuming a single table for now
	*******************************************/
	fstream* 		tableStream;
	fstream* 		indexStream;
	list<sIndex*>	sIndexes;
	binding* 		bind;
	char* 			line;
	sqlParser* 		sp;
	Statement*		statement;

	public:
		sqlEngine(sqlParser*);
		ParseResult 	execute(Statement*);
		ParseResult 	open();
		ParseResult 	close();
		ParseResult		getConditionColumns();
		ParseResult 	update();
		ParseResult 	insert();
		string 			select();
		char* 			getRecord(		long, fstream*, int);
		long			appendRecord(	void*, fstream*, int);
		bool 			writeRecord(	void*, long, fstream*, int);
		string			outputLine(		list<Column*>);
		string 			formatOutput(	Column*);
		ParseResult 	formatInput(	char*, Column*);
		ParseResult 	updateIndexes(	long);
		string 			tableScan(		list<Column*>);
		searchIndexes* 	determineIndex();
		string 			searchForward(	Search*, char*, int, SEARCH);
		string 			searchBack(		Search*, char*, int);
		string 			indexRead(		searchIndexes*);
		string  		htmlHeader(		list<Column*>,int32_t);
		string  		textHeader(		list<Column*>);

};
/******************************************************
 * SQL Engine Constuctor
 ******************************************************/
sqlEngine::sqlEngine(sqlParser* _sp)
{
	sp = _sp;
}
ParseResult sqlEngine::execute(Statement* _statement)
{
		statement = _statement;
		if(statement == nullptr)
		{
			utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Statement is null");
		}
		if(open() == ParseResult::FAILURE)
		{
			return ParseResult::FAILURE;
		}
		switch(statement->plan->action)
		{
			case SQLACTION::CREATE:
			{
				utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false,"Not implemented at this time");
				return ParseResult::FAILURE;
				break;
			}
			case SQLACTION::DELETE:
			{
				utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false,"Not implemented at this time");
				return ParseResult::FAILURE;
				break;
			}
			case SQLACTION::NOACTION:
			{
				utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false,"Invalid SQL Action");
				return ParseResult::FAILURE;
				break;
			}
			case SQLACTION::SELECT:
			{
				returnResult.resultTable = select();
				break;
			}
			case SQLACTION::INSERT:
			{
				insert();
				break;
			}
			case SQLACTION::INVALID:
			{
				utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false,"Invalid SQL Action");
				return ParseResult::FAILURE;
				break;
			}
			case SQLACTION::UPDATE:
			{
				update();
				break;
			}
		}
		return ParseResult::SUCCESS;
}
/******************************************************
 * Open
 ******************************************************/
ParseResult sqlEngine::open()
{
		////Open data file
        tableStream = new fstream{};
		tableStream->open(statement->table->fileName, ios::in | ios::out | ios::binary);
		if (!tableStream->is_open()) {
            utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,true,statement->table->fileName);
            utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false," not opened ");
			utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false,strerror(errno));
			return ParseResult::FAILURE;
		}

		if(statement->table->indexes.size() > 0)
		{
			for(sIndex* idx : statement->table->indexes)
			{
				//TODO test that file
				if(idx->open() == nullptr)
				{
					utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Failed to open index: ");
					utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false,idx->fileName);
					return ParseResult::FAILURE;
				}
				idx->index = new Index(idx->fileStream);
			}
		}
		return ParseResult::SUCCESS;
}
/******************************************************
 * Close
 ******************************************************/
ParseResult sqlEngine::close()
{
	if(tableStream != nullptr)
		tableStream->close();
	return ParseResult::SUCCESS;
}
/******************************************************
 * Get Condition Columns
 ******************************************************/
ParseResult sqlEngine::getConditionColumns()
{
	//Get the SQL definition of the full table because the condition column may
	//	not be in the list of columns to be returned
	sTable* sqlTable = lookup::getTableByName(sp->tables,statement->table->name);
	
	Column* col;
	for(Condition* condition : statement->table->conditions)
	{
		if(condition == nullptr)
			return ParseResult::FAILURE;
		

		col = sqlTable->getColumn(condition->name);

		if(col == nullptr)
		{
			utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false," condition column ");
			utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false,condition->name);
			utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false," not found ");
			statement->plan->rowsToReturn = 1;
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

	if(getConditionColumns() == ParseResult::FAILURE)
	{
		utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,true, " Query condition failure");
		return ParseResult::FAILURE;
	};

	//table scan for now
	while(true)
	{
		line = getRecord(recordPosition,tableStream, statement->table->recordLength);
		if(line == nullptr)
			break;
		
		if(compareToCondition::queryContitionsMet(statement->table->conditions, line) == ParseResult::SUCCESS)
		{
			for (Column* col :statement->table->columns) 
			{
				if(formatInput(line,col) == ParseResult::FAILURE)
					return ParseResult::FAILURE;
			}
			if(!writeRecord(line,recordPosition,tableStream,statement->table->recordLength))
			{
				utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false," Failed to update record");
				return ParseResult::FAILURE;
			}
			rowCount++;
		}
		recordPosition = recordPosition + statement->table->recordLength;
	}
	returnResult.message.append(std::to_string(rowCount));
	returnResult.message.append(" rows updated");
	return ParseResult::SUCCESS;
}
/******************************************************
 * HTML Header
 ******************************************************/
string sqlEngine::htmlHeader(list<Column*> _columns, int32_t _sumOfColumnSize)
{
	double percentage = 0;
	string header;
	header.append(rowBegin);
	for (Column* col : _columns) 
	{
		header.append("\n\t");
		header.append(hdrBegin);
		header.append(" style="" width:");
		if(col->edit == t_edit::t_date)
		{
			percentage = 12 / _sumOfColumnSize * 100;
		}
		else
			percentage = (double)col->length / _sumOfColumnSize * 100;
		header.append(to_string((int)percentage));
		header.append("%"">");
		header.append(col->name);
		header.append(hdrEnd);
	}
	header.append(rowEnd);
	return header;
}
/******************************************************
 * Text Header
 ******************************************************/
string sqlEngine::textHeader(list<Column*> _columns)
{
	size_t pad = 0;
	string header;
	header.append("\n");
	for (Column* col : _columns) 
	{
		if((size_t)col->length > strlen(col->name))
		{
			header.append(col->name);
			pad = col->length - strlen(col->name);
			for(size_t i =0;i<pad;i++)
			{
				header.append(" ");
			}
		}
		else
		{
			char* name = col->name;
			name[col->length] = '\0';
			header.append(name);
			header.append(" ");
		}
	}

	return header;
}
/******************************************************
 * Fetch Row
 ******************************************************/
string sqlEngine::select()
{
	string rowResponse;
	string header;

	int sumOfColumnSize = 0;

	list<Column*> columns = statement->table->columns;
	for( Column* col: columns)
	{
		if(col->edit == t_edit::t_date)
		{
			sumOfColumnSize = sumOfColumnSize + 16;
		}
		else
		{
			sumOfColumnSize = sumOfColumnSize + col->length;
		}
    }
	if(presentationType == PRESENTATION::HTML)
		header = htmlHeader(columns,sumOfColumnSize);
	else
		header = textHeader(columns);

	rowResponse.append(header);

	//No conditions
	if(statement->table->conditions.size() == 0)
	{
		rowResponse.append(tableScan(columns));
		return rowResponse;
	}

	//checks the index columns against the queryColumns
	searchIndexes* searchOn = determineIndex();

	//query condition but not on an indexed column
	if(searchOn == nullptr)
	{
		rowResponse.append(tableScan(columns));
		return rowResponse;
	}
	
	rowResponse.append(indexRead(searchOn));
	return rowResponse;

}
/******************************************************
 * Table Scan
 ******************************************************/
string sqlEngine::tableScan(list<Column*> _columns)
{
	string rowResponse;
	int rowCount = 0;
	int recordPosition 	= 0;
	int resultCount 	= 0; 

	string lineResult;

	while(true)
	{
		line = getRecord(recordPosition,tableStream, statement->table->recordLength);
		
		//End of File
		if(line == nullptr)
		{
			break;
		}


		//select top n
		if(statement->plan->rowsToReturn > 0
		&& rowCount >= statement->plan->rowsToReturn)
		{
			break;
		}

		resultCount++;

		lineResult = outputLine(_columns);
		if(lineResult.length() > 0)
		{
			rowResponse.append(lineResult);
			rowCount++;
		}
		recordPosition = recordPosition + statement->table->recordLength;
	}
	utilities::sendMessage(MESSAGETYPE::INFORMATION,presentationType,false,"Table scan: rows scanned ");
	utilities::sendMessage(MESSAGETYPE::INFORMATION,presentationType,false,std::to_string(resultCount).c_str());
	utilities::sendMessage(MESSAGETYPE::INFORMATION,presentationType,true,"rows returned ");
	utilities::sendMessage(MESSAGETYPE::INFORMATION,presentationType,false,std::to_string(rowCount).c_str());
	return rowResponse;
}
/******************************************************
 * Index read
 ******************************************************/
searchIndexes* sqlEngine::determineIndex()
{
	searchIndexes* searchOn = nullptr;
	for(sIndex* idx : statement->table->indexes)
	{
		for(Column* col : idx->columns)
		{
			for(Condition* condition : statement->table->conditions)
			{
				if(strcasecmp(col->name,condition->col->name) == 0 )
				{
					//TODO  only affects char* indexes
				   if(searchOn == nullptr)
						searchOn = new searchIndexes();
					searchOn->index = idx;
					searchOn->op	= condition->op;
					condition->col->value = condition->value;
					searchOn->col.push_back(condition->col);
					return searchOn;
				}
			}
		}
	}

	return nullptr;
}
/******************************************************
 * Index read
 ******************************************************/
string sqlEngine::indexRead(searchIndexes* _searchOn)
{

	string lineResult;
	SEARCH op;
	int rowsToReturn 	= 0;

	if(statement->plan->rowsToReturn > 0)
	{
		rowsToReturn = statement->plan->rowsToReturn;
	}
	else	
	{
		rowsToReturn = MAXRESULTS;
	}

	Search* search = new Search(_searchOn->index->fileStream);

	utilities::sendMessage(MESSAGETYPE::INFORMATION,presentationType,false,"indexed read on ");
	utilities::sendMessage(MESSAGETYPE::INFORMATION,presentationType,false,_searchOn->index->name);

	//TODO using one column
	Column* col = _searchOn->col.front();

	if(col == nullptr)
	{
		utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Seach on column is null ");
		return "";
	}

	if(strcasecmp(_searchOn->op,(char*)sqlTokenEqual) == 0)
	{
		op = SEARCH::EXACT;
	}
	else
	if(strcasecmp(_searchOn->op,(char*)sqlTokenGreater) == 0
	|| strcasecmp(_searchOn->op,(char*)sqlTokenGreaterOrEqual) == 0)
	{
		op = SEARCH::FORWARD;
	}
	else
	if(strcasecmp(_searchOn->op,(char*)sqlTokenLessThan) == 0
	|| strcasecmp(_searchOn->op,(char*)sqlTokenLessOrEqual) == 0)
	{
		op = SEARCH::BACK;
		return searchBack(search,col->value,rowsToReturn);
	}
	else
	if(strcasecmp(_searchOn->op,(char*)sqlTokenLike) == 0)
	{
		op = SEARCH::LIKE;
	}
	
	return searchForward(search,col->value,rowsToReturn, op);
}
/******************************************************
 * Search Forward
 ******************************************************/
string sqlEngine::searchForward(Search* _search, char* _value, int _rowsToReturn, SEARCH _op)
{
	string rowResponse;
	string lineResult;
	int rowCount 		= 0;
	ResultList* results = _search->findRange(_value, _rowsToReturn, _op);
	
	while(results != nullptr)
	{
		line = getRecord(results->location,tableStream, statement->table->recordLength);
		
		//End of File
		if(line == nullptr)
			break;

		//select top n
		if(rowCount >= _rowsToReturn)
			break;

		lineResult = outputLine(statement->table->columns);
		if(lineResult.length() > 0)
		{
			rowResponse.append(lineResult);
			rowCount++;
		}
		results = results->next;
	}
	utilities::sendMessage(MESSAGETYPE::INFORMATION,presentationType,true,"Rows returned ");
	utilities::sendMessage(MESSAGETYPE::INFORMATION,presentationType,false,std::to_string(rowCount).c_str());
	return rowResponse;
}
/******************************************************
 * Search Back
 ******************************************************/
string sqlEngine::searchBack(Search* _search, char* _value, int _rowsToReturn)
{
	string rowResponse;
	string lineResult;
	int rowCount 		= 0;
	int location		= 0;
	char* key;
	Node* _leaf = _search->findLeafBase(_value);
	ScrollNode* scrollNode = _search->scrollIndexBackward(_leaf,_value);
	key = _value;
	
	while(true)
	{
		scrollNode = _search->scrollIndexBackward(scrollNode->leaf,key);
		location = _search->getLocation(scrollNode->leaf,scrollNode->position);
		_search->getKeyFromPosition(scrollNode->leaf, key, scrollNode->position);
		
		line = getRecord(location,tableStream, statement->table->recordLength);
		
		//End of File
		if(line == nullptr)
			break;

		//select top n
		if(rowCount >= _rowsToReturn)
			break;

		lineResult = outputLine(statement->table->columns);
		if(lineResult.length() > 0)
		{
			rowResponse.append(lineResult);
			rowCount++;
		}
	}
	utilities::sendMessage(MESSAGETYPE::INFORMATION,presentationType,true,"Rows returned ");
	utilities::sendMessage(MESSAGETYPE::INFORMATION,presentationType,false,std::to_string(rowCount).c_str());
	return rowResponse;
}
/******************************************************
 * Format Ouput
 ******************************************************/
string sqlEngine::outputLine(list<Column*> columns)
{
	size_t pad = 0;
	string result;
	if(compareToCondition::queryContitionsMet(statement->table->conditions,line) == ParseResult::SUCCESS)
	{
		result.append("\n");
		if(presentationType == PRESENTATION::HTML)
		{
			//newline and tabs aid the reading of html source
			result.append("\t\t");
			result.append(rowBegin);
		}
		for (Column* col : columns) 
		{
			if(presentationType == PRESENTATION::HTML)
			{
				result.append(cellBegin);
				result.append(formatOutput(col));
				result.append(cellEnd);
			}
			else
			{
				string out = formatOutput(col);
				result.append(out);
				if((size_t)col->length > out.length())
				{
					pad = col->length - out.length();
					for(size_t i =0;i<pad;i++)
					{
						result.append(" ");
					}
				}
			}
		}
		if(presentationType == PRESENTATION::HTML)
			result.append(rowEnd);
	}
	return result;
}
/******************************************************
 * Format Ouput
 ******************************************************/
string sqlEngine::formatOutput(Column* _col)
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
			char boole;
			memcpy(&boole, line+_col->position, 1);
			int b = (int)boole;
			if(b == 1)
				formatString.append("T ");
			else
				formatString.append("F ");
			return formatString;
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
	return "e";
}
/******************************************************
 * Store Record
 ******************************************************/
ParseResult sqlEngine::insert()
{
	size_t count = 0;
	char* buff = (char*)malloc(statement->table->recordLength);
	for(Column* col : statement->table->columns)
	{
		if(formatInput(buff, col) == ParseResult::FAILURE)
			return ParseResult::FAILURE;
		count++;
	}
	
	long recordNumber = appendRecord(buff, tableStream, statement->table->recordLength);
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
	utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false," location 0 - insert failed");
	return ParseResult::FAILURE;
}
/******************************************************
 * Format Input
 ******************************************************/
ParseResult sqlEngine::formatInput(char* _buff, Column* _col)
{
	if(_col->value == nullptr)
	{
		utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false,_col->name);
		utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false," is null");
		return ParseResult::FAILURE;
	}
	if(((size_t)_col->position + strlen(_col->value)) > (size_t)statement->table->recordLength)
	{
		utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false,"buffer overflow on ");
		utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false,_col->name);
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
	Column* qColumn;

	//TODO joins are a problem here
	for(sIndex* idx : statement->table->indexes)
	{
		if(idx == nullptr)
			return ParseResult::SUCCESS;

		if(idx->columns.size() == 0)
			return ParseResult::FAILURE;

		for(Column* iColumn : idx->columns)
		{
			
			qColumn = statement->table->getColumn(iColumn->name);

			if(qColumn == nullptr)
				return ParseResult::FAILURE;

			if(!idx->index->insertIndex->insert(qColumn->value,_location))
			{
				utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false," insert on ");
				utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false,iColumn->name);
				utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false," failed ");
				return ParseResult::FAILURE;
			};
		}
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
        utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false, e.what());
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
			utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false,"Write to file failed");
			eof = 0;
		}
			

		_file->flush();

		return eof;
	}
	catch(const std::exception& e)
    {
        utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false, e.what());
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
        utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false, e.what());
        return 0;
    } 
	
}
