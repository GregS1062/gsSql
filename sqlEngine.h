#pragma once

#include <system_error>
#include <iostream>
#include <fstream>
#include <cstdint>
#include <filesystem>

#include "global.h"
#include "sqlClassLoader.h"
#include "parseSql.h"

using namespace std;

class sqlEngine
{
	/***************************************
	 * Assuming a single table for now
	*******************************************/
	table* queryTable;
	fstream* tableFile;
	public:
		static	void*	getRecord(long, fstream*, int);
    	sqlEngine(sqlParser*, table*);
};

sqlEngine::sqlEngine(sqlParser* _query, table* _table)
{
    /***************************************
	 * Assuming a single table for now
	*******************************************/

		////Open index file
		tableFile =_table->open();

}

/*-----------------------------------------------------------
	Get (read) record
-------------------------------------------------------------*/
void* sqlEngine::getRecord(long _address, fstream* _file, int _size)
{
	try
	{
		if (_size < 1)
		{
			return nullptr;
		}

		void* _ptr = malloc(_size);

		_file->clear();

		if (!_file->seekg(_address))
		{
			free(_ptr);
			return nullptr;
		}

		if (!_file->read(reinterpret_cast<char*>(_ptr), _size))
			_ptr = nullptr;
		
		
		_file->flush();

		return _ptr;
	}
    catch(const std::exception& e)
    {
        errText.append( e.what());
        return nullptr;
    } 
}