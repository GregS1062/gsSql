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

   ParseResult	join(shared_ptr<Statement>,shared_ptr<response>);
   ParseResult 	merge(size_t, shared_ptr<response>);
   ParseResult 	searchOnKey(shared_ptr<sIndex>, size_t,shared_ptr<response>,SEARCH);
   ParseResult  joinOnKey(Search*,vector<shared_ptr<TempColumn>>,size_t,SEARCH);


};
/******************************************************
 * Join
 ******************************************************/
ParseResult sqlJoinEngine::join(shared_ptr<Statement> _statement, shared_ptr<response> _results)
{
	/*
		1) Get condition
		2) Get row column position (nbr) for key
		3) determine index or table scan
		4) match row column to key column
		5) retrieve line
		6) add table columns to row
	*/

	if(debug)
        fprintf(traceFile,"\n\n-------------------------BEGIN JOIN ENGINE-------------------------------------------");

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
	string op;

	// 1)
	for(shared_ptr<Condition> condition : statement->table->conditions)
	{
		/*	Differentiate between 'join' and 'where' conditions
			1) Where condition will have a null compareToColumn
			2) where condition will have a compareToColumn with a table name same as the column table name
		*/
		if(condition->compareToColumn == nullptr)
			continue;
		if(condition->compareToColumn->tableName.compare(condition->col->tableName) == 0)
			continue;
		//set up the keys
		key = condition->col;
		op = condition->op;
		//Once the key saved, the condition will be satisfied in the subsequent logic, so remove the condition since it is redundant
		statement->table->conditions.remove(condition);
		break;
	}

	if(key == nullptr)
	{
		sendMessage(MESSAGETYPE::ERROR,presentationType,true,"No condition found for this join ");
		return ParseResult::FAILURE;
	}

	vector<vector<shared_ptr<TempColumn>>> rows = _results->rows;
	vector<shared_ptr<TempColumn>>row = rows.at(0);

	// 2) Get the key's position in the row.
	size_t keyColumnNbr = std::string::npos;
	char* name;
	for(size_t i = 0;i < row.size();i++)
	{

		if(!row.at(i)->alias.empty())
			name = (char*)row.at(i)->alias.c_str();
		else
			name = (char*)row.at(i)->name.c_str();
		if(strcasecmp(key->name.c_str(),name) == 0 )
		{
			keyColumnNbr = (int)i;
			break;
		}
	}

	if(keyColumnNbr == std::string::npos)
	{
		sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Could not match join key column name:");
		sendMessage(MESSAGETYPE::ERROR,presentationType,false,key->name.c_str());
		return ParseResult::FAILURE;
	}

	// 3)	Find the index
	//Again, assuming one column on primary index
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

	//Condition search type transformed into an enum that btree can understand
	SEARCH searchType = getSearchType(op);
	if(index == nullptr)
	{
			//table scan
		close();
		return ParseResult::SUCCESS;
	}

	ParseResult retResult;
	if(searchOnKey(index,keyColumnNbr,_results, searchType) == ParseResult::SUCCESS)
		retResult = ParseResult::SUCCESS;
	else
	    retResult = ParseResult::FAILURE;

	close();
	return retResult;
}
/******************************************************
 * Table Scan
 ******************************************************/
ParseResult sqlJoinEngine::merge(size_t _keyColumnNbr,shared_ptr<response> _results)
{
	list<int> sortList;
	sortList.push_back((int)_keyColumnNbr);
	_results->Sort(sortList,true);
	return ParseResult::SUCCESS;
}
/******************************************************
 * Search On Key
 ******************************************************/
ParseResult sqlJoinEngine::searchOnKey(shared_ptr<sIndex> _index, size_t _keyColumnNbr,shared_ptr<response> _results, SEARCH _searchType)
{
	if(debug)
		fprintf(traceFile,"\njoin on index %s",_index->name.c_str());

	vector<shared_ptr<TempColumn>>row;
	search = new Search(_index->fileStream);
	
	// 4)
	for (size_t i = 0; i < _results->rows.size(); i++) 
	{ 
		row = (vector<shared_ptr<TempColumn>>)_results->rows[i];
		if(joinOnKey(search,row,_keyColumnNbr,_searchType) == ParseResult::FAILURE)
			break;
	}

	delete search;
	return ParseResult::SUCCESS;
}
/******************************************************
 * Join On Key
 ******************************************************/
ParseResult sqlJoinEngine::joinOnKey(Search* _search,vector<shared_ptr<TempColumn>> _row, size_t _keyColumnNbr,SEARCH _searchType)
{
	/*  1) _row = the single row of product from the previous selects and joins
		2) searchResults will be the rows produced by the join statement  (for instance row might be an order and searchResults may be the line items)
		3) merge these products
	*/
	char* key = (char*)_row.at(_keyColumnNbr)->charValue.c_str();
	vector<shared_ptr<TempColumn>>	newRow;

	QueryResultList* searchResults = _search->findRange(key, 0, _searchType);

	while(searchResults != nullptr)
	{
		// A no-hit is legit
		if( searchResults->location == NEGATIVE)
			return ParseResult::SUCCESS;

		line = getRecord(searchResults->location,tableStream, statement->table->recordLength);

		//End of File
		if(line == nullptr)
		{
			sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Invalid location at index key ");
			return ParseResult::FAILURE;
		}

		if(queryContitionsMet(statement->table->conditions, line) == ParseResult::SUCCESS)
		{
			//product of join statement
			newRow = outputLine	(statement->table->columns);
			//previous product
			for(size_t i=0;i<_row.size();i++)
			{	
				//merge
				newRow.push_back(_row.at(i));
			}
			results->addRow(newRow);

		}
		searchResults = searchResults->next;
	}

	return ParseResult::SUCCESS;
}

