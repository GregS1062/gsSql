
#include <fstream>
#include <iostream>
#include <string>
#include <cstring>
#include <list>
#include <algorithm>
#include "defines.h"
#include "sqlCommon.h"
#include "utilities.cpp"
#include "lookup.cpp"
#include "prepareQuery.cpp"
using namespace std;

class newSql
{
    protected:
        string          sqlString;
        list<string> split(string,string);

        ParseResult parseTable(string);
        ParseResult parseIndex(string);
        ParseResult parseColumn(string);
        t_edit      parseColumnEdit(char*);

    public:
        newSql(string);
        ParseResult parse();

};
newSql::newSql(string _sqlString)
{
    sqlString = _sqlString;
}

/******************************************************
 * Parse 
 ******************************************************/
ParseResult newSql::parse()
{
    // split SQL text by create strings
    list<string> createStrings = split(sqlString,sqlTokenCreate);
    list<string> tableStrings;
    list<string> indexStrings;

    // separate table from index strings
    for(string str : createStrings)
    {
        string upperStr = str;
        transform(upperStr.begin(), upperStr.end(), upperStr.begin(), ::toupper);

        //find table string
        int pos = upperStr.find(sqlTokenTable);
        if(pos > 0)  
        { 
            //trim the keyword TABLE off string
            int newPos = pos + 1 + strlen(sqlTokenTable);
            tableStrings.push_back(str.substr(newPos,str.length() - newPos));
            continue;
        }

        // find index string
        pos = upperStr.find(sqlTokenIndex);
        if(pos > 0)  
        {
            //trim the word INDEX off string
            int newPos = pos + 1 + strlen(sqlTokenIndex);
            indexStrings.push_back(str.substr(newPos,str.length() - newPos));
        }
    }

    // sanitizeQuery remove white noise and sets rules on use of spaces
    for(string str : tableStrings)
    {
        if(parseTable((char*)sanitizeQuery((char*)str.c_str(),MAXINPUTSIZE).get()) == ParseResult::FAILURE)
            return ParseResult::FAILURE;
    }
    
    for(string str : indexStrings)
        parseIndex((char*)sanitizeQuery((char*)str.c_str(),MAXINPUTSIZE).get());

    return ParseResult::SUCCESS;
}

/******************************************************
 * Parse Table
 ******************************************************/
ParseResult newSql::parseTable(string _tableString)
{
    //Left trim table string
    _tableString.erase(0, _tableString.find_first_not_of(' '));
    
    //find first space
    int pos = _tableString.find(' ');
    if(pos < 1)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Expecting table name in ",_tableString.c_str());
        return ParseResult::FAILURE;
    }

    shared_ptr<sTable> tbl = make_shared<sTable>();

    //Table name
    tbl->name = (char*)_tableString.substr(0,pos).c_str();

    //Table file Name
    int posFileName = lookup::findDelimiter(_tableString,sqlTokenAs);
    if(posFileName < 1)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Expecting file name",_tableString.c_str());
        return ParseResult::FAILURE;
    }

    int posOpenParen = _tableString.find(OPENPAREN);

    posFileName = posFileName+strlen(sqlTokenAs);
    string fileName = _tableString.substr(posFileName,posOpenParen-posFileName);
    tbl->fileName = (char*)fileName.c_str();

    printf("\n\nTable Name:%s",tbl->name);
    printf("\nFile Name:%s",tbl->fileName);
    
    //break into column strings
    string columnText = _tableString.substr(posOpenParen+1, _tableString.length() - (posOpenParen+1));

    char * token;
    char *str=const_cast< char *>(columnText.c_str()); 
    token = strtok (str,",");
    while (token != NULL)
    {
        lTrim(token,' ');
        if(parseColumn(string(token)) == ParseResult::FAILURE)
            return ParseResult::FAILURE;
        token = strtok (NULL, ",");
    }
    return ParseResult::SUCCESS;
}

/******************************************************
 * Parse Index
 ******************************************************/
