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
#define EXCLAMATION     '!'
#define GTR             '>'
#define LST             '<'

#define DELIMITERERR        -2
#define MAXSQLTOKENSIZE	    100
#define MAXSQLSTRINGSIZE	1000

#define cellBegin   "<td>"
#define cellEnd     "</td>"
#define rowBegin    "<tr>"
#define rowEnd      "</tr>"
#define hdrBegin    "<th"
#define hdrEnd      "</th>"


#define sqlTokenCreate          "CREATE"
#define sqlTokenTable           "TABLE"
#define sqlTokenIndex           "INDEX"
#define sqlTokenSelect          "SELECT"
#define sqlTokenInsert          "INSERT"
#define sqlTokenDelete          "DELETE"
#define sqlTokenUpdate          "UPDATE"
#define sqlTokenTop             "TOP"
#define sqlTokenAs              "AS"
#define sqlTokenInto            "INTO"
#define sqlTokenFrom            "FROM"
#define sqlTokenWhere           "WHERE"
#define sqlTokenSet             "SET"
#define sqlTokenKey             "KEY"
#define sqlTokenPrimary         "PRIMARY"
#define sqlTokenLike            "LIKE"
#define sqlTokenValues          "VALUES"
#define sqlTokenEditBool        "BOOL"
#define sqlTokenEditChar        "CHAR"
#define sqlTokenEditDate        "DATE"
#define sqlTokenEditDouble      "DOUBLE"
#define sqlTokenEditInt         "INT"
#define sqlTokenAsterisk        "*"
#define sqlTokenQuote           "\""
#define sqlTokenPeriod          "."
#define sqlTokenOpenParen       "("     //Note difference between OPENPAREN and sqlTokenOpenParen
#define sqlTokenCloseParen      ")"
#define sqlTokenEqual           "="
#define sqlTokenNotEqual        "<>"
#define sqlTokenGreater         ">"
#define sqlTokenLessThan        "<"
#define sqlTokenLessOrEqual     "<="
#define sqlTokenGreaterOrEqual  ">="
#define sqlTokenAll             "ALL"
#define sqlTokenAny             "ANY"
#define sqlTokenBetween         "BETWEEN"
#define sqlTokenExits           "EXISTS"
#define sqlTokenSome            "SOME"
#define sqlTokenUnique          "UNIQUE"
#define sqlTokenIN              "IN"
#define sqlTokenOrderBy         "ORDER BY"
#define sqlTokenGroupBy         "GROUP BY"
#define sqlTokenHaving          "HAVING"
#define sqlTokenBY              "BY"
#define sqlTokenAnd             "AND"
#define sqlTokenOr              "OR"
#define sqlTokenOn              "ON"
#define sqlTokenJoin            "JOIN"
#define sqlTokenInner           "INNER"
#define sqlTokenLeft            "LEFT"
#define sqlTokenRight           "RIGHT"
#define sqlTokenOuter           "OUTER"
#define sqlTokenNatural         "NATURAL"
#define sqlTokenCross           "CROSS"
#define sqlTokenFull            "FULL"
#define sqlTokenInnerJoin       "INNER JOIN"
#define sqlTokenLeftJoin        "LEFT JOIN"
#define sqlTokenRightJoin       "RIGHT JOIN"
#define sqlTokenOuterJoin       "OUTER JOIN"
#define sqlTokenOrderDescending "DESC"
#define sqlTokenOrderAcending   "ASC"
#define sqlTokenTrue            "T"
#define sqlTokenCount           "COUNT"
#define sqlTokenSum             "SUM"
#define sqlTokenMax             "MAX"
#define sqlTokenMin             "MIN"
#define sqlTokenAvg             "AVG"

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
    FAILURE,
    CONTINUE 
};

enum class SQLACTION{
    NOACTION,
    INVALID,
    INSERT,
    CREATE,
    SELECT,
    UPDATE,
    DELETE,
    JOIN,
    INNER,      //Join
    OUTER,      //Join
    LEFT,       //Join
    RIGHT,      //Join
    NATURAL,    //Join
    CROSS,      //Join
    FULL        //Join
};

enum class RETURNACTION
{
    ORDER,
    GROUP,
    JOIN,
    SEND
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
enum class t_aggregate
{
    NONE,
    COUNT,
    MAX,
    MIN,
    AVG,
    SUM
};


string errText;
string msgText;
PRESENTATION presentationType;
