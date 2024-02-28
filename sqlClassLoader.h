#pragma once
#include <iostream>
#include <map>
#include <string>
#include <cstdio>
#include <list>
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
#define lit_indexList           "indexes"
#define lit_index               "index"
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
};

class baseData
{
    protected:
        fstream* fileStream = new fstream{};
    public:
        string          name;
        string          fileName;
        list<column*>   columns;
        fstream*        open();
        void            close();
};
fstream* baseData::open()
{
		////Open index file
		fileStream->open(fileName, ios::in | ios::out | ios::binary);
		if (!fileStream->is_open()) {
            errText.append(fileName);
            errText.append(" not opened ");
			return nullptr;
		}
        return fileStream;
}
void baseData::close()
{
    fileStream->close();
}
class cIndex : public baseData
{
    public:
        cIndex*         next = nullptr;
};
class cTable : public baseData
{  
    public:
        list<cIndex*>    indexes;
        int             recordLength = 0;
        cTable*         next = nullptr;
};


class sqlClassLoader
{
    list<cTable*> tables;
    public:
        ParseResult loadSqlClasses(const char*, const char*);
        ParseResult calculateTableColumnValues(cTable*);
        ParseResult loadTableColumns(keyValue*, cTable*);
        ParseResult loadIndexColumns(keyValue*, cIndex*);
        ParseResult loadColumnValues(keyValue*, column*);
        ParseResult loadIndexes(keyValue*, cTable*);
        ParseResult loadIndexValues(keyValue*, column*);
        ParseResult loadTables(valueList*);
        cTable*     getTableByName(char*);
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
ParseResult sqlClassLoader::calculateTableColumnValues(cTable* _table)
{
    try
    {
        int recordLength        = 0;
        int position            = 0;
        for(column* c : _table->columns)
        {
            c->position     = position;
            position        = position + c->length;
            recordLength    = recordLength + c->length;
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
ParseResult sqlClassLoader::loadTableColumns(keyValue* _tableKV, cTable* _table)
{
    try
    {

        valueList* columnList = getNodeList(_tableKV,lit_columnList);
        while(columnList != nullptr)
        {
            
            keyValue* kv = (keyValue*)columnList->value;
            valueList* v2 = (valueList*)kv->value;
            if(v2 != nullptr)
            {
                if(v2->value != nullptr)
                {
                    if(v2->t_type == t_string)
                    {
                        column* c = new column();
                        c->name = (char*)v2->value;
                        if(debug)
                            printf("\n column %s", c->name.c_str());
                        loadColumnValues(kv,c);
                        _table->columns.push_back(c);
                    }
                }

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
 * Load Columns
 ******************************************************/
ParseResult sqlClassLoader::loadIndexColumns(keyValue* _indexKV, cIndex* _index)
{
    try
    {

        valueList* columnList = getNodeList(_indexKV,lit_columnList);
        while(columnList != nullptr)
        {
            
            keyValue* kv = (keyValue*)columnList->value;
            valueList* v2 = (valueList*)kv->value;
            if(v2 != nullptr)
            {
                if(v2->value != nullptr)
                {
                    if(v2->t_type == t_string)
                    {
                        column* c = new column();
                        c->name = (char*)v2->value;

                        if(debug)
                            printf("\n column %s", c->name.c_str());

                        loadColumnValues(kv,c);
                        _index->columns.push_back(c);
                    }
                }

            }
            columnList = columnList->next;
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
 * Load Indexes
 ******************************************************/
ParseResult sqlClassLoader::loadIndexes(keyValue* _tableKV, cTable* _table)
{
    try
    {

        valueList* indexList = getNodeList(_tableKV,lit_indexList);
        while(indexList != nullptr)
        {
            keyValue* kv = (keyValue*)indexList->value;
            if(debug)
                printf("\n table = %s", _table->name.c_str());
            valueList* v2 = (valueList*)kv->value;
            if(v2->t_type == t_string)
            {
                cIndex* index = new cIndex();
                index->name.append((char*)v2->value);
                index->fileName = getMemberValue(kv,"location");
                if(debug)
                {
                    printf("\n index name = %s",index->name.c_str());
                    printf("\n index location = %s",index->fileName.c_str());
                }
                loadIndexColumns(kv,index);
                _table->indexes.push_back(index);
            }
            indexList = indexList->next;
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
 * Load Tables
 ******************************************************/
ParseResult sqlClassLoader::loadTables(valueList* _tableList)
{
    try{
        while(_tableList != nullptr)
        {
            if(_tableList->t_type == t_Object)
            {
                cTable* t = new cTable();
                keyValue* kv = (keyValue*)_tableList->value;

                if(strcmp(kv->key,lit_table) != 0)
                {
                    if(debug)
                        printf("\n key = %s", kv->key);
                    break;
                }

                valueList* v2 = (valueList*)kv->value;
                if(v2->t_type == t_string)
                {
                    t->name = (char*)v2->value;
                }
                t->fileName = getMemberValue(kv,"location");
 
                if(loadTableColumns(kv,t) == ParseResult::FAILURE)
                    return ParseResult::FAILURE;

                if(loadIndexes(kv,t) == ParseResult::FAILURE)
                    return ParseResult::FAILURE;

                tables.push_back(t);
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
cTable* sqlClassLoader::getTableByName(char* _tableName)
{
    for(cTable* table : tables)
    {
        if(strcmp(table->name.c_str(), _tableName) == 0)
            return table;
    }
    return nullptr;
}