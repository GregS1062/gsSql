#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <syslog.h>
#include "global.h"
#include "userException.h"
#include "pipes.h"
#include "keyValue.h"
#include "parseJason.h"
#include <cgicc/CgiDefs.h> 
#include <cgicc/Cgicc.h> 
#include <cgicc/HTTPHTMLHeader.h> 
#include <cgicc/HTMLClasses.h>  

using namespace std;
using namespace cgicc;

string formatNotFound(string _target)
{
	string errText;
	errText.append("<p>");
	errText.append(_target);
	errText.append(" not found");
	return errText;
}
string parseSql(string _database, string _table, string _column)
{
	string htmResponse;
	valueList* v;
    keyValue* result;
    keyValue* kv = parseDatabaseDefinition();
    result = getDatabaseEntity(kv, lit_database, _database.c_str());
	if(result == nullptr)
		return formatNotFound(_database);

    result = getDatabaseEntity(result, lit_table, _table.c_str());
	if(result == nullptr)
		return formatNotFound(_table);

    result = getDatabaseEntity(result, lit_column, _column.c_str());
	if(result == nullptr)
		return formatNotFound(_column);

    v = (valueList*)result->value;
    if(v->t_type == t_Object)
    {
        keyValue* nkv = (keyValue*)v->value;
        htmResponse.append("key="); 
		htmResponse.append(nkv->key);
        v = (valueList*)nkv->value;
        if(v->t_type == t_string)
		{
			htmResponse.append("length:");
			htmResponse.append((char*)v->value);
		}
    }

    return htmResponse;
}
/*---------------------------------------
   Main
-----------------------------------------*/
int main()
{
    Cgicc formData;

	string htmlRequest;

	string htmlResponse;

	ofstream traceFile;

	try{

		htmlRequest.append(serializeCGIFormData(formData));
		
		if(debug)
		{
			traceFile.open ("CGITrace.txt",std::ios_base::app);
			traceFile << "\n-----------------------\n";
			traceFile << htmlRequest;
			traceFile.close();
		}

		htmlResponse.append("Content-type:text/html\r\n\r\n");
		htmlResponse.append("<!DOCTYPE HTML//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">");
		htmlResponse.append("\n");
		htmlResponse.append("<html>");
		htmlResponse.append("\n<link rel=\"stylesheet\" href=\"gsSql.css\">");
		htmlResponse.append("\n<body>");
		htmlResponse.append("\n<FORM METHOD=POST ACTION=\"http://localhost/gsSql.cgi#\">");
		htmlResponse.append("\n<table>");
		htmlResponse.append("\n\t<tr>");
		htmlResponse.append("\n\t<td width=\"30%\"></td>");
		htmlResponse.append("\n\t<td width=\"30%\" style=\"text-align:center; font-size:34px;\">Enter Query</td>");
		htmlResponse.append("\n\t<td align=\"right\">");
		htmlResponse.append("\n\t\t\t<button class=\"btnSubmit\" style=\"height:50px; width:150px; font-size:24px;\" >Submit</button>");
		htmlResponse.append("\n\t\t\t</td>");
		htmlResponse.append("\n\t</tr>");
		htmlResponse.append("\n\t<tr>");
		htmlResponse.append("\n\t\t<td colspan=\"3\">");
		htmlResponse.append("\n\t\t<textarea name=\"query\" rows=\"5\" cols=\"80\" wrap=\"soft\" maxlength=\"60\"");
		htmlResponse.append(" style=\"overflow:hidden; resize:none; font-size:24px;\"> ");
		htmlResponse.append(htmlRequest);
		htmlResponse.append("</textarea>");
		htmlResponse.append("\n\t\t</td>");
		htmlResponse.append("\n\t</tr>");
		htmlResponse.append("\n\t<tr>");
		htmlResponse.append("\n\t\t<td height=\"100px\" colspan=\"3\" style=\"font-size:24px; background-color: white; vertical-align:top;\">");
		htmlResponse.append("\n\t\t<div name=\"errmsg\" contenteditable=\"true\" style=\"font-size:24px; background-color: white;\">");
		htmlResponse.append("\n\t\t<span style=\"color: blue;\">");
		htmlResponse.append(htmlRequest);
		htmlResponse.append("</span>");
		htmlResponse.append("\n\t\t<span style=\"color: red;\">error text</span>");
		htmlResponse.append("\n\t\t</div>");
		htmlResponse.append("\n\t\t</td>");
		htmlResponse.append("\n\t</tr>");
		htmlResponse.append("\n</table>");
		htmlResponse.append("\n</FORM>");
		htmlResponse.append("\n</body>");
		htmlResponse.append("\n</html>");
		cout << htmlResponse;
		return 0;
	}
	catch_and_trace
}