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
#include "utilities.h"
#include "lookup.h"

using namespace std;


class sqlParser
{
    public:
    char*           sqlString;
    signed long     sqlStringLength;
    tokenParser*    tok;
    sTable*         table;
    list<sTable*>   tables;

    sqlParser(char*);
    ParseResult parse();
    ParseResult createTable();
    ParseResult createIndex();
    ParseResult parseColumns(char*);
    ParseResult parsePrimaryKey();
    ParseResult parseColumnEdit(Column*);
    ParseResult calculateTableColumnValues(sTable*);

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
    while(!tok->eof)
    {
        token = tok->getToken();

        if(token == nullptr)
            break;

        if(tok->eof)
            break;

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
    table = new sTable();

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

    if(parseColumns(table->name) == ParseResult::FAILURE)
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
ParseResult sqlParser::parseColumns(char* _tableName)
{
    //Get first token
    char* token = tok->getToken();
    Column* col;

    if(strcasecmp(token,(char*)sqlTokenOpenParen) != 0)
    {
        errText.append("sql parser expecting '(' to open column list");
        return ParseResult::FAILURE;
    }
    
    while(!tok->eof)
    {
        token = tok->getToken();

        if(token == nullptr)
            break;

        if(strcasecmp(token,(char*)sqlTokenPrimary) == 0)
        {
            if(parsePrimaryKey() == ParseResult::FAILURE)
                return ParseResult::FAILURE;

            token = tok->getToken();
        }

        //End of column list?
        if(strcasecmp(token,(char*)sqlTokenCloseParen) == 0)
            return ParseResult::SUCCESS;
        
        col = new Column();

        //Get column name
        col->name       = token;
        col->tableName = _tableName;

        if(parseColumnEdit(col) == ParseResult::FAILURE)
            return ParseResult::FAILURE;    
        
        table->columns.push_back(col);
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
    Column* col;
    
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

    while(!tok->eof)
    {
        token = tok->getToken();

        if(token == nullptr)
            break;

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
ParseResult sqlParser::parseColumnEdit(Column* _col)
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
        if(!utilities::isNumeric(token))
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

    sIndex* index = new sIndex();
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

    table = lookup::getTableByName(tables,token); 
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
    Column* col;
    while(!tok->eof)
    {
        token = tok->getToken();

        if(token == nullptr)
            continue;

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
        index->columns.push_back(col);

    }   

    return ParseResult::SUCCESS;
};
/******************************************************
 * Calculate Table Column Values
 ******************************************************/
ParseResult sqlParser::calculateTableColumnValues(sTable* _table)
{
    try
    {
        int     recordLength        = 0;
        int     position            = 0;
        for (Column* col : _table->columns)
        {
            col->position   = position;
            position        = position + col->length;
            recordLength    = recordLength + col->length;
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



