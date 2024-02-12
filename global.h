#pragma once
#include <string>
#include <fstream>

using namespace std;

#define NEGATIVE		-1
#define QUOTE			'"'
#define SPACE  			' '
#define NEWLINE 		'\n'
#define TAB 			'\t'
#define COLON 			':'
#define COMMA 			','
#define MAXSQLTOKENSIZE	50

#define errorColor "#ef7f7d"
#define infoColor "#f9f1b2"

enum class DISPLAY
	{ ERROR, INFO };

enum ParseResult
{
    SUCCESS,
    FAILURE
};

bool debug = false;
string errText;
