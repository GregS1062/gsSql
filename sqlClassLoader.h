#pragma once
#include <iostream>
#include <map>
#include <string>
#include <cstdio>
#include "keyValue.h"
#include "parseJason.h"
#include "global.h"

using namespace std;

#define lit_databaseList        "databases"
#define lit_database            "database"
#define lit_tableList           "tables"
#define lit_table               "table"
#define lit_columnList          "columns"
#define lit_column              "column"
#define lit_type                "type"
#define lit_char                "char"
#define lit_int                 "int"

enum t_edit{
    t_char,
    t_int,
    t_date
};

class column
{
    public:
    string name;
    t_edit edit;
    int length = 0;
    int position = 0;
    column* next = nullptr;
};
class table
{
    protected:
        fstream* tableFile = new fstream{};
    
    public:
        string      name;
        string      fileName;
        int         recordLength = 0;
        column*     columnHead = nullptr;
        table*      next = nullptr;
        fstream*    open();
        void        close();
};
fstream* table::open()
{

		////Open index file
		tableFile ->open(fileName, ios::in | ios::out | ios::binary);
		if (!tableFile ->is_open()) {
			return nullptr;
		}
        return tableFile;
}
void table::close()
{
    tableFile->close();
}


class sqlClassLoader
{
    table* tableHead;
    table* tableTail;
    public:
        ParseResult calculateTableColumnValues(table*);
        ParseResult loadSqlClasses(const char*, const char*);
        ParseResult calculateTableColumnValues(table* _table);
        ParseResult loadColumnValues(keyValue*, column*);
        ParseResult loadColumns(keyValue*, table*);
        ParseResult loadTables(valueList*);
        table*      getTableByName(table*, char*);
};
/*-------------------------------------------------------------
    These functions read through the hierachy of key/value and 
    value lists built from a json file and load a hierarchy
    of classes to define the database, tables and columns
 ------------------------------------------------------------*/
 /******************************************************
 * Load Sql Classes
 ******************************************************/
ParseResult sqlClassLoader::loadSqlClasses(const char* _jasonFile, const char* _database)
{
    try
    {
        keyValue* jasonDef = parseJasonDatabaseDefinition(_jasonFile);
        valueList* dbValueList = getNodeList(jasonDef,lit_databaseList);
        keyValue* databaseDef = getMember(dbValueList,_database);
        if(databaseDef == nullptr)
        {
            errText.append("<p> Failed to find database ");
            errText.append(_database);
            return ParseResult::FAILURE;
        }

        valueList* tableList = getNodeList(databaseDef,lit_tableList);
        if(tableList == nullptr)
        {
            errText.append("<p> Failed to find table list for database ");
            errText.append(_database);
            return ParseResult::FAILURE;
        }

        return loadTables(tableList);
    }
    catch(const std::exception& e)
    {
        errText.append( e.what());
        return ParseResult::FAILURE;
    }
    
}
/******************************************************
 * Calculate Table Column Values
 ******************************************************/
ParseResult sqlClassLoader::calculateTableColumnValues(table* _table)
{
    try
    {
        int recordLength        = 0;
        int position            = 0;
        column* c               = _table->columnHead;
        while(c != nullptr)
        {
            c->position     = position;
            position        = position + c->length;
            recordLength    = recordLength + c->length;
            c = c->next;
        }
        _table->recordLength = recordLength;
        return ParseResult::SUCCESS;
    }
    catch(const std::exception& e)
    {
        errText.append( e.what());
        return ParseResult::FAILURE;
    }
}
/******************************************************
 * Load Column Values
 ******************************************************/
ParseResult sqlClassLoader::loadColumnValues(keyValue* _columnKV, column* _column)
{
    try
    {
        keyValue* kv = _columnKV;
        while(kv != nullptr)
        {
            if(strcmp(kv->key,lit_type) == 0 )
            {
                valueList* vl = kv->value;
                keyValue* typekv = (keyValue*)vl->value;
                if(strcmp(typekv->key,lit_char) == 0)
                    _column->edit = t_char;
                if(strcmp(typekv->key,lit_int) == 0)
                    _column->edit = t_int;
                valueList* vl2 = (valueList*)typekv->value;
                if(vl2->t_type == t_string)
                {
                    _column->length = atoi((char*)vl2->value);
                }
            }
            kv = kv->next;
        }
        return ParseResult::SUCCESS;
    }
    catch(const std::exception& e)
    {
        errText.append( e.what());
        return ParseResult::FAILURE;
    }
}
/******************************************************
 * Load Columns
 ******************************************************/
ParseResult sqlClassLoader::loadColumns(keyValue* _tableKV, table* _table)
{
    try
    {
        column* tail = new column();

        valueList* columnList = getNodeList(_tableKV,lit_columnList);
        while(columnList != nullptr)
        {
            keyValue* kv = (keyValue*)columnList->value;
            valueList* v2 = (valueList*)kv->value;
            if(v2->t_type == t_string)
            {
                column* c = new column();
                c->name = (char*)v2->value;
                loadColumnValues(kv,c);
                if(_table->columnHead == nullptr)
                {
                    _table->columnHead = c;
                    tail = c;
                }
                column* temp = tail;
                tail = c;
                temp->next = c;
            }
            columnList = columnList->next;
        }
        calculateTableColumnValues(_table);
        return ParseResult::SUCCESS;
    }
    catch(const std::exception& e)
    {
        errText.append( e.what());
        return ParseResult::FAILURE;
    }

}
/******************************************************
 * Load Tables
 ******************************************************/
ParseResult sqlClassLoader::loadTables(valueList* _tableList)
{
    try{
        while(_tableList != nullptr)
        {
            if(_tableList->t_type == t_Object)
            {
                table* t = new table();
                keyValue* kv = (keyValue*)_tableList->value;
                valueList* v2 = (valueList*)kv->value;
                if(v2->t_type == t_string)
                {
                    t->name = (char*)v2->value;
                }
                t->fileName = getMemberValue(kv,"location");
 
                if(loadColumns(kv,t) == ParseResult::FAILURE)
                    return ParseResult::FAILURE;

                if(tableHead == nullptr)
                {
                    tableHead = t;
                    tableTail = t;
                }
                else
                {
                    table* temp = tableTail;
                    tableTail = t;
                    temp->next = t;
                }
            }
            _tableList = _tableList->next;
        }
        return ParseResult::SUCCESS;
    }
    catch(const std::exception& e)
    {
        errText.append( e.what());
        return ParseResult::FAILURE;
    }

}

/******************************************************
 * Get Table By Name
 ******************************************************/
table* sqlClassLoader::getTableByName(table* _tableList, char* _tableName)
{
    while(_tableList != nullptr)
    {
        if(strcmp(_tableList->name.c_str(), _tableName) == 0)
            return _tableList;
        _tableList = _tableList->next;
    }
    return nullptr;
}