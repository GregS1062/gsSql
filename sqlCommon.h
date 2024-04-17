#pragma once
#include <string>
#include "defines.h"
#include "utilities.h"
#include "universal.h"
#include "index.h"

using namespace std;

/******************************************************
 * Result
 ******************************************************/
class ReturnResult{
  public:
  string resultTable;
  string message;
  string error;
};

ReturnResult returnResult;

/******************************************************
 * Token Pair
 ******************************************************/
class TokenPair
{
    public:
    char* one;
    char* two;
};

/******************************************************
 * Column Alias
 ******************************************************/
class ColumnAlias
{
    public:
    char* columnName;
    char* tableName;
};

/******************************************************
 * Column
 ******************************************************/
class Column
{
    public:
    char*   tableName;
    char*   name;
    bool    primary = false;
    t_edit  edit;
    int     length = 0;
    int     position = 0;
    char*   value;
};
/******************************************************
 * Order By
 ******************************************************/
class OrderBy
{
    public:
        Column* col;
        char*   DescAsc;
};
/******************************************************
 * Condition
 ******************************************************/
class Condition
{
    public:
        char*   name            = nullptr;
        char*   compareToName   = nullptr;
        char*   op              = nullptr;  // operator is a reserved word
        char*   prefix          = nullptr;  // (
        char*   condition       = nullptr;	// and/or
        char*   suffix          = nullptr;  // )
        char*   value           = nullptr;
        int     intValue        = 0;
        double  doubleValue     = 0;
        bool    boolValue       = false;
        t_tm    dateValue;
        Column* col;
        Column* compareToColumn;
        list<char*> valueList {};
};

/******************************************************
 * Base
 ******************************************************/
class BaseData
{
    public:
        fstream*            fileStream = nullptr;
        char*               name;
        char*               fileName;
        list<Column*>       columns{};
        fstream*            open();
        void                close();
        Column*             getColumn(char*);
};
/******************************************************
 * Base Open
 ******************************************************/
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
/******************************************************
 * Base Close
 ******************************************************/
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
/******************************************************
 * Base Get Column
 ******************************************************/
Column* BaseData::getColumn(char* _name)
{

    for (Column* col : columns) {
        if(strcasecmp(col->name,_name) == 0)
            return col;
    }
    return nullptr;
};


/******************************************************
 * Index
 ******************************************************/
class sIndex : public BaseData
{
    public:
    Index* index;
    bool openIndex();
};
/******************************************************
 * Open Index
 ******************************************************/
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


/******************************************************
 * Table
 ******************************************************/
class sTable : public BaseData
{  
    public:
        list<sIndex*>    indexes{};
        list<Condition*> conditions{};
        list<OrderBy*>   orderBy{};
        list<Condition*> groupBy{};
        int              recordLength = 0;
        char*            alias;
};

/******************************************************
 * Results
 ******************************************************/
class Results
{
    PRESENTATION        presentation;
    int rowCount        = 0;
    list<Column*>       columns{};
    list<list<Column*>> rows{};
};

/******************************************************
 * Plan
 ******************************************************/
class Plan
{
    public:
        SQLACTION   action;
        int         orderOfExecution;
        int         rowsToReturn;
};

/******************************************************
 * Statement
 ******************************************************/
class Statement
{
    public:
    Plan*       plan = new Plan();
    sTable*     table;
    Results*    results = new Results();
};

/******************************************************
 * Execution
 ******************************************************/
class Execution
{
    
};


