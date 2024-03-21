#pragma once
#include <string>
#include <string.h>
#include <stdio.h>
#include <list>
#include <vector>
#include <algorithm>
#include <iostream>
#include <map>
#include <cstdio>
#include <fstream>
#include "sqlCommon.h"
#include "index.h"
#include "tokenParser.h"

using namespace std;


class column
{
    public:
    string  name;
    bool    primary = false;
    t_edit  edit;
    int     length = 0;
    int     position = 0;
    char*   value;
};

class baseData
{
 
    public:
        fstream* fileStream;
        string              name;
        string              fileName;
        map<char*,column*>  columns;
        map<char*,column*>::iterator columnItr;
        fstream*            open();
        void                close();
};
fstream* baseData::open()
{
		////Open index file
        fileStream = new fstream{};
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
    //All the relevant variable are in baseData
    public:
    Index* index;
};
class cTable : public baseData
{  
    public:
        list<cIndex*>   indexes;
        int             recordLength = 0;
        column*         getColumn(char* _name);
};
column* cTable::getColumn(char* _name)
{

    for (columnItr = columns.begin(); columnItr != columns.end(); ++columnItr) {
        if(strcasecmp(columnItr->first,_name) == 0)
            return (column*)columnItr->second;
    }
    return nullptr;
}

class sqlParser
{
    public:
    char*           sqlString;
    signed long     sqlStringLength;
    tokenParser*    tok;
    cTable*         table;
    list<cTable*>   tables;

    sqlParser(char*);
    ParseResult parse();
    ParseResult createTable();
    ParseResult createIndex();
    ParseResult parseColumns();
    ParseResult parsePrimaryKey();
    ParseResult parseColumnEdit(column*);
    ParseResult calculateTableColumnValues(cTable*);
    bool        isNumeric(char*);
    cTable*     getTableByName(char*);

};
/******************************************************
 * SQL Parser Constructor
 ******************************************************/
sqlParser::sqlParser(char* _sqlString)
{
    sqlString         = _sqlString;
    sqlStringLength   = strlen(sqlString);
    tok               = new tokenParser(sqlString);
}
/******************************************************
 * Parse
 ******************************************************/
ParseResult sqlParser::parse()
{

    char* token;
    pos = 0;
    while(pos < sqlStringLength)
    {
        token = tok->getToken();

        if(strlen(token) == 0)
            continue;

        if(strcasecmp(token,(char*)sqlTokenCreate) == 0)
        {
            continue;
        }

        if(strcasecmp(token,(char*)sqlTokenTable) == 0)
        {
            if(createTable() == ParseResult::FAILURE) 
                return ParseResult::FAILURE;
            continue;
        }

        if(strcasecmp(token,(char*)sqlTokenIndex) == 0)
        {
            if(createIndex() == ParseResult::FAILURE) 
                return ParseResult::FAILURE;
            continue;
        }
        errText.append("sql parser expecting a CREATE statement instead we got ");
        errText.append(token);
        return ParseResult::FAILURE;
    }
    
    return ParseResult::SUCCESS;
};
/******************************************************
 * Create Table
 ******************************************************/
ParseResult sqlParser::createTable()
{

    char* token;
    table = new cTable();

    //Get table name
    token           = tok->getToken();
    table->name     = token;

    //Get AS statement
    token           = tok->getToken();
    if(strcasecmp(token,(char*)sqlTokenAs) != 0)
    {
        errText.append("sql parser expecting  AS statement (table file location).");
        return ParseResult::FAILURE;
    }

    //Get table location
    token               = tok->getToken();
    table->fileName     = token;

    if(parseColumns() == ParseResult::FAILURE)
    {
        return ParseResult::FAILURE;
    }

    calculateTableColumnValues(table);

    tables.push_back(table);

    return ParseResult::SUCCESS;
    
}
/******************************************************
 * Parse Columns
 ******************************************************/
ParseResult sqlParser::parseColumns()
{
    //Get first token
    char* token = tok->getToken();
    column* col;

    if(strcasecmp(token,(char*)sqlTokenOpenParen) != 0)
    {
        errText.append("sql parser expecting '(' to open column list");
        return ParseResult::FAILURE;
    }
    
    while(pos < sqlStringLength)
    {
        token = tok->getToken();

        if(strcasecmp(token,(char*)sqlTokenPrimary) == 0)
        {
            if(parsePrimaryKey() == ParseResult::FAILURE)
                return ParseResult::FAILURE;

            token = tok->getToken();
        }

        //End of column list?
        if(strcasecmp(token,(char*)sqlTokenCloseParen) == 0)
            return ParseResult::SUCCESS;
        
        col = new column();

        //Get column name
        col->name = token;

        if(parseColumnEdit(col) == ParseResult::FAILURE)
            return ParseResult::FAILURE;    
        
        table->columns.insert({token, col});
    }
    return ParseResult::SUCCESS;
};
/******************************************************
 * Parse Primary Key
 ******************************************************/
ParseResult sqlParser::parsePrimaryKey()
{
    //Get first token
    char* token = tok->getToken();
    column* col;
    
    if(strcasecmp(token,(char*)sqlTokenKey) != 0)
    {
        errText.append("sql parser expecting 'Key', found ");
        errText.append(token);
        return ParseResult::FAILURE;
    }
                        
    token = tok->getToken();
    if(strcasecmp(token,(char*)sqlTokenOpenParen) != 0)
    {
        errText.append("sql parser expecting '(', found ");
        errText.append(token);
        return ParseResult::FAILURE;
    }

    while(pos < sqlStringLength)
    {
        token = tok->getToken();

        if(strcasecmp(token,(char*)sqlTokenCloseParen) == 0)
            return ParseResult::SUCCESS;

        if(strcasecmp(token,(char*)sqlTokenCreate) == 0)
        {
            errText.append("sql parser expecting ')', found ");
            errText.append(token);
            return ParseResult::FAILURE;
        }

        col = table->getColumn(token);
        if(col == nullptr)
        {
            errText.append("sql parser:primary column name not found. see ");
            errText.append(token);
            return ParseResult::FAILURE;
        }
        col->primary = true;
    }
    return ParseResult::SUCCESS;
}
/******************************************************
 * Parse Column Edit
 ******************************************************/
