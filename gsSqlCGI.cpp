#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <syslog.h>
#include "parseSQL.cpp"
#include "controller.cpp"

#include <cgicc/CgiDefs.h> 
#include <cgicc/Cgicc.h> 
#include <cgicc/HTTPHTMLHeader.h> 
#include <cgicc/HTMLClasses.h>  

using namespace std;
using namespace cgicc;

ParseResult runQuery(string _htmlRequest)
{

	std::string sqlFileName = "bike.sql";
	std::ifstream ifs(sqlFileName);
	std::string sqlFile ( (std::istreambuf_iterator<char>(ifs) ),
					(std::istreambuf_iterator<char>()    ) );

	debug = false;

	presentationType = PRESENTATION::HTML;
        
	auto sql = make_unique<parseSql>(sqlFile);
	debug = false;
	if(sql->parse() == ParseResult::FAILURE)
	{
		fprintf(traceFile,"\n******************SQL Parser FAILED*****************");
		fprintf(traceFile,"\n %s",errText.c_str());
		return ParseResult::FAILURE;
	}

	debug = true;

	controller controller(sql->isqlTables);
	controller.runQuery(_htmlRequest);

	return ParseResult::SUCCESS;
}
/*---------------------------------------
   Main
-----------------------------------------*/
int main()
{

    Cgicc formData;

	string htmlRequest = "select top 5 * from customers";

	string htmlResponse;

	traceFile = fopen ("trace.txt" , "w");

	try
	{
		presentationType = PRESENTATION::HTML;
		returnResult.resultTable.append(" ");
		returnResult.message.append(" ");
		returnResult.error.append(" ");

		Cgicc cgi = formData;
		const_form_iterator iter;
		for (iter = cgi.getElements().begin();
		iter != cgi.getElements().end();
		++iter)
		{
			if(iter->getName() == "query")
			{
				htmlRequest = iter->getValue().c_str();	
				break;			
			}
		}

		if (runQuery(htmlRequest) == ParseResult::FAILURE)
		{
			returnResult.resultTable = "";
		};

		returnResult.error.append(errText);
		returnResult.message.append(msgText);

		//Format output
		htmlResponse.append("Content-type:text/html\r\n\r\n");
		htmlResponse.append("<!DOCTYPE HTML//EN\" \"http://www.w3.org/TR/REC-html40/strict.dtd\">");
		htmlResponse.append("\n");
		htmlResponse.append("<html>");
		htmlResponse.append("\n<link rel=\"stylesheet\" href=\"gsSql.css\">");
		htmlResponse.append("\n<body>");
		htmlResponse.append("\n<FORM METHOD=POST ACTION=\"http://localhost/gsSql.cgi#\">");
		htmlResponse.append("\n<table style=""width:100%"">");
		htmlResponse.append("\n\t<tr>");
		htmlResponse.append("\n\t<td width=\"30%\"></td>");
		htmlResponse.append("\n\t<td width=\"30%\" style=\"text-align:center; font-size:34px;\">Enter Query</td>");
		htmlResponse.append("\n\t<td align=\"right\">");
		htmlResponse.append("\n\t\t\t<button class=\"btnSubmit\" style=\"height:50px; width:150px; font-size:24px;\" >Submit</button>");
		htmlResponse.append("\n\t\t\t</td>");
		htmlResponse.append("\n\t</tr>");
		htmlResponse.append("\n\t<tr>");
		htmlResponse.append("\n\t\t<td colspan=\"3\">");
		htmlResponse.append("\n\t\t<textarea name=\"query\" rows=\"5\" cols=\"100\" wrap=\"soft\" maxlength=\"1000\"");
		htmlResponse.append(" style=\"overflow:hidden; resize:none; font-size:24px;\">");
		htmlResponse.append(htmlRequest);
		htmlResponse.append("</textarea>");
		htmlResponse.append("\n\t\t</td>");
		htmlResponse.append("\n\t</tr>");
		htmlResponse.append("\n\t<tr>");
		htmlResponse.append("\n\t\t<td height=\"100px\" colspan=\"3\" style=\"font-size:24px; background-color: white; vertical-align:top;\">");
		htmlResponse.append("\n\t\t<div name=\"errmsg\" contenteditable=\"true\" style=\"font-size:24px; background-color: white;\">");
		htmlResponse.append("\n\t\t<span style=\"color: blue;\">");
		//Return messages
		htmlResponse.append(returnResult.message);
		htmlResponse.append("</span>");
		htmlResponse.append("\n\t\t<p><span style=\"color: red;\">");
		//Report errors
		htmlResponse.append(returnResult.error);
		htmlResponse.append("</span>");
		htmlResponse.append("\n\t\t</div>");
		htmlResponse.append("\n\t\t</td>");
		htmlResponse.append("\n\t</tr>");
		htmlResponse.append("\n</table>");
		htmlResponse.append("\n<table style=""width:100%"">");
		htmlResponse.append(returnResult.resultTable);
		htmlResponse.append("\n</table>");
		htmlResponse.append("\n</FORM>");
		htmlResponse.append("\n</body>");
		htmlResponse.append("\n</html>");

		//Send Response
		cout << htmlResponse;
		fclose(traceFile);
		return 0;
	}
    catch(const std::exception& e)
    {
        errText.append( e.what());
	//	traceFile.open ("gsSqlTrace.txt");
	//	traceFile <<  errText;
	//	traceFile.close();
		cout << "<html>";
		cout << errText;
		cout << "</html>";
        return 0;
    } 
}