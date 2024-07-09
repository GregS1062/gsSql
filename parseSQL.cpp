
#include <fstream>
#include <iostream>
#include <string>
#include <cstring>
#include <list>
#include <algorithm>
#include "defines.h"
#include "sqlCommon.h"
#include "prepareQuery.cpp"
#include "interfaces.h"
using namespace std;

class parseSql
{
    protected:
        string          sqlString;
        list<string> split(string,string);

        ParseResult parseTable(string);
        ParseResult parseIndex(string);
        ParseResult parseColumn(shared_ptr<sTable>,string);
        t_edit      parseColumnEdit(char*);
        ParseResult calculateTableColumnValues(shared_ptr<sTable> _table);

    public:
        shared_ptr<iSQLTables> isqlTables = make_shared<iSQLTables>();
        parseSql(string);
        ParseResult parse();

};
parseSql::parseSql(string _sqlString)
{
    sqlString = _sqlString;
}

/******************************************************
 * Parse 
 ******************************************************/
ParseResult parseSql::parse()
{
    // split SQL text by create strings
    list<string> createStrings = split(sqlString,sqlTokenCreate);
    list<string> tableStrings;
    list<string> indexStrings;

    try{
        // separate table from index strings
        for(string str : createStrings)
        {
           /// string upperStr = str;
           // transform(upperStr.begin(), upperStr.end(), upperStr.begin(), ::toupper);

            //find table string
            size_t pos = str.find(sqlTokenTable);
            if(pos != std::string::npos)  
            { 
                //trim the keyword TABLE off string
                pos = pos + 1 + strlen(sqlTokenTable);
                tableStrings.push_back(str.substr(pos,str.length() - pos));
                continue;
            }

            // find index string
            pos = str.find(sqlTokenIndex);
            if(pos > 0)  
            {
                //trim the word INDEX off string
                size_t newPos = pos + 1 + strlen(sqlTokenIndex);
                indexStrings.push_back(str.substr(newPos,str.length() - newPos));
            }
        }

        string normalized;
        // normalizeQuery remove white noise and sets rules on use of spaces
        for(string str : tableStrings)
        {
            normalized = normalizeQuery(str,MAXINPUTSIZE);
            if(normalized.empty())
                return ParseResult::FAILURE;
            if(parseTable(normalized) == ParseResult::FAILURE)
                return ParseResult::FAILURE;
        }
        
        for(string str : indexStrings)
        {
            normalized = normalizeQuery(str,MAXINPUTSIZE);
            if(normalized.empty())
                return ParseResult::FAILURE;
            if(parseIndex(normalized) == ParseResult::FAILURE)
                return ParseResult::FAILURE;
        }

        return ParseResult::SUCCESS;
    }
    catch(const std::exception& e)
    {
         sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Exception error",e.what());
        return ParseResult::FAILURE;
    }
}

/******************************************************
 * Parse Table
 ******************************************************/
ParseResult parseSql::parseTable(string _tableString)
{
    try{
        //Left trim table string
        _tableString.erase(0, _tableString.find_first_not_of(SPACE));
        
        //find first space
        size_t pos = _tableString.find(SPACE);
        if(pos == std::string::npos)
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Expecting table name in ",_tableString.c_str());
            return ParseResult::FAILURE;
        }

        shared_ptr<sTable> tbl = make_shared<sTable>();

        //Table name
        tbl->name = (char*)_tableString.substr(0,pos).c_str();

        //Table file Name
        size_t posFileName = _tableString.find(sqlTokenAs);
        if(posFileName == std::string::npos)
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Expecting file name",_tableString.c_str());
            return ParseResult::FAILURE;
        }

        size_t posOpenParen = _tableString.find(OPENPAREN);

        posFileName = posFileName+strlen(sqlTokenAs);
        string fileName = _tableString.substr(posFileName,posOpenParen-posFileName);
        fileName = trim(fileName);
        fileName.erase(0, fileName.find_first_not_of(QUOTE));
        fileName.erase(fileName.find_last_not_of(QUOTE)+1);
        tbl->fileName = (char*)fileName.c_str();
        
        //break into column strings
        string columnText = _tableString.substr(posOpenParen+1, _tableString.length() - (posOpenParen+1));

        char * token;
        char *str=const_cast< char *>(columnText.c_str()); 
        token = strtok (str,",");
        while (token != NULL)
        {
            lTrim(token,SPACE);
            if(parseColumn(tbl,string(token)) == ParseResult::FAILURE)
                return ParseResult::FAILURE;
            token = strtok (NULL, ",");
        }
        calculateTableColumnValues(tbl);
        isqlTables->tables.push_back(tbl);

        return ParseResult::SUCCESS;
    }
    catch(const std::exception& e)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Exception error",e.what());
        return ParseResult::FAILURE;
    }
}

