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
#include "resultList.h"


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
	resultList*		results;

	public:
		sqlEngine						(sqlParser*);
		ParseResult 	execute			(Statement*);
		ParseResult 	open			();
		ParseResult 	close			();
		ParseResult 	update			();
		ParseResult 	insert			();
		ParseResult 	Delete			();
		ParseResult		select			();
		char* 			getRecord		(long, fstream*, int);
		long			appendRecord	(void*, fstream*, int);
		bool 			writeRecord		(void*, long, fstream*, int);
		ParseResult 	formatInput		(char*, Column*);
		ParseResult 	updateIndexes	(long, SQLACTION);
		ParseResult 	tableScan		(list<Column*>, SQLACTION);
		searchIndexes* 	determineIndex	();
		string 			searchForward	(Search*, char*, int, SEARCH);
		string 			searchBack		(Search*, char*, int);
		string 			indexRead		(searchIndexes*);
		string  		htmlHeader		(list<Column*>,int32_t);
		string  		textHeader		(list<Column*>);
		bool			isRecordDeleted (bool);
		vector<TempColumn*>	outputLine	(list<Column*>);

};
/******************************************************
 * SQL Engine Constuctor
 ******************************************************/
sqlEngine::sqlEngine(sqlParser* _sp)
{
	sp = _sp;
}
/******************************************************
 * Execute
 ******************************************************/
ParseResult sqlEngine::execute(Statement* _statement)
{
	if(debug)
        fprintf(traceFile,"\n\n-------------------------BEGIN ENGINE-------------------------------------------");
	results = new resultList(_statement);
	statement = _statement;
	if(statement == nullptr)
	{
		utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Statement is null");
	}
	if(open() == ParseResult::FAILURE)
	{
		return ParseResult::FAILURE;
	}
	switch(statement->action)
	{
		case SQLACTION::CREATE:
		{
			utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false,"Not implemented at this time");
			return ParseResult::FAILURE;
			break;
		}
		case SQLACTION::DELETE:
		{
			Delete();
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
			select();
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
	
	if(statement->table->indexes.size() > 0)
	{
		for(sIndex* idx : statement->table->indexes)
		{
			//TODO test that file
			idx->close();
		}
	}
	return ParseResult::SUCCESS;
}
/******************************************************
 * Update Record
 ******************************************************/
ParseResult sqlEngine::update()
{

	if(tableScan(statement->table->columns,SQLACTION::UPDATE) == ParseResult::FAILURE)
	{
		utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false," Failed to update record");
		return ParseResult::FAILURE;
	};

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
 * Select
 ******************************************************/
ParseResult sqlEngine::select()
{
	fprintf(traceFile,"\n------------------------Select-----------------------------");
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

	returnResult.resultTable.append(header);

	//No conditions
	if(statement->table->conditions.size() == 0)
	{
		if(tableScan(columns, SQLACTION::SELECT) == ParseResult::FAILURE)
			return ParseResult::FAILURE;
		results->Sort();
		results->print();
		return ParseResult::SUCCESS;
	}

	//checks the index columns against the queryColumns
	searchIndexes* searchOn = determineIndex();

	//query condition but not on an indexed column
	if(searchOn == nullptr)
	{
		if(tableScan(columns, SQLACTION::SELECT) == ParseResult::FAILURE)
			return ParseResult::FAILURE;
		results->Sort();
		results->print();
		return ParseResult::SUCCESS;
	}
	
	returnResult.resultTable.append(indexRead(searchOn));
	
	return ParseResult::SUCCESS;

}
/******************************************************
 * Delete
 ******************************************************/
ParseResult sqlEngine::Delete()
{
	//checks the index columns against the queryColumns
	searchIndexes* searchOn = determineIndex();

	//query condition but not on an indexed column
	if(searchOn == nullptr)
	{
		tableScan(statement->table->columns, SQLACTION::DELETE);
		return ParseResult::SUCCESS;
	}
	
	indexRead(searchOn);

	return ParseResult::SUCCESS;
}
bool sqlEngine::isRecordDeleted(bool _ignoreDelete)
{
	//used for testing
	if(_ignoreDelete)
		return false;

	char buff[60];
	memcpy(&buff, line, 1);
	buff[1] = '\0';
	if(strcasecmp(buff,(char*)sqlTokenTrue) == 0)
		return true;
	
	return false;
}
/******************************************************
 * Table Scan
 ******************************************************/
ParseResult sqlEngine::tableScan(list<Column*> _columns, SQLACTION _action)
{

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

		if(isRecordDeleted(false))
		{
			recordPosition = recordPosition + statement->table->recordLength;
			continue;
		}


		//select top n
		if(statement->rowsToReturn > 0
		&& rowCount >= statement->rowsToReturn)
		{
			break;
		}

		if(compareToCondition::queryContitionsMet(statement->table->conditions, line) == ParseResult::SUCCESS)
		{

			resultCount++;
			
			//--------------------------------------------------------
			// Select Logic
			//--------------------------------------------------------
			if(_action == SQLACTION::SELECT)
			{
			
				results->addRow(outputLine(_columns));
				rowCount++;
			}
			
			//--------------------------------------------------------
			// Update and Delete Logic
			//--------------------------------------------------------
			if(_action == SQLACTION::UPDATE
			|| _action == SQLACTION::DELETE)
			{
				if(updateIndexes(recordPosition,_action)== ParseResult::FAILURE)
				{
					utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,true," Failed to update indexes");
					return ParseResult::FAILURE;
				}

				for (Column* col :statement->table->columns) 
				{
					if(formatInput(line,col) == ParseResult::FAILURE)
					{
						utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,true,"format input failure");
						return ParseResult::FAILURE;
					}
				}
				if(!writeRecord(line,recordPosition,tableStream,statement->table->recordLength))
				{
					utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,true," Failed to write record");
					return ParseResult::FAILURE;
				}
				
				rowCount++;
			}

		}

		recordPosition = recordPosition + statement->table->recordLength;
	}
	utilities::sendMessage(MESSAGETYPE::INFORMATION,presentationType,false,"Table scan: rows scanned ");
	utilities::sendMessage(MESSAGETYPE::INFORMATION,presentationType,false,std::to_string(resultCount).c_str());
	utilities::sendMessage(MESSAGETYPE::INFORMATION,presentationType,true,"rows returned ");
	utilities::sendMessage(MESSAGETYPE::INFORMATION,presentationType,false,std::to_string(rowCount).c_str());
	return ParseResult::SUCCESS;
}
/******************************************************
 * Output Line
 ******************************************************/