ParseResult newSql::parseIndex(string _indexString)
{
    //printf("\n%s",_indexString.c_str());
    return ParseResult::SUCCESS;
}
/******************************************************
 * Parse Index
 ******************************************************/
ParseResult newSql::parseColumn(string _columnString)
{
    /*
        Requirements and assumptions

        -SanitizeQuery is run in parse(), therefore
            - all white space tabs, formfeeds, newlines, etc are removed
            - all instances of more than one consecutive spaces are reduced to one
            - a single space is inserted between key elements: names, edits and open and close parenthesis
    */
    string columnString =_columnString;

    shared_ptr<Column> col = make_shared<Column>();

    //left trim
    columnString.erase(0, columnString.find_first_not_of(' '));
    int pos = columnString.find(' ');
    if(pos == NEGATIVE)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Expecting column name ",columnString.c_str());
        return ParseResult::FAILURE;
    }
    col->name = (char*)columnString.substr(0,pos).c_str();

    columnString = columnString.substr(pos+1,columnString.length() - (pos-1));
    pos = columnString.find(' ');

    if(pos == NEGATIVE
    && lookup::findDelimiter((char*)columnString.c_str(),(char*)sqlTokenEditBool) == NEGATIVE)
    {
         sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Expecting column edit type ",columnString.c_str());
         return ParseResult::FAILURE;
    }
    col->edit = parseColumnEdit((char*)columnString.substr(0,pos).c_str());

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
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Cannot determine edit",columnString.c_str());
            return ParseResult::FAILURE;
    }
    
    // Get char length
    if(col->edit == t_edit::t_char)
    {
        columnString = columnString.substr(pos+1,columnString.length() - (pos-1));
        pos = columnString.find(OPENPAREN);
        if(pos == NEGATIVE)
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Expecting opening parenhesis for column length ",col->name);
            return ParseResult::FAILURE;
        }
        columnString = columnString.substr(pos+1,columnString.length() - (pos-1));
        pos = columnString.find(CLOSEPAREN);
        if(pos == NEGATIVE)
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Expecting closing parenhesis for column length ",col->name);
            return ParseResult::FAILURE;
        }
        columnString = columnString.substr(1,pos-2);
        if(!isNumeric((char*)columnString.c_str()))
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Expecting column length |",columnString);
            sendMessage(MESSAGETYPE::ERROR,presentationType,false,"|");
            return ParseResult::FAILURE;
        }
        col->length = atoi(columnString.c_str());
    }

    printf("\n\tColumn Name %s",col->name);
    printf("\t edit %i",col->edit);
    printf("\t length %i",col->length);
    return ParseResult::SUCCESS;
}
/******************************************************
 * Parse Column Edit
 ******************************************************/
t_edit newSql::parseColumnEdit(char* _edit)
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


/******************************************************
 * Split
 ******************************************************/
list<string> newSql::split(string _inString, string _delimiter) 
{
    //** REQUIRED Delimiter must be uppercase */
    
    //Copy input to working string
    string strUpper = _inString;

    //transform working string to uppercase
    transform(strUpper.begin(), strUpper.end(), strUpper.begin(), ::toupper);
    
    //find first delimiter
    int pos = strUpper.find(_delimiter);
    int prior = pos;

    list<string> strings;
    //Find first occurance
    if (pos == -1)
        return strings;

    while (pos != -1) {
        //save prior position
        prior = pos;
        //get next position
        pos = strUpper.find(_delimiter, pos + 1);
        //NOTE: it is the parameter _workstring that is being split here, not the uppercase string
        strings.push_back(_inString.substr(prior,pos-prior));
    }
    return strings;
}

/******************************************************
 * Main
 ******************************************************/
int main(int argc, char* argv[])
{
    presentationType = PRESENTATION::TEXT;
    std::string sqlFileName = "bike.sql";
    std::ifstream ifs(sqlFileName);
    std::string sqlFile ( (istreambuf_iterator<char>(ifs) ),
                       (istreambuf_iterator<char>()    ) );

    newSql sql(sqlFile);
    sql.parse();
    printf("\n\nERROR: %s\n",errText.c_str());
}