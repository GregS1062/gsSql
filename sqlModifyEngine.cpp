#pragma once
#include "defines.h"
#include "sqlCommon.h"
#include "sqlEngine.cpp"
/******************************************************
 * SQL MODIFY ENGINE
 ******************************************************/
class sqlModifyEngine : public sqlEngine
{
    /*
        Engine to insert, update and delete records
    */

   public:
        ParseResult insert(shared_ptr<Statement>);
        ParseResult update(shared_ptr<Statement>);
		ParseResult tableScan(SQLACTION);
        ParseResult formatInput(char*, shared_ptr<Column>);
        ParseResult updateIndexes(long, SQLACTION);
		ParseResult useIndex(shared_ptr<searchIndexes>, SQLACTION);
		ParseResult checkPrimaryKey(shared_ptr<Column>);
		ParseResult searchForward(Search*, string, size_t, SEARCH,SQLACTION);
		ParseResult searchBack(Search*, string, size_t, SQLACTION);
		long 		appendRecord(void*, fstream*, int);
		bool 		writeRecord(void*, long, fstream*, int);
};
/******************************************************
 * Insert
 ******************************************************/
ParseResult sqlModifyEngine::insert(shared_ptr<Statement> _Statement)
{
	statement = _Statement;

	open();

	size_t count = 0;
	ParseResult returnValue = ParseResult::SUCCESS;
	char* buff = (char*)malloc(statement->table->recordLength);
	shared_ptr<Column> primaryKey;
	for(shared_ptr<Column> col : statement->table->columns)
	{
		if(col->primary)
			primaryKey = col;
		returnValue = formatInput(buff, col);
		if(returnValue == ParseResult::FAILURE)
			break;;
		count++;
	}

	if(primaryKey != nullptr)
		returnValue = checkPrimaryKey(primaryKey);

	long recordNumber;
	if(returnValue == ParseResult::SUCCESS)
		recordNumber = appendRecord(buff, tableStream, statement->table->recordLength);
	
	free(buff);

	if(returnValue == ParseResult::SUCCESS
	&& recordNumber > NEGATIVE)
		returnValue = updateIndexes(recordNumber, SQLACTION::INSERT);

	if(returnValue == ParseResult::FAILURE)
	{;
		sendMessage(MESSAGETYPE::ERROR,presentationType,true,"insert failed");
	}
	else
	{
		sendMessage(MESSAGETYPE::INFORMATION,presentationType,true,"1 record inserted");
	}

	close();

	return returnValue;
}
/******************************************************
 * Check Primary Key
 ******************************************************/
ParseResult sqlModifyEngine::checkPrimaryKey(shared_ptr<Column> _primaryKey)
{
	if(statement->table->indexes.size() == 0)
	{
		sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Primary key declared but no index found ",_primaryKey->value);
		return ParseResult::FAILURE;
	}
	for(shared_ptr<sIndex> idx : statement->table->indexes)
	{
		for(shared_ptr<Column> col : idx->columns)
		{
			if(strcasecmp(col->name.c_str(),_primaryKey->name.c_str()) == 0 )
			{
				search = new Search(idx->fileStream);
				if(search->find((char*)_primaryKey->value.c_str()) == NEGATIVE)
					return ParseResult::SUCCESS;
				else
				{
					sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Duplicate: record at ");
					sendMessage(MESSAGETYPE::ERROR,presentationType,false,_primaryKey->value);
					sendMessage(MESSAGETYPE::ERROR,presentationType,false," exists ");
					return ParseResult::FAILURE;
				}
			}
		}
	}
	return ParseResult::FAILURE;
}
/******************************************************
 * Update Record
 ******************************************************/
ParseResult sqlModifyEngine::update(shared_ptr<Statement> _statement)
{
	/*
		NOTE: Update and Delete use the same logic
	*/
	statement = _statement;

	ParseResult returnValue = ParseResult::FAILURE;;

	open();
	
	//checks the index columns against the queryColumns
	shared_ptr<searchIndexes> searchOn = determineIndex();

	//query condition but not on an indexed column
	if(searchOn == nullptr)
		returnValue = tableScan(statement->action);
	else
	    returnValue = useIndex(searchOn,statement->action);

	close();

	return returnValue;
}