vector<TempColumn*>	sqlEngine::outputLine(list<Column*> _columns)
{

	vector<TempColumn*> row;
	for(Column* col : _columns )
	{
		TempColumn* temp = new TempColumn();
		temp->name		= col->name;
		temp->length	= col->length;
		temp->edit		= col->edit;
		switch(col->edit)
		{
			case t_edit::t_bool:
			{
				memcpy(&temp->boolValue,line+col->position,col->length);
				break;
			}
			case t_edit::t_char:
			{
				
				char buff[60];
				strncpy(buff, line+col->position, col->length);
				buff[col->length] = '\0';
				temp->charValue = utilities::dupString(buff);
				break;
			}
			case t_edit::t_int:
			{
				memcpy(&temp->intValue,line+col->position,col->length);
				break;
			}
			case t_edit::t_double:
			{
				memcpy(&temp->doubleValue,line+col->position,col->length);
				break;
			}
			case t_edit::t_date:
			{
				memcpy(&temp->dateValue,line+col->position,col->length);
				break;
			}
		}
		row.push_back(temp);
	}
	return row;
};
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

	if(statement->rowsToReturn > 0)
	{
		rowsToReturn = statement->rowsToReturn;
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

	utilities::upperCase(col->value);

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
	if(debug)
	{
		fprintf(traceFile,"\nOp=%d",(int)_op);
	}
	ResultList* searchResults = _search->findRange(_value, _rowsToReturn, _op);
	
	while(searchResults != nullptr)
	{
		line = getRecord(searchResults->location,tableStream, statement->table->recordLength);
		
		//End of File
		if(line == nullptr)
			break;

		//select top n
		if(rowCount >= _rowsToReturn)
			break;
		
		if(compareToCondition::queryContitionsMet(statement->table->conditions, line) == ParseResult::SUCCESS)
		{
			results->addRow(outputLine(statement->table->columns));
			rowCount++;

		}
		searchResults = searchResults->next;
	}
	if(statement->table->orderBy.size() > 0)
		results->Sort();
	results->print();
	utilities::sendMessage(MESSAGETYPE::INFORMATION,presentationType,true,"Rows returned ");
	utilities::sendMessage(MESSAGETYPE::INFORMATION,presentationType,false,std::to_string(rowCount).c_str());
	return "";
}
/******************************************************
 * Search Back
 ******************************************************/