/******************************************************
 * Parse Index
 ******************************************************/
ParseResult parseSql::parseIndex(string _indexString)
{
    try
    {
        string indexString = _indexString;
        string indexUpper = prepareStringTemplate(_indexString);

        string target;

        //Working position in string
        size_t pos = 0;

        //find location
        size_t findLocation = 0;

        //Left trim table string
        indexString.erase(0, indexString.find_first_not_of(SPACE));
        
        target.append(" ").append(sqlTokenAs).append(" ");
        findLocation = indexUpper.find(target);
        if(findLocation < 1)
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Expecting index name in ",indexString);
            return ParseResult::FAILURE;
        }

        shared_ptr<sIndex> idx = make_shared<sIndex>();

        //Index name
        idx->name = indexString.substr(0,findLocation);
        
        //Position at the beginning of file name
        pos = findLocation+target.length();
        target.clear();

        //Find end of file name
        target.append(" ").append(sqlTokenOn).append(" ");
        findLocation = indexUpper.find(target);
        if(findLocation < 1)
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Expecting index file location in ",indexString);
            return ParseResult::FAILURE;
        }
        idx->fileName = indexString.substr(pos,findLocation-pos);
        idx->fileName = trim(idx->fileName);
        idx->fileName.erase(0, idx->fileName.find_first_not_of(QUOTE));
        idx->fileName.erase(idx->fileName.find_last_not_of(QUOTE)+1);

        //Position at the beginning of table name
        pos = findLocation+target.length();

        findLocation = indexUpper.find(OPENPAREN);
        if(findLocation < 1)
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Expecting table name ",indexString);
            return ParseResult::FAILURE;
        }
        string tableName = indexString.substr(pos,findLocation-pos-1);

        //Get table to attach index to
        shared_ptr<sTable> tbl = getTableByName(isqlTables->tables,tableName);
        if(tbl == nullptr)
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Could not find table ",tableName);
            return ParseResult::FAILURE;
        }

        //Position to find column
        pos = findLocation+1;
        
        findLocation = indexUpper.find(CLOSEPAREN);
        if(findLocation < 1)
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Expecting column name ",indexString);
            return ParseResult::FAILURE;
        }
        string columnName = indexString.substr(pos+1,findLocation-pos-2);

        shared_ptr<Column> col = tbl->getColumn(columnName);
        if(col == nullptr)
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Could not find column ",columnName);
            return ParseResult::FAILURE;
        }
        idx->columns.push_back(col);
        tbl->indexes.push_back(idx);
        return ParseResult::SUCCESS;
    }
    catch(const std::exception& e)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Exception error",e.what());
        return ParseResult::FAILURE;
    }
}
/******************************************************
 * Parse Column
 ******************************************************/
