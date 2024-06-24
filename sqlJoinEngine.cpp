#pragma once
#include "defines.h"
#include "sqlCommon.h"
#include "sqlEngine.cpp"
/******************************************************
 * SQL SELECT ENGINE
 ******************************************************/
class sqlJoinEngine : public sqlEngine
{
    /*
        Engine to Join records
    */

   public:

   ParseResult	join(shared_ptr<Statement>,shared_ptr<tempFiles>);
   ParseResult  joinOnKey(Search*,vector<shared_ptr<TempColumn>>,size_t);

};
/******************************************************
 * Join
 ******************************************************/
ParseResult sqlJoinEngine::join(shared_ptr<Statement> _statement, shared_ptr<tempFiles> _results)
{
	/*
		1) Get condition
		2) Get row column position (nbr) for key
		3) determine index or table scan
		4) match row column to key column
		5) retrieve line
		6) add table columns to row
	*/

	if(_results == nullptr)
	{
		sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Join results are null ");
		return ParseResult::FAILURE;
	}

	// Nothing to join with
	if(_results->rows.size() == 0)
		return ParseResult::SUCCESS;

	statement = _statement;

	open();

	//Assuming indexed read on one key column

	shared_ptr<Column> key{};

	// 1)
	for(shared_ptr<Condition> condition : statement->table->conditions)
	{
		key = condition->col;
		break;
	}
	if(key == nullptr)
	{
		sendMessage(MESSAGETYPE::ERROR,presentationType,true,"No condition found for this join ");
		return ParseResult::FAILURE;
	}

	vector<vector<shared_ptr<TempColumn>>> rows = _results->rows;
	vector<shared_ptr<TempColumn>>row = rows.at(0);

	// 2)
	int keyColumnNbr = NEGATIVE;
	char* name;
	for(size_t i = 0;i < row.size();i++)
	{

		if(!row.at(i)->alias.empty())
			name = (char*)row.at(i)->alias.c_str();
		else
			name = (char*)row.at(i)->name.c_str();
		if(debug)
			fprintf(traceFile,"\nmatching %s to column %s",key->name.c_str(),name);
		if(strcasecmp(key->name.c_str(),name) == 0 )
		{
			keyColumnNbr = (int)i;
			break;
		}
	}

	if(keyColumnNbr == NEGATIVE)
	{
		sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Could not match join key column name:");
		sendMessage(MESSAGETYPE::ERROR,presentationType,false,key->name.c_str());
		return ParseResult::FAILURE;
	}

	// 3)	//Again, assuming one column on primary index
	shared_ptr<sIndex> index{};
	for(shared_ptr<sIndex> idx : statement->table->indexes)
	{
		for(shared_ptr<Column> col : idx->columns)
		{
			if(strcasecmp(col->name.c_str(),key->name.c_str()) == 0 )
			{
				index = idx;
				break;
			}
		}
	}

	if(index == nullptr)
	{
		sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Cannot find join index on ");
		sendMessage(MESSAGETYPE::ERROR,presentationType,false,key->name);
		return ParseResult::FAILURE;
	}
	if(debug)
		fprintf(traceFile,"\njoin on index %s",index->name.c_str());

	search = new Search(index->fileStream);
	
	// 4)
	for (size_t i = 0; i < _results->rows.size(); i++) 
	{ 
		row = (vector<shared_ptr<TempColumn>>)_results->rows[i];
		if(joinOnKey(search,row,(size_t)keyColumnNbr) == ParseResult::FAILURE)
			break;
	}

	delete search;
	close();
	return ParseResult::SUCCESS;
}
/******************************************************
 * Join On Key
 ******************************************************/
ParseResult sqlJoinEngine::joinOnKey(Search* _search,vector<shared_ptr<TempColumn>> _row, size_t _keyColumnNbr)
{
	vector<shared_ptr<TempColumn>>	newRow;
	long location = _search->find((char*)_row.at(_keyColumnNbr)->charValue.c_str());

	if(debug)
		fprintf(traceFile,"\n join key %s location %ld",_row.at(_keyColumnNbr)->charValue.c_str(),location);

	// A no-hit is legit
	if( location == NEGATIVE)
	 	return ParseResult::SUCCESS;

	line = getRecord(location,tableStream, statement->table->recordLength);

	//End of File
	if(line == nullptr)
	{
		sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Invalid location at index key ");
		return ParseResult::FAILURE;
	}


	newRow = outputLine	(statement->table->columns);
	for(size_t i=0;i<_row.size();i++)
	{
		newRow.push_back(_row.at(i));
	}
	results->addRow(newRow);

	return ParseResult::SUCCESS;
}

