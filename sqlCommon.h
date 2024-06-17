#pragma once
#include <string>
#include <index.h>
#include <universal.h>
#include "defines.h"
#include "utilities.cpp"
#include "userException.h"

using namespace std;

struct columnParts
{
    public:
        string fullName;
        string tableAlias;
        string columnName;
        string columnAlias;
        string fuction;
        string value;
};

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
struct TokenPair
{
    string one;
    string two;
};

/******************************************************
 * Column
 ******************************************************/
class Column
{
    public:
    string          tableName{};
    string          name{};
    string          alias{};
    bool            primary         = false;
    t_function      functionType    = t_function::NONE;
    t_edit          edit            = t_edit::t_undefined;
    int             length          = 0;
    int             position        = 0;
    string          value{};
};
/******************************************************
 * Temp Column 
 ******************************************************/
class TempColumn : public Column
{
    public:
    string  charValue       {};
    int     intValue        = 0;
    double  doubleValue     = 0;
    bool    boolValue       = false;
    t_tm    dateValue       = {};
};
/******************************************************
 * Condition
 ******************************************************/
class Condition
{
    public:
        shared_ptr<columnParts>    name            = nullptr;
        shared_ptr<columnParts>   compareToName   = nullptr;
        string           op             {};  // operator is a reserved word
        string           prefix         {};  // (
        string           condition      {};	// and/or
        string           suffix         {};  // )
        string           value          {};
        int             intValue        = 0;
        double          doubleValue     = 0;
        bool            boolValue       = false;
        t_tm            dateValue       {};
        Column*         col             {};
        Column*         compareToColumn {};
        list<string>     valueList       {};
};
/******************************************************
 * Base Order/Group
 ******************************************************/
class OrderOrGroup
{
    public:
        Column*                     col         {};
        shared_ptr<columnParts>     name        = nullptr;
        int                         columnNbr   = 0;        //Tells the sort routing which column to sort on
};
/******************************************************
 * Order By
 ******************************************************/
class OrderBy
{
    public:
        list<OrderOrGroup>      order{};
        bool    asc             = true;
};
/******************************************************
 * Group By
 ******************************************************/
class GroupBy
{
    public:
        list<OrderOrGroup>      group{}; 
        list<Condition*>        having{};
};


/******************************************************
 * Base
 ******************************************************/
class BaseData
{
    public:
        fstream*                    fileStream = nullptr;
        string                      name;
        string                      fileName;
        list<shared_ptr<Column>>    columns{};
        fstream*                    open();
        void                        close();
        shared_ptr<Column>          getColumn(string);
};
/******************************************************
 * Base Open
 ******************************************************/
fstream* BaseData::open()
{
    
    if(fileName.length() == 0)
        return nullptr;

    ////Open index file
    fileStream = new fstream{};
    fileStream->open(fileName, ios::in | ios::out | ios::binary);
    if (!fileStream->is_open()) {
        sendMessage(MESSAGETYPE::ERROR,presentationType,false,fileName);
        sendMessage(MESSAGETYPE::ERROR,presentationType,false," not opened ");
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
shared_ptr<Column> BaseData::getColumn(string _name)
{

    for (shared_ptr<Column> col : columns) {
        if(strcasecmp(col->name.c_str(),_name.c_str()) == 0)
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
    Index*  index;
    bool    openIndex();
};
/******************************************************
 * Open Index
 ******************************************************/
bool sIndex::openIndex()
{
    if(name.length() == 0)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,false,"Index name not set");
        return false;
    }
    if(fileName.length() == 0)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,false,"Index filename not set");
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
        list<shared_ptr<sIndex>>    indexes{};
        list<shared_ptr<Condition>> conditions{};
        int                         recordLength = 0;
        string                      alias;
};

/******************************************************
 * Statement
 ******************************************************/
class Statement
{
    public:
        SQLACTION           action          = SQLACTION::NOACTION;
        shared_ptr<OrderBy> orderBy         {};
        shared_ptr<GroupBy> groupBy         {};     
        int                 rowsToReturn    = 0;
        id_t                exectionOrder   = 0;
        shared_ptr<sTable>  table           {};
};

FILE* traceFile;