/******************************************************
 * Use Index
 ******************************************************/
ParseResult sqlModifyEngine::useIndex(shared_ptr<searchIndexes> _searchOn, SQLACTION _action)
{
	SEARCH searchPath = indexRead(_searchOn);
	if(searchPath == SEARCH::FAILED)
		return ParseResult::FAILURE;

	//TODO assuming single column searches for this sprint
	shared_ptr<Column> col = _searchOn->col.front();

	if(searchPath == SEARCH::BACK)
		return searchBack(search,col->value, statement->rowsToReturn, _action);
	
	return searchForward(search,col->value, statement->rowsToReturn,searchPath,_action);
}
/******************************************************
 * Table Scan
 ******************************************************/
ParseResult sqlModifyEngine::tableScan(SQLACTION _action)
{
	
	int recordPosition 	= 0;
	int resultCount 	= 0; 
	int rowCount		= 0;

	while(true)
	{
		
		line = getRecord(recordPosition,tableStream, statement->table->recordLength);

		//End of File
		if(line == nullptr)
			break;

		if(isRecordDeleted(false))
		{
			recordPosition = recordPosition + statement->table->recordLength;
			continue;
		}

		rowCount++;

		//select top n
		if(statement->rowsToReturn > 0
		&& rowCount >= statement->rowsToReturn)
			break;

		if(queryContitionsMet(statement->table->conditions, line) == ParseResult::SUCCESS)
		{

			resultCount++;
			
			//--------------------------------------------------------
			// Update and Delete Logic
			//--------------------------------------------------------
			if(_action == SQLACTION::UPDATE
			|| _action == SQLACTION::DELETE)
			{
				if(updateIndexes(recordPosition,_action)== ParseResult::FAILURE)
				{
					sendMessage(MESSAGETYPE::ERROR,presentationType,true," Failed to update indexes");
					return ParseResult::FAILURE;
				}

				for (shared_ptr<Column> col :statement->table->columns) 
				{
					if(formatInput(line,col) == ParseResult::FAILURE)
					{
						sendMessage(MESSAGETYPE::ERROR,presentationType,true,"format input failure");
						return ParseResult::FAILURE;
					}
				}
				if(!writeRecord(line,recordPosition,tableStream,statement->table->recordLength))
				{
					sendMessage(MESSAGETYPE::ERROR,presentationType,true," Failed to write record");
					return ParseResult::FAILURE;
				}

			}

		}

		recordPosition = recordPosition + statement->table->recordLength;
	}
	sendMessage(MESSAGETYPE::INFORMATION,presentationType,true,"Table scan: rows scanned ");
	sendMessage(MESSAGETYPE::INFORMATION,presentationType,false,std::to_string(rowCount).c_str());
	sendMessage(MESSAGETYPE::INFORMATION,presentationType,true,"rows modified ");
	sendMessage(MESSAGETYPE::INFORMATION,presentationType,false,std::to_string(resultCount).c_str());
	return ParseResult::SUCCESS;
}
/******************************************************
 * Search Forward
 ******************************************************/
ParseResult sqlModifyEngine::searchForward(Search* _search, string _value, size_t _rowsToReturn, SEARCH _op,SQLACTION _action)
{
	int rowCount = 0;
	char* key = (char*)_value.c_str();
	QueryResultList* searchResults = _search->findRange(key, (int)_rowsToReturn, _op);
	
	while(searchResults != nullptr)
	{
		line = getRecord(searchResults->location,tableStream, statement->table->recordLength);
		
		//End of File
		if(line == nullptr)
			break;

		//select top n
		if(_rowsToReturn > 0
		&& results->rows.size() >= _rowsToReturn)
			break;
		
		if(queryContitionsMet(statement->table->conditions, line) == ParseResult::SUCCESS)
		{
			//--------------------------------------------------------
			// Update and Delete Logic
			//--------------------------------------------------------
			if(_action == SQLACTION::UPDATE
			|| _action == SQLACTION::DELETE)
			{
				rowCount++;
				if(updateIndexes(searchResults->location,_action)== ParseResult::FAILURE)
				{
					sendMessage(MESSAGETYPE::ERROR,presentationType,true," Failed to update indexes");
					return ParseResult::FAILURE;
				}

				for (shared_ptr<Column> col :statement->table->columns) 
				{
					if(formatInput(line,col) == ParseResult::FAILURE)
					{
						sendMessage(MESSAGETYPE::ERROR,presentationType,true,"format input failure");
						return ParseResult::FAILURE;
					}
				}
				if(!writeRecord(line,searchResults->location,tableStream,statement->table->recordLength))
				{
					sendMessage(MESSAGETYPE::ERROR,presentationType,true," Failed to write record");
					return ParseResult::FAILURE;
				}

			}
		}

		searchResults = searchResults->next;
	}

	rowsUpdatedMsg(rowCount);

	return ParseResult::SUCCESS;
}
/******************************************************
 * Search Back
 ******************************************************/
