#pragma once
#include <stdio.h>
#include <string.h>
#include <string>
#include <iostream>
#include <map>
#include <iomanip>
#include <sstream>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include "userException.h"

#define BUFFERHEADERLENGTH 7

#include <cgicc/CgiDefs.h> 
#include <cgicc/Cgicc.h> 

using namespace std;
using namespace cgicc;

const char * pipeRequest = "/var/www/html/request";
const char * pipeResponse = "/var/www/html/response";

enum PipeType{ REQUESTREAD, REQUESTWRITE, RESPONSEREAD, RESPONSEWRITE};

const char rowDeliminator = '\n';
const char columnDeliminator = '|';

/*---------------------------------------
   Serialize Form Data
-----------------------------------------*/
string serializeCGIFormData(Cgicc _formData)
{
	/* This is one half of a process that accept html form data as
		a CGI map and passes them through a named pipe in the form 
		of name/value pairs
	*/
	string nameValuePairString;
	string value;
	Cgicc cgi = _formData;
	const_form_iterator iter;
	try{
		for (iter = cgi.getElements().begin();
		iter != cgi.getElements().end();
		++iter)
		{
			nameValuePairString.append(iter->getName());
			nameValuePairString += columnDeliminator;
			value = iter->getValue().c_str();
			if(value.length() == 0)
			{
				value.append(" ");
			}
			nameValuePairString.append(value);
			nameValuePairString += rowDeliminator;
		}
		return nameValuePairString;
	}
	catch_and_throw_to_caller
}
/*---------------------------------------
   Deserialize Form Data
-----------------------------------------*/
void deserializeFormData(std::map<string, string> &_requestData, string _nameValue)
{
	/*  This is half of a serialize/deserialize process that passes html form data
		through a named pipes in the form of name/value pairs then loads them into 
		an STL map dictionar
	*/
	ofstream traceFile;
	traceFile.open ("pipeTrace.txt");
	traceFile << _nameValue;
	traceFile.close();

	char * token;
	string name;
	string value;
	token = strtok (&_nameValue[0],"\n");
	while(token != NULL)
	{
		string wrk = token;
		auto pos = wrk.find("|");
		if(pos > 0)
		{
			name = wrk.substr(0,pos).c_str();
			value = wrk.substr(pos+1,wrk.length() - (pos+1));
			//clear the space placed into an empty string by the Serializer
			if((value.length() == 1)
			&& (value.compare(" ") == 0))
				value.clear();
		}
		else
		{
			throw_user_exception(" request Data tab not found ");
		}
		_requestData[name] = value;
		token = strtok (NULL,"\n");
	}
}
/*---------------------------------------
   Format Pipe Message Length
-----------------------------------------*/
std::string formatPipeMessageLength(std::string message)
{
	/*  The pipe read function needs to know the length of the message
			therefore, the message length is sent in the first eight 
			characters into the write pipe buffer.

			This formats that message
	*/
    int messageLength = (int)message.length() + BUFFERHEADERLENGTH;;
    std::ostringstream oss;
    oss << std::setfill('0') << std::setw(BUFFERHEADERLENGTH) << messageLength << "\0";
    return oss.str();
}

/*---------------------------------------
   Open Pipe
-----------------------------------------*/
int openPipe(PipeType _pipeType)
{
	int pipeHandle = NEGATIVE;

	switch(_pipeType)
	{
		case PipeType::REQUESTREAD:
			pipeHandle = open(pipeRequest,O_RDONLY);
			break;
		case PipeType::REQUESTWRITE:
			pipeHandle = open(pipeRequest,O_WRONLY);
			break;
		case PipeType::RESPONSEREAD:
			pipeHandle = open(pipeResponse,O_RDONLY);
			break;
		case PipeType::RESPONSEWRITE:
			pipeHandle = open(pipeResponse,O_WRONLY);
			break;
	}

	if(pipeHandle == NEGATIVE)
	{
		string errorString("Opening pipe resulted in NEGATIVE error = ");
		errorString += std::to_string(errno);
		throw_user_exception(errorString);
	}

	return pipeHandle;
	
}
/*---------------------------------------
   Write Pipe
-----------------------------------------*/
void writePipe(PipeType _pipeType, string _messageHtml)
{
	// FIFO handle
	int pipeHandle = 0;

	string message;

	try{

		//Format message to embed length in first 7 characters

		message.append(formatPipeMessageLength(_messageHtml));

		message.append(_messageHtml);

		// open in write mode
		pipeHandle = openPipe(_pipeType);

		//write
		write(pipeHandle, message.c_str(), message.length());

		//close
		close(pipeHandle);

	}
	catch_and_throw_to_caller

}
/*---------------------------------------
   Read Pipe
-----------------------------------------*/
string readPipe(PipeType _pipeType)
{
	// FIFO handle
	int pipeHandle;

	char getPipeLengthBuffer[BUFFERHEADERLENGTH];

	try{

		// First open in read only and read
		pipeHandle = openPipe(_pipeType);


		//read first eight bytes
		read(pipeHandle, getPipeLengthBuffer, BUFFERHEADERLENGTH);


		int messageLength = 0;


		messageLength = atoi(getPipeLengthBuffer) - (BUFFERHEADERLENGTH-1);

		char* getFullMessageBuffer = new char[messageLength];

		//read full message
		read(pipeHandle, getFullMessageBuffer, messageLength);
		getFullMessageBuffer[messageLength -1] = '\0';

		//close
		close(pipeHandle);

		string returnString(getFullMessageBuffer);

		delete getFullMessageBuffer;
		
		return returnString;
	}	
	catch_and_throw_to_caller
}