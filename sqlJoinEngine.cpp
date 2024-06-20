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

   ParseResult	join(Statement,resultList*);
   ParseResult  joinOnKey(Search*,vector<TempColumn*>,size_t);

};
/******************************************************
 * Join
 ******************************************************/
ParseResult sqlJoinEngine::join(Statement _statement, resultList* _results)
{
	/*
		1) Get condition
		2) Get row column position (nbr) for key
		3) determine index or table scan
		4) match row column to key column
		5) retrieve line
		6) add table columns to row
	*/

	// Nothing to join with
	if(_results->rows.size() == 0)
		return ParseResult::SUCCESS;

	statement = &_statement;

	open();

	//Assuming indexed read on one key column

	Column* key{};

	// 1)
	for(Condition* condition : statement->table->conditions)
	{
		key = condition->col;
		break;
	}
	if(key == nullptr)
	{
		sendMessage(MESSAGETYPE::ERROR,presentationType,true,"No condition found for this join ");
		return ParseResult::FAILURE;
	}

	vector<vector<TempColumn*>> rows = _results->rows;
	vector<TempColumn*>row = rows.at(0);

	// 2)
	int keyColumnNbr = NEGATIVE;
	char* name;
	for(size_t i = 0;i < row.size();i++)
	{

		if(row.at(i)->alias != nullptr)
			name = row.at(i)->alias;
		else
			name = row.at(i)->name;
		if(debug)
			fprintf(traceFile,"\nmatching %s to column %s",key->name,name);
		if(strcasecmp(key->name,name) == 0 )
		{
			keyColumnNbr = (int)i;
			break;
		}
	}

	if(keyColumnNbr == NEGATIVE)
	{
		sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Could not match join key column name:");
		sendMessage(MESSAGETYPE::ERROR,presentationType,false,key->name);
		return ParseResult::FAILURE;
	}

	// 3)	//Again, assuming one column on primary index
	sIndex* index{};
	for(sIndex* idx : statement->table->indexes)
	{
		for(Column* col : idx->columns)
		{
			if(strcasecmp(col->name,key->name) == 0 )
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
		fprintf(traceFile,"\njoin on index %s",index->name);

	search = new Search(index->fileStream);
	
	// 4)
	for (size_t i = 0; i < _results->rows.size(); i++) 
	{ 
		row = (vector<TempColumn*>)_results->rows[i];
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
ParseResult sqlJoinEngine::joinOnKey(Search* _search,vector<TempColumn*> _row, size_t _keyColumnNbr)
{
	vector<TempColumn*>	newRow;
	long location = _search->find(_row.at(_keyColumnNbr)->charValue);

	if(debug)
		fprintf(traceFile,"\n join key %s location %ld",_row.at(_keyColumnNbr)->charValue,location);

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

