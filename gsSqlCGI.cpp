#include <stdio.h>
#include <iostream>
#include <fstream>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <syslog.h>
#include "parseJason.h"
#include "parseSql.h"
#include "sqlClassLoader.h"
#include "sqlEngine.h"
#include "global.h"
#include "userException.h"
//#include "pipes.h"
#include "keyValue.h"

#include <cgicc/CgiDefs.h> 
#include <cgicc/Cgicc.h> 
#include <cgicc/HTTPHTMLHeader.h> 
#include <cgicc/HTMLClasses.h>  

using namespace std;
using namespace cgicc;

/*---------------------------------------
   Main
-----------------------------------------*/
int main()
{
    Cgicc formData;

	string htmlRequest = "select top 5 * from customer";

	string htmlResponse;

	ofstream traceFile;

	try
	{
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
		
		sqlParser* parser = new sqlParser();
		sqlClassLoader* loader = new sqlClassLoader();

		if(parser->parse(htmlRequest.c_str()) == ParseResult::SUCCESS)
		{
		
			loader->loadSqlClasses("dbDef.json","bike");
			
			cTable* qtable = loader->getTableByName((char*)"customer");

			if(qtable == nullptr)
			{
				returnResult.error.append("customer table not found");
			}
			else{
			
				sqlEngine* engine = new sqlEngine(parser,qtable);
				if(engine->open() == ParseResult::SUCCESS)
				{
					if(engine->ValidateQueryColumns() == ParseResult::SUCCESS)
					{
						if(engine->query->sqlAction == SQLACTION::SELECT)
							returnResult.resultTable = engine->fetchData();
						if(engine->query->sqlAction == SQLACTION::INSERT)
						{
							if(engine->storeData() == ParseResult::SUCCESS)
								returnResult.message.append(" Record Stored");
						}
					}
				}
			}
		}

		returnResult.error.append(errText);

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

		traceFile.open ("gsSqlTrace.txt");
		traceFile << htmlResponse;
		traceFile.close();

		//Send Response
		cout << htmlResponse;
		return 0;
	}
    catch(const std::exception& e)
    {
        errText.append( e.what());
        return 0;
    } 
}