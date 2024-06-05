#pragma once
#include <string>
#include <index.h>
#include <universal.h>
#include "defines.h"
#include "utilities.cpp"

using namespace std;

struct columnParts
{
    public:
        char* fullName;
        char* tableAlias;
        char* columnName;
        char* columnAlias;
        char* fuction;
        char* value;
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
class TokenPair
{
    public:
    char* one;
    char* two;
};

/******************************************************
 * Column
 ******************************************************/
class Column
{
    public:
    char*       tableName;
    char*       name;
    char*       alias;
    bool        primary = false;
    t_function functionType;
    t_edit      edit;
    int         length = 0;
    int         position = 0;
    char*       value;
};
/******************************************************
 * Temp Column 
 ******************************************************/
class TempColumn : public Column
{
    public:
    char*   charValue       = nullptr;
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
        columnParts*    name            = nullptr;
        columnParts*    compareToName   = nullptr;
        char*           op              = nullptr;  // operator is a reserved word
        char*           prefix          = nullptr;  // (
        char*           condition       = nullptr;	// and/or
        char*           suffix          = nullptr;  // )
        char*           value           = nullptr;
        int             intValue        = 0;
        double          doubleValue     = 0;
        bool            boolValue       = false;
        t_tm            dateValue       {};
        Column*         col             {};
        Column*         compareToColumn {};
        list<char*>     valueList       {};
};
/******************************************************
 * Base Order/Group
 ******************************************************/
class OrderOrGroup
{
    public:
        Column*         col         {};
        columnParts*    name        = nullptr;
        int             columnNbr   = 0;        //Tells the sort routing which column to sort on
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
    
    if(fileName == nullptr)
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
    Index*  index;
    bool    openIndex();
};
/******************************************************
 * Open Index
 ******************************************************/
bool sIndex::openIndex()
{
    if(strlen(name) == 0)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,false,"Index name not set");
        return false;
    }
    if(strlen(fileName) == 0)
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
        list<sIndex*>    indexes{};
        list<Condition*> conditions{};
        int              recordLength = 0;
        char*            alias;
};

/******************************************************
 * Statement
 ******************************************************/
class Statement
{
    public:
        SQLACTION           action          = SQLACTION::NOACTION;
        OrderBy*            orderBy         {};
        GroupBy*            groupBy         {};     
        int                 rowsToReturn    = 0;
        id_t                exectionOrder   = 0;
        sTable*             table           {};
};

FILE* traceFile;


