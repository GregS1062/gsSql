#pragma once

#include <system_error>
#include <iostream>
#include <fstream>
#include <cstdint>
#include <filesystem>

using namespace std;

class DataAccess
{
public:
	static	void*	getRecord(long, fstream*, int);
	static	bool	writeRecord(void*, long, fstream*, int);
	static	long	appendRecord(void*, fstream*, int);
};

/*-----------------------------------------------------------
	Get (read) record
-------------------------------------------------------------*/
void* DataAccess::getRecord(long _address, fstream* _file, int _size)
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
	catch_and_throw_to_caller
}
/*-----------------------------------------------------------
	Write record
-------------------------------------------------------------*/
bool DataAccess::writeRecord(void* _ptr, long _address, fstream* _file, int _size)
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
	catch_and_throw_to_caller
	
}
/*-----------------------------------------------------------
	Append record
-------------------------------------------------------------*/
long DataAccess::appendRecord(void* _ptr, fstream* _file, int _size)
{
	try
	{
		_file->clear();
		_file->seekg(0, _file->end);
		long eof = _file->tellg();
		if (!_file->write((char*)_ptr, _size))
			throw_user_exception("Write to file failed");

		_file->flush();

		return eof;
	}
	catch_and_throw_to_caller
}