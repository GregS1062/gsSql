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

   ParseResult	join(Statement*,vector<vector<TempColumn*>>);
   ParseResult  joinOnKey(Column* key);
};
/******************************************************
 * Join
 ******************************************************/
ParseResult sqlJoinEngine::join(Statement* _statement, vector<vector<TempColumn*>> _rows)
{
	/*
		Simple join on indexed column
	*/
	statement = _statement;
	vector<TempColumn*>row;

	if(_rows.size() == 0)
		return;


	for (size_t i = 0; i < _rows.size(); i++) { 
		row = (vector<TempColumn*>)_rows[i];
		if(row.size() > 0)
			//printRow(row);
	}

	return ParseResult::SUCCESS;
}
/******************************************************
 * Join On Key
 ******************************************************/
ParseResult sqlJoinEngine::joinOnKey(Column* _key)
{

	for(Condition* condition :statement->table->conditions)
	{
		
	}
	for(sIndex* idx : statement->table->indexes)
	{
		for(Column* col : idx->columns)
		{
			if(strcasecmp(col->name,_key->name) == 0 )
			{
				search = new Search(idx->fileStream);
				if(search->find(_key->value) == NEGATIVE)
					return ParseResult::SUCCESS;
				else
				{
					sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Duplicate: record at ");
					sendMessage(MESSAGETYPE::ERROR,presentationType,false,_key->value);
					sendMessage(MESSAGETYPE::ERROR,presentationType,false," exists ");
					return ParseResult::FAILURE;
				}
			}
		}
	}
	return ParseResult::FAILURE;
}

