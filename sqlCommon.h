#pragma once
#include <string>
#include "universal.h"
#include "index.h"

using namespace std;

#define SPACE  			' '
#define CRLF            "\r\n"  
#define NEWLINE 		'\n'
#define TAB 			'\t'
#define VTAB 			'\v'
#define RETURN          '\r'
#define FORMFEED        '\f'
#define COLON 			':'
#define COMMA 			','
#define OPENPAREN       '('   //Note difference between OPENPAREN and sqlTokenOpenParen
#define CLOSEPAREN      ')'
#define EQUAL           '='

#define MAXSQLTOKENSIZE	    100
#define MAXSQLSTRINGSIZE	1000

#define gtr         ">"
#define cellBegin   "<td>"
#define cellEnd     "</td>"
#define rowBegin    "<tr>"
#define rowEnd      "</tr>"
#define hdrBegin    "<th"
#define hdrEnd      "</th>"


#define sqlTokenCreate      "CREATE"
#define sqlTokenTable       "TABLE"
#define sqlTokenIndex       "INDEX"
#define sqlTokenSelect      "SELECT"
#define sqlTokenInsert      "INSERT"
#define sqlTokenDelete      "DELETE"
#define sqlTokenUpdate      "UPDATE"
#define sqlTokenTop         "TOP"
#define sqlTokenAs          "AS"
#define sqlTokenAsterisk    "*"
#define sqlTokenOpenParen   "("     //Note difference between OPENPAREN and sqlTokenOpenParen
#define sqlTokenCloseParen   ")"
#define sqlTokenEqual       "="
#define sqlTokenNotEqual    "!="
#define sqlTokenGreater     ">"
#define sqlTokenLessThan    "<"
#define sqlTokenInto        "INTO"
#define sqlTokenFrom        "FROM"
#define sqlTokenWhere       "WHERE"
#define sqlTokenSet         "SET"
#define sqlTokenKey         "KEY"
#define sqlTokenPrimary     "PRIMARY"
#define sqlTokenLike        "LIKE"
#define sqlTokenAnd         "AND"
#define sqlTokenOr          "OR"
#define sqlTokenOn          "ON"
#define sqlTokenJoin        "JOIN"
#define sqlTokenValues      "VALUES"
#define sqlTokenEditBool    "BOOL"
#define sqlTokenEditChar    "CHAR"
#define sqlTokenEditDate    "DATE"
#define sqlTokenEditDouble  "DOUBLE"
#define sqlTokenEditInt     "INT"

enum ParseResult
{
    SUCCESS,
    FAILURE
};

enum class SQLACTION{
    NOACTION,
    INVALID,
    INSERT,
    CREATE,
    SELECT,
    UPDATE,
    DELETE
};

// C++ tm is 58 characters long - this is 16
struct t_tm
{
    int year;
    int month;
    int day;
    int yearMonthDay;
};

enum class t_edit
{
    t_bool,
    t_char,
    t_date,
    t_double,
    t_int
};

class ReturnResult{
  public:
  string resultTable;
  string message;
  string error;
};

class splitToken
{
    public:
    char* one;
    char* two;
};

ReturnResult returnResult;
string errText;
class column
{
    public:
    char*   name;
    bool    primary = false;
    t_edit  edit;
    int     length = 0;
    int     position = 0;
    char*   value;
    char*   alias;
};

class baseData
{
    public:
        fstream*            fileStream = nullptr;
        char*               name;
        char*               fileName;
        list<column*>       columns;
        fstream*            open();
        void                close();
        column*             getColumn(char*);
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
};
void baseData::close()
{
    if(fileStream != nullptr)
    {
        if(fileStream->is_open())
        {
            fileStream->close();
        }
    } 
};
column* baseData::getColumn(char* _name)
{

    for (column* col : columns) {
        if(strcasecmp(col->name,_name) == 0)
            return col;
    }
    return nullptr;
};
class sIndex : public baseData
{
    public:
    Index* index;
    bool openIndex();
};
bool sIndex::openIndex()
{
    if(strlen(name) == 0)
    {
        errText.append("Index name not set");
        return false;
    }
    if(strlen(fileName) == 0)
    {
        errText.append("Index filename not set");
        return false;
    }
    fileStream = open();
    index = new Index(fileStream);
    return true;
};

class sTable : public baseData
{  
    public:
        list<sIndex*>   indexes;
        int             recordLength = 0;
        char*           alias;
};

signed long pos   = 0; //pointer to position in string being parsed
string debugText;

