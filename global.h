#pragma once
#include <string>
#include <fstream>

using namespace std;

#define NEGATIVE		-1
#define QUOTE			  '"'
#define SPACE  			' '
#define CRLF        "\r\n"  
#define NEWLINE 		'\n'
#define TAB 			  '\t'
#define VTAB 			  '\v'
#define RETURN      '\r'
#define FORMFEED    '\f'
#define COLON 			':'
#define COMMA 			','
#define OPENPAREN   '('   //Note difference between OPENPAREN and sqlTokenOpenParen
#define CLOSEPAREN  ')'
#define EQUAL       '='

#define MAXSQLTOKENSIZE	50

#define errorColor  "#ef7f7d"
#define infoColor   "#f9f1b2"

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
#define sqlTokenInto        "INTO"
#define sqlTokenFrom        "FROM"
#define sqlTokenWhere       "WHERE"
#define sqlTokenSet         "SET"
#define sqlTokenAnd         "AND"
#define sqlTokenOr          "OR"
#define sqlTokenValues      "VALUES"

#define sqlTokenEditBool    "BOOL"
#define sqlTokenEditChar    "CHAR"
#define sqlTokenEditDate    "DATE"
#define sqlTokenEditDouble  "DOUBLE"
#define sqlTokenEditInt     "INT"

enum class DISPLAY
	{ ERROR, INFO };

enum ParseResult
{
    SUCCESS,
    FAILURE
};

class ReturnResult{
  public:
  string resultTable;
  string message;
  string error;
};

// C++ tm is 58 characters long - this is 12
struct t_tm
{
    int year;
    int month;
    int day;
};

ReturnResult returnResult;
signed long pos   = 0; //pointer to position in string being parsed
bool debug        = false;
string debugText;
string errText;