ParseResult sqlModifyEngine::searchBack(Search* _search, string _value, size_t _rowsToReturn, SQLACTION _action)
{
	char* key = (char*)_value.c_str();
	int rowCount = 0;
	long location		= 0;
	Node* _leaf = _search->findLeafBase(key);
	ScrollNode* scrollNode = _search->scrollIndexBackward(_leaf,key);


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
		if(_rowsToReturn > 0
		&& results->rows.size() >= _rowsToReturn)
			break;

		if(queryContitionsMet(statement->table->conditions, line) == ParseResult::SUCCESS)
		{
			rowCount++;
			//--------------------------------------------------------
			// Update and Delete Logic
			//--------------------------------------------------------
			if(_action == SQLACTION::UPDATE
			|| _action == SQLACTION::DELETE)
			{
				if(updateIndexes(location,_action)== ParseResult::FAILURE)
				{
					sendMessage(MESSAGETYPE::ERROR,presentationType,true," Failed to update indexes");
					return ParseResult::FAILURE;
				}

				for (shared_ptr<Column> col :statement->table->columns) 
				{
					if(formatInput(line,col) == ParseResult::FAILURE)
					{
						sendMessage(MESSAGETYPE::ERROR,presentationType,true,"format input failure");
						return ParseResult::FAILURE;
					}
				}
				if(!writeRecord(line,location,tableStream,statement->table->recordLength))
				{
					sendMessage(MESSAGETYPE::ERROR,presentationType,true," Failed to write record");
					return ParseResult::FAILURE;
				}

			}
		}
	}

	rowsUpdatedMsg(rowCount);

	return ParseResult::SUCCESS;
}


/******************************************************
 * Format Input
 ******************************************************/
ParseResult sqlModifyEngine::formatInput(char* _buff, shared_ptr<Column> _col)
{
	if(_col->value.empty())
	{
		sendMessage(MESSAGETYPE::ERROR,presentationType,true,_col->name);
		sendMessage(MESSAGETYPE::ERROR,presentationType,false," is null");
		return ParseResult::FAILURE;
	}
	if(((size_t)_col->position + _col->value.length()) > (size_t)statement->table->recordLength)
	{
		sendMessage(MESSAGETYPE::ERROR,presentationType,true,"buffer overflow on ");
		sendMessage(MESSAGETYPE::ERROR,presentationType,false,_col->name);
		return ParseResult::FAILURE;
	}
	switch(_col->edit)
	{
		case t_edit::t_bool:
		{
			memmove(&_buff[_col->position], _col->value.c_str(), _col->length);
			break;
		}
		case t_edit::t_char:
		{
			//fprintf(traceFile,"\n %d %s %d",_col->position, _col->value.c_str(), _col->length);
			memmove(&_buff[_col->position], _col->value.c_str(), _col->length);
			break;
		}
		case t_edit::t_int:
		{
			int _i = atoi(_col->value.c_str());
			memmove(&_buff[_col->position], &_i, _col->length);
			break;
		}
		case t_edit::t_double:
		{
			double _d = atof(_col->value.c_str());
			memmove(&_buff[_col->position], &_d, _col->length);
			break;
		}
		case t_edit::t_date:
		{
			t_tm _d = parseDate((char*)_col->value.c_str());
			memmove(&_buff[_col->position], &_d, _col->length);
			break;
		}
		default:
			break;
	}
	return ParseResult::SUCCESS;
}
/******************************************************
 * Update Indexes
 ******************************************************/
