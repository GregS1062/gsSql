#pragma once
#include <string>
#include <fstream>

using namespace std;

#define NEGATIVE		-1
#define QUOTE			  '"'
#define SPACE  			' '
#define NEWLINE 		'\n'
#define TAB 			  '\t'
#define COLON 			':'
#define COMMA 			','
#define OPENPAREN   '('   //Note difference between OPENPAREN and sqlTokenOpenParen
#define CLOSEPAREN  ')'

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

ReturnResult returnResult;

bool debug = false;
string debugText;
string errText;