ParseResult sqlParser::parseColumnEdit(column* _col)
{
    char* token;
    token   = tok->getToken();

    if(strcasecmp(token,(char*)sqlTokenEditBool) == 0)
    {
        _col->edit = t_edit::t_bool;
        _col->length = 1;
        return ParseResult::SUCCESS;
    }

    if(strcasecmp(token,(char*)sqlTokenEditInt) == 0)
    {
        _col->edit = t_edit::t_int;
        _col->length = sizeof(int);
        return ParseResult::SUCCESS;
    }

    if(strcasecmp(token,(char*)sqlTokenEditDouble) == 0)
    {
        _col->edit = t_edit::t_double;
        _col->length = sizeof(double);
        return ParseResult::SUCCESS;
    }

    if(strcasecmp(token,(char*)sqlTokenEditDate) == 0)
    {
        _col->edit = t_edit::t_date;
        _col->length = sizeof(t_tm);
        return ParseResult::SUCCESS;
    }

    if(strcasecmp(token,(char*)sqlTokenEditChar) == 0)
    {
        _col->edit = t_edit::t_char;
    }


    // Get char length
    if(_col->edit == t_edit::t_char)
    {
        token = tok->getToken();
        if(strcmp(token,(char*)sqlTokenOpenParen) != 0)
        {
            errText.append("sql parser expecting '(' for column length. See ");
            errText.append(_col->name);
            errText.append("  ");
            return ParseResult::FAILURE;
        }

        token = tok->getToken();
        if(!isNumeric(token))
        {
            errText.append("sql parser expecting a number for column length. See ");
            errText.append(_col->name);
            errText.append("  ");
            errText.append(token);
            errText.append("  ");
            return ParseResult::FAILURE;
        }
        _col->length = atoi(token);

        token = tok->getToken();
        if(strcmp(token,(char*)sqlTokenCloseParen) != 0)
        {
            errText.append("sql parser expecting ')' for column length. See ");
            errText.append(_col->name);
            errText.append("  ");
            return ParseResult::FAILURE;
        }
    }
    return ParseResult::SUCCESS;
}
/******************************************************
 * Create Index
 ******************************************************/
ParseResult sqlParser::createIndex()
{
    //The first token must be the index name
    char* token = tok->getToken();

    cIndex* index = new cIndex();
    index->name = token;

    token = tok->getToken();
    
    //The next token must be the literal as
    if(strcasecmp(token,(char*)sqlTokenAs) != 0)
    {
        errText.append(" Expecting the token 'as' received ");
        errText.append(token);
        errText.append(" instead");
        return ParseResult::FAILURE;
    }
    //The next token must be file name
    token = tok->getToken();

    index->fileName = token;

     token = tok->getToken();

    //The next token must be the literal on
    if(strcasecmp(token,(char*)sqlTokenOn) != 0)
    {
        errText.append(" Expecting the token 'ON' received ");
        errText.append(token);
        errText.append(" instead");
        return ParseResult::FAILURE;
    }

    //This must be table name
    token = tok->getToken();

    table = getTableByName(token); 
    if(table == nullptr)
    {
        errText.append(" Index table ");
        errText.append(token);
        errText.append(" not found");
        return ParseResult::FAILURE;
    }

    table->indexes.push_back(index);

    token = tok->getToken();

    //The next token must be (
    if(strcasecmp(token,(char*)sqlTokenOpenParen) != 0)
    {
        errText.append(" Expecting '(' to open column list");
        return ParseResult::FAILURE;
    }
    column* col;
    while(pos < sqlStringLength)
    {
        token = tok->getToken();
        if(strcasecmp(token,(char*)sqlTokenCloseParen) == 0)
        {
            break;
        }
        col = table->getColumn(token);
        if(col == nullptr)
        {
            errText.append(" Index column ");
            errText.append(token);
            errText.append(" not found");
            return ParseResult::FAILURE;
        }
        index->columns.insert({token, col});

    }   

    return ParseResult::SUCCESS;
};
/******************************************************
 * Calculate Table Column Values
 ******************************************************/
ParseResult sqlParser::calculateTableColumnValues(cTable* _table)
{
    try
    {
        int     recordLength        = 0;
        int     position            = 0;
        column* c;
        for (_table->columnItr = _table->columns.begin(); _table->columnItr != _table->columns.end(); ++_table->columnItr) 
        {
            c               = (column*)_table->columnItr->second;
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
 * Is Numeric
 ******************************************************/
bool sqlParser::isNumeric(char* _token)
{

    if(_token == nullptr)
        return false;

    for(size_t i=0;i<strlen(_token);i++)
    {
        if(_token[i] == '.')
            continue;
        if(!isdigit(_token[i]))
            return false;
    }
    return true;
}
/******************************************************
 * Get Table By Name
 ******************************************************/
cTable* sqlParser::getTableByName(char* _tableName)
{
    for(cTable* tbl : tables)
    {
       // printf("\n looking for %s found %s",_tableName,tbl->name.c_str());
        if(strcasecmp(tbl->name.c_str(), _tableName) == 0)
            return tbl;
    }
    return nullptr;
}