ParseResult sqlModifyEngine::updateIndexes(long _location, SQLACTION _action)
{	
	shared_ptr<Column> qColumn;
	//TODO joins are a problem here
	for(shared_ptr<sIndex> idx : statement->table->indexes)
	{

		if(idx == nullptr)
			return ParseResult::SUCCESS;

		if(debug)
			fprintf(traceFile,"\n index %s",idx->name.c_str());

		if(idx->columns.size() == 0)
		{
			sendMessage(MESSAGETYPE::ERROR,presentationType,true,"index has no columns");
			return ParseResult::FAILURE;
		}

		//TODO ONLY WORKS FOR SINGLE COLUMN INDEXES
		for(shared_ptr<Column> iColumn : idx->columns)
		{

			qColumn = statement->table->getColumn(iColumn->name);

			if(qColumn == nullptr)
			{
				continue;
			}

			transform(qColumn->value.begin(), qColumn->value.end(), qColumn->value.begin(), ::toupper);

			if(_action == SQLACTION::INSERT)
			{
				if(!idx->index->insertIndex->insert((char*)qColumn->value.c_str(),_location))
				{
					sendMessage(MESSAGETYPE::ERROR,presentationType,true,"insert on ");
					sendMessage(MESSAGETYPE::ERROR,presentationType,false,iColumn->name.c_str());
					sendMessage(MESSAGETYPE::ERROR,presentationType,false," failed ");
					return ParseResult::FAILURE;
				};
			}
			if(_action == SQLACTION::UPDATE)
			{
				//If not primary delete and insert column - note: primary indexes cannot change.
				if(!iColumn->primary)
				{
					if(debug)
						fprintf(traceFile,"\ncolumn name %s position %d length %li",qColumn->name.c_str(),qColumn->position,qColumn->length);
					
					char buffBeforValue[60];
					memcpy(&buffBeforValue,line+qColumn->position, qColumn->length);
					buffBeforValue[qColumn->length] = '\0';
					
					if(debug)
						fprintf(traceFile,"\nUpdate before value %s \nchanged value %s",buffBeforValue,qColumn->value.c_str());
					
					if(strcasecmp(buffBeforValue,qColumn->value.c_str()) != 0)
					{
						if(!idx->index->deleteIndex->deleteEntry(buffBeforValue,_location))
						{
							sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Delete on ");
							sendMessage(MESSAGETYPE::ERROR,presentationType,false,iColumn->name);
							sendMessage(MESSAGETYPE::ERROR,presentationType,false," failed ");
							return ParseResult::FAILURE;
						};
						if(!idx->index->insertIndex->insert((char*)qColumn->value.c_str(),_location))
						{
							sendMessage(MESSAGETYPE::ERROR,presentationType,true,"insert on ");
							sendMessage(MESSAGETYPE::ERROR,presentationType,false,iColumn->name);
							sendMessage(MESSAGETYPE::ERROR,presentationType,false," failed ");
							return ParseResult::FAILURE;
						};
					}
				};
			}
			if(_action == SQLACTION::DELETE)
			{
				if(!idx->index->deleteIndex->deleteEntry(qColumn->value,_location))
				{
					sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Delete on ");
					sendMessage(MESSAGETYPE::ERROR,presentationType,false,iColumn->name);
					sendMessage(MESSAGETYPE::ERROR,presentationType,false," failed ");
					return ParseResult::FAILURE;
				};
			}
		}
	}
	
	return ParseResult::SUCCESS;
}
/******************************************************
 * Append Record
 ******************************************************/
long sqlModifyEngine::appendRecord(void* _ptr, fstream* _file, int _size)
{
	try
	{
		_file->clear();
		_file->seekg(0, _file->end);
		long eof = _file->tellg();
		if (!_file->write((char*)_ptr, _size))
		{
			sendMessage(MESSAGETYPE::ERROR,presentationType,false,"Write to file failed");
			eof = 0;
		}
			

		_file->flush();

		return eof;
	}
	catch(const std::exception& e)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,false, e.what());
        return 0;
    } 
}
/*-----------------------------------------------------------
	Write record
-------------------------------------------------------------*/
bool sqlModifyEngine::writeRecord(void* _ptr, long _address, fstream* _file, int _size)
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
        sendMessage(MESSAGETYPE::ERROR,presentationType,false, e.what());
        return 0;
    } 
	
}
