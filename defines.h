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

enum MESSAGETYPE
{
    ERROR,
    INFORMATION
};

enum PRESENTATION
{
    HTML,
    TEXT
};
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


string errText;
string msgText;
PRESENTATION presentationType;
