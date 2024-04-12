#pragma once
#include <string>
#include "defines.h"
#include "utilities.h"
#include "universal.h"
#include "index.h"

using namespace std;

class ReturnResult{
  public:
  string resultTable;
  string message;
  string error;
};

ReturnResult returnResult;

class TokenPair
{
    public:
    char* one;
    char* two;
};

class ColumnAlias
{
    public:
    char* columnName;
    char* tableName;
};

class Column
{
    public:
    char*   name;
    bool    primary = false;
    t_edit  edit;
    int     length = 0;
    int     position = 0;
    char*   value;
    char*   tableName;
};
class Condition
{
    public:
        char*   name            = nullptr;  // described by user
        char*   op              = nullptr;  // operator is a reserved word
        char*   value           = nullptr;
        int     intValue        = 0;
        double  doubleValue     = 0;
        bool    boolValue       = false;
        t_tm    dateValue;
        char*   prefix          = nullptr;  // (
        char*   condition       = nullptr;	// and/or
        char*   suffix          = nullptr;  // )
        Column* col             = new Column();                     // edit of column in condition
};

class BaseData
{
    public:
        fstream*            fileStream = nullptr;
        char*               name;
        char*               fileName;
        list<Column*>       columns;
        fstream*            open();
        void                close();
        Column*             getColumn(char*);
};
fstream* BaseData::open()
{
		////Open index file
        fileStream = new fstream{};
		fileStream->open(fileName, ios::in | ios::out | ios::binary);
		if (!fileStream->is_open()) {
            utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false,fileName);
            utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false," not opened ");
			return nullptr;
		}
        return fileStream;
};
void BaseData::close()
{
    if(fileStream != nullptr)
    {
        if(fileStream->is_open())
        {
            fileStream->close();
        }
    } 
};
Column* BaseData::getColumn(char* _name)
{

    for (Column* col : columns) {
        if(strcasecmp(col->name,_name) == 0)
            return col;
    }
    return nullptr;
};
class sIndex : public BaseData
{
    public:
    Index* index;
    bool openIndex();
};
bool sIndex::openIndex()
{
    if(strlen(name) == 0)
    {
        utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false,"Index name not set");
        return false;
    }
    if(strlen(fileName) == 0)
    {
        utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false,"Index filename not set");
        return false;
    }
    fileStream = open();
    index = new Index(fileStream);
    return true;
};

class sTable : public BaseData
{  
    public:
        list<sIndex*>    indexes;
        list<Condition*> conditions;
        int              recordLength = 0;
        char*            alias;
};

class Results
{
    PRESENTATION        presentation;
    int rowCount        = 0;
    list<Column*>       columns;
    list<list<Column*>> rows;
};
class Plan
{
    public:
        SQLACTION   action;
        int         orderOfExecution;
        int         rowsToReturn;
};
class Statement
{
    public:
    Plan*       plan = new Plan();
    sTable*     table;
    Results*    results = new Results();
};
class Execution
{
    
};