ParseResult parseSql::parseColumn(shared_ptr<sTable> _tbl,string _columnString)
{
    /*
        Requirements and assumptions

        -normalizeQuery is run in parse(), therefore
            - all white space tabs, formfeeds, newlines, etc are removed
            - all instances of more than one consecutive spaces are reduced to one
            - a single space is inserted between key elements: names, edits and open and close parenthesis
    */
    try
    {
        string columnString = _columnString;
        string templateString = prepareStringTemplate(_columnString);
        auto col = make_shared<Column>();

        size_t pos = templateString.find(sqlTokenPrimary);
        if(pos != std::string::npos)
        {
            pos = columnString.find(OPENPAREN);
            if(pos == std::string::npos)
            {
                sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Expecting ( on primary key declaration. ",columnString.c_str());
                return ParseResult::FAILURE;
            }
            string columnPrimary = columnString.substr(pos+1,columnString.length() - (pos-1));
            
            pos = columnPrimary.find(CLOSEPAREN);
            if(pos == std::string::npos)
            {
                sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Expecting ( on primary key declaration. ",columnPrimary.c_str());
                return ParseResult::FAILURE;
            }
            columnPrimary = columnPrimary.substr(1,pos-2);
            col = _tbl->getColumn(columnPrimary);
            if(col == nullptr)
            {
                sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Cannot find primary column in table column list:",columnPrimary.c_str());
                return ParseResult::FAILURE;
            }
            col->primary = true;
            return ParseResult::SUCCESS;
        }

        //left trim
        columnString.erase(0, columnString.find_first_not_of(SPACE));
        pos = columnString.find(SPACE);

        if(pos == std::string::npos)
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Expecting column name ",columnString.c_str());
            return ParseResult::FAILURE;
        }
        col->name = columnString.substr(0,pos);

        string columnType = columnString.substr(pos+1,columnString.length() - (pos-2));
        
        pos = columnType.find(SPACE);

        if(pos != std::string::npos)
            columnType = columnType.substr(0,pos);
        
        col->edit = parseColumnEdit((char*)columnType.c_str());
        
        switch(col->edit)
        {
            case t_edit::t_bool:
                col->length = 1;
                break;
            case t_edit::t_int:
                col->length = sizeof(int);
                break;
            case t_edit::t_double:
                col->length = sizeof(double);
                break;
            case t_edit::t_date:
                col->length = sizeof(t_tm);
                break;
            case t_edit::t_char:
                break;
            case t_edit::t_undefined:
                sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Cannot determine edit ",_columnString.c_str());
                return ParseResult::FAILURE;
        }
        
        // Get char length
        if(col->edit == t_edit::t_char)
        {
            string columnLength = columnString.substr(pos+1,columnString.length() - (pos-1));
            pos = columnLength.find(OPENPAREN);
            if(pos == std::string::npos)
            {
                sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Expecting opening parenhesis for column length ",col->name);
                return ParseResult::FAILURE;
            }
            columnLength = columnLength.substr(pos+1,columnLength.length() - (pos-1));
            pos = columnLength.find(CLOSEPAREN);
            if(pos == std::string::npos)
            {
                sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Expecting closing parenhesis for column length ",col->name);
                return ParseResult::FAILURE;
            }
            columnLength = columnLength.substr(1,pos-2);
            if(!isNumeric((char*)columnLength.c_str()))
            {
                sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Expecting column length |",columnLength);
                sendMessage(MESSAGETYPE::ERROR,presentationType,false,"|");
                return ParseResult::FAILURE;
            }
            col->length = atoi(columnLength.c_str());
        }
        col->tableName = _tbl->name;

        _tbl->columns.push_back(col);

        return ParseResult::SUCCESS;
    }
    catch(const std::exception& e)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Exception error",e.what());
        return ParseResult::FAILURE;
    }
}
/******************************************************
 * Parse Column Edit
 ******************************************************/
t_edit parseSql::parseColumnEdit(char* _edit)
{
    try
    {
        
        if(strcasecmp(_edit,(char*)sqlTokenEditBool) == 0)
            return t_edit::t_bool;

        if(strcasecmp(_edit,(char*)sqlTokenEditInt) == 0)
            return t_edit::t_int;

        if(strcasecmp(_edit,(char*)sqlTokenEditDouble) == 0)
            return t_edit::t_double;

        if(strcasecmp(_edit,(char*)sqlTokenEditDate) == 0)
            return t_edit::t_date;

        if(strcasecmp(_edit,(char*)sqlTokenEditChar) == 0)
            return t_edit::t_char;

        return t_edit::t_undefined;
    }
    catch(const std::exception& e)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Exception error",e.what());
        return t_edit::t_undefined;
    }
}


/******************************************************
 * Split
 ******************************************************/
list<string> parseSql::split(string _inString, string _delimiter) 
{
    //** REQUIRED Delimiter must be uppercase */
    list<string> strings;
    try
    {
    
        //Copy input to working string
        string strUpper = _inString;

        //transform working string to uppercase
        transform(strUpper.begin(), strUpper.end(), strUpper.begin(), ::toupper);
        
        //find first delimiter
        size_t pos = strUpper.find(_delimiter);
        size_t prior = pos;
        //Find first occurance
        if (pos == std::string::npos)
            return strings;

        while (pos != std::string::npos) {
            //save prior position
            prior = pos;
            //get next position
            pos = strUpper.find(_delimiter, pos + 1);
            //NOTE: it is the parameter _workstring that is being split here, not the uppercase string
            strings.push_back(_inString.substr(prior,pos-prior));
        }
        return strings;
    }
    catch(const std::exception& e)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Exception error",e.what());
        return strings;
    }
}
/******************************************************
 * Calculate Table Column Values
 ******************************************************/
ParseResult parseSql::calculateTableColumnValues(shared_ptr<sTable> _table)
{
    try
    {
        size_t  recordLength        = 0;
        int     position            = 0;
        for (shared_ptr<Column> col : _table->columns)
        {
            col->position   = position;
            position        = position + (int)col->length;
            recordLength    = recordLength + (int)col->length;
        }
        _table->recordLength = (int)recordLength;
        return ParseResult::SUCCESS;
    }
    catch(const std::exception& e)
    {
        errText.append( e.what());
        return ParseResult::FAILURE;
    }
}
