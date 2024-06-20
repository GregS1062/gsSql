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
#include <insert.h>
#include <search.h>
#include "defines.h"
#include "sqlCommon.h"
#include "compare.cpp"
#include "tempFiles.cpp"


using namespace std;

struct searchIndexes
{
		shared_ptr<sIndex> 			index;
		string						op;
		list<shared_ptr<Column>> 	col;
};


/******************************************************
 * SQL Engine
 ******************************************************/
class sqlEngine
{
	/***************************************
	 * Common engine to access a single table 
	*******************************************/
	protected:
		fstream* 		tableStream;
		fstream* 		indexStream;
		char* 			line;
		shared_ptr<Statement>	statement;
		Search*			search;

		ParseResult 	open			();
		ParseResult 	close			();
		SEARCH 			indexRead		(shared_ptr<searchIndexes>);
		char* 			getRecord		(long, fstream*, int);
		shared_ptr<searchIndexes> 	determineIndex	();
		bool			isRecordDeleted (bool);
		vector<shared_ptr<TempColumn>>	outputLine	(list<shared_ptr<Column>>);
		void			rowsUpdatedMsg 	(int);
		void 			rowsReturnedMsg ();

	public:
		shared_ptr<tempFiles>		results			= make_shared<tempFiles>();

};

/******************************************************
 * Open
 ******************************************************/
ParseResult sqlEngine::open()
{
		////Open data file
        tableStream = new fstream{};
		tableStream->open(statement->table->fileName, ios::in | ios::out | ios::binary);
		if (!tableStream->is_open()) {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,statement->table->fileName);
            sendMessage(MESSAGETYPE::ERROR,presentationType,false," not opened ");
			sendMessage(MESSAGETYPE::ERROR,presentationType,false,strerror(errno));
			return ParseResult::FAILURE;
		}

		if(statement->table->indexes.size() > 0)
		{
			for(shared_ptr<sIndex> idx : statement->table->indexes)
			{
				if(idx->open() == nullptr)
				{
					sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Failed to open index: ");
					sendMessage(MESSAGETYPE::ERROR,presentationType,false,idx->fileName);
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
		for(shared_ptr<sIndex> idx : statement->table->indexes)
		{
			//TODO test that file
			idx->close();
		}
	}
	return ParseResult::SUCCESS;
}

/******************************************************
 * Is Record Deleted
 ******************************************************/
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
 * Output Line
 ******************************************************/
vector<shared_ptr<TempColumn>>	sqlEngine::outputLine(list<shared_ptr<Column>> _columns)
{

	vector<shared_ptr<TempColumn>> row;
	for(shared_ptr<Column> col : _columns )
	{
		shared_ptr<TempColumn> temp = make_shared<TempColumn>();
		temp->name		= col->name;
		temp->length	= col->length;
		temp->edit		= col->edit;
		temp->alias		= col->alias;
		temp->functionType		= col->functionType;
		if(col->functionType == t_function::COUNT
		&& strcasecmp(col->name.c_str(),sqlTokenCount) == 0)
		{
			temp->intValue = 1;
			row.push_back(temp);
			continue;
		}
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
				temp->charValue = dupString(buff);
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
			default:
				break;
		}
		row.push_back(temp);
	}
	return row;
};
/******************************************************
 * Determine Index
 ******************************************************/
shared_ptr<searchIndexes> sqlEngine::determineIndex()
{
	shared_ptr<searchIndexes> searchOn = make_shared<searchIndexes>();
	for(shared_ptr<sIndex> idx : statement->table->indexes)
	{
		for(shared_ptr<Column> col : idx->columns)
		{
			for(shared_ptr<Condition> condition : statement->table->conditions)
			{
				if(strcasecmp(col->name.c_str(),condition->col->name.c_str()) == 0 )
				{
					//TODO  only affects char* indexes
				   if(searchOn == nullptr)
						searchOn = make_unique<searchIndexes>();
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
SEARCH sqlEngine::indexRead(shared_ptr<searchIndexes> _searchOn)
{

	search = new Search(_searchOn->index->fileStream);

	sendMessage(MESSAGETYPE::INFORMATION,presentationType,false,"indexed read on ");
	sendMessage(MESSAGETYPE::INFORMATION,presentationType,false,_searchOn->index->name);

	//TODO using one column
	shared_ptr<Column> col = _searchOn->col.front();

	if(col == nullptr)
	{
		sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Seach on column is null ");
		return SEARCH::FAILED;
	}
	if(col->value.empty())
	{
		sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Seach on column value is null ");
		return SEARCH::FAILED;;
	}

	transform(col->value.begin(), col->value.end(), col->value.begin(), ::toupper);

	if(strcasecmp(_searchOn->op.c_str(),(char*)sqlTokenEqual) == 0)
		return SEARCH::EXACT;

	if(strcasecmp(_searchOn->op.c_str(),(char*)sqlTokenGreater) == 0
	|| strcasecmp(_searchOn->op.c_str(),(char*)sqlTokenGreaterOrEqual) == 0)
		return SEARCH::FORWARD;

	if(strcasecmp(_searchOn->op.c_str(),(char*)sqlTokenLessThan) == 0
	|| strcasecmp(_searchOn->op.c_str(),(char*)sqlTokenLessOrEqual) == 0)
		return SEARCH::BACK;

	if(strcasecmp(_searchOn->op.c_str(),(char*)sqlTokenLike) == 0)
		return SEARCH::LIKE;
	
	//should not get here
	return SEARCH::FAILED;
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
        sendMessage(MESSAGETYPE::ERROR,presentationType,false, e.what());
        return nullptr;
    } 
}
/******************************************************
 * 	Rows Returned Message
 ******************************************************/
void sqlEngine::rowsReturnedMsg()
{
	sendMessage(MESSAGETYPE::INFORMATION,presentationType,true,"rows returned ");
	sendMessage(MESSAGETYPE::INFORMATION,presentationType,false,std::to_string(results->rows.size()).c_str());
}
/******************************************************
 * 	Rows Updated Message
 ******************************************************/
void sqlEngine::rowsUpdatedMsg(int _rowsUpdated)
{
	sendMessage(MESSAGETYPE::INFORMATION,presentationType,true,"rows updated ");
	sendMessage(MESSAGETYPE::INFORMATION,presentationType,false,std::to_string(_rowsUpdated).c_str());
}
