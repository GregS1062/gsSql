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
string parseSQL(string _database, string _table, string _column)
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

		cout << "Content-type:text/html\r\n\r\n";
		cout << HTMLDoctype(HTMLDoctype::eStrict) << endl;
		htmlResponse.append("<html><body>");
		htmlResponse.append("<table>");
		//htmlResponse.append("<td id=" + QUOTE);
		//htmlResponse.append("123" + QUOTE);
		//htmlResponse.append(">");
		htmlResponse.append("<tr>");
		htmlResponse.append("<td>Enter something</td>");
		htmlResponse.append("<input></input>");
		htmlResponse.append("<button type=\"submit\">");
		htmlResponse.append("<td>");
		htmlResponse.append(htmlRequest);
		htmlResponse.append("</td></tr></table></body></html>");
		cout << htmlResponse;
		return 0;
	}
	catch_and_trace
}