string sqlEngine::searchBack(Search* _search, char* _value, int _rowsToReturn)
{

	int rowCount 		= 0;
	long location		= 0;
	char* key;
	Node* _leaf = _search->findLeafBase(_value);
	ScrollNode* scrollNode = _search->scrollIndexBackward(_leaf,_value);
	key = _value;

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
		if(rowCount >= _rowsToReturn)
			break;

		if(compareToCondition::queryContitionsMet(statement->table->conditions, line) == ParseResult::SUCCESS)
		{
			results->addRow(outputLine(statement->table->columns));
			rowCount++;
		}
	}
	if(statement->table->orderBy.size() > 0)
		results->Sort();
	results->print();
	utilities::sendMessage(MESSAGETYPE::INFORMATION,presentationType,true,"Rows returned ");
	utilities::sendMessage(MESSAGETYPE::INFORMATION,presentationType,false,std::to_string(rowCount).c_str());
	return "";
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
		if(updateIndexes(recordNumber, SQLACTION::INSERT) == ParseResult::FAILURE)
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
			//fprintf(traceFile,"\n %d %s %d",_col->position, _col->value, _col->length);
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
ParseResult sqlEngine::updateIndexes(long _location, SQLACTION _action)
{	
	Column* qColumn;
	//TODO joins are a problem here
	for(sIndex* idx : statement->table->indexes)
	{

		if(idx == nullptr)
			return ParseResult::SUCCESS;

		if(debug)
			fprintf(traceFile,"\n index %s",idx->name);

		if(idx->columns.size() == 0)
		{
			utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,true,"index has no columns");
			return ParseResult::FAILURE;
		}

		//TODO ONLY WORKS FOR SINGLE COLUMN INDEXES
		for(Column* iColumn : idx->columns)
		{

			qColumn = statement->table->getColumn(iColumn->name);

			if(qColumn == nullptr)
			{
				continue;
			}

			if(_action == SQLACTION::INSERT)
			{
				if(!idx->index->insertIndex->insert(qColumn->value,_location))
				{
					utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,true,"insert on ");
					utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false,iColumn->name);
					utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false," failed ");
					return ParseResult::FAILURE;
				};
			}
			if(_action == SQLACTION::UPDATE)
			{
				//If not primary delete and insert column - note: primary indexes cannot change.
				if(!iColumn->primary)
				{
					if(debug)
						fprintf(traceFile,"\ncolumn name %s position %d length %d",qColumn->name,qColumn->position,qColumn->length);
					
					char buffBeforValue[60];
					memcpy(&buffBeforValue,line+qColumn->position, qColumn->length);
					buffBeforValue[qColumn->length] = '\0';
					
					if(debug)
						fprintf(traceFile,"\nUpdate before value %s \nchanged value %s",buffBeforValue,qColumn->value);
					
					if(strcasecmp(buffBeforValue,qColumn->value) != 0)
					{
						if(!idx->index->deleteIndex->deleteEntry(buffBeforValue,_location))
						{
							utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Delete on ");
							utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false,iColumn->name);
							utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false," failed ");
							return ParseResult::FAILURE;
						};
						if(!idx->index->insertIndex->insert(qColumn->value,_location))
						{
							utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,true,"insert on ");
							utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false,iColumn->name);
							utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false," failed ");
							return ParseResult::FAILURE;
						};
					}
				};
			}
			if(_action == SQLACTION::DELETE)
			{
				if(!idx->index->deleteIndex->deleteEntry(qColumn->value,_location))
				{
					utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Delete on ");
					utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false,iColumn->name);
					utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false," failed ");
					return ParseResult::FAILURE;
				};
			}
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
		{
			if(debug)
				fprintf(traceFile,"\nwrite Record seek failure address %ld size %i line %s",_address,_size,(char*)_ptr);
			return false;
		}

		if (!_file->write((char*)_ptr, _size))
		{
			if(debug)
				fprintf(traceFile,"\nwrite failure address %ld size %i line %s",_address,_size,(char*)_ptr);
			return false;
		}

		_file->flush();
		return true;
	}
	catch(const std::exception& e)
    {
        utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false, e.what());
        return 0;
    } 
	
}
