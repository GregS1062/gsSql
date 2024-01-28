#pragma once
#include <iostream>
#include <stdio.h>  
#include <stdlib.h> 
#include "global.h"

using namespace std;

/*
    Static class to display messages.
*/
class Message
{
    public:
    static void message(enum DISPLAY _display, const char *_msg, const char *_info);
    static void trace(const char *_msg);
};
/*---------------------------------------*
	Display Message
  -----------------------------------------*/
void Message::message(enum DISPLAY _display, const char *_msg, const char *_info)
{
	try
	{
        //syslog(LOG_ERR, "%s %s", _msg, _info);
		cout << "\n<table width=\"80%\">";
		cout << "\n\t<tr style=\"background-color:";

		if (_display == DISPLAY::INFO)
			cout << infoColor;

		if (_display == DISPLAY::ERROR)
			cout << errorColor;

		cout << ";\">";
        cout << "\n\t\t<td width=\"40%\">";
        cout << _msg;
        cout << "</td>";
		cout << "\n\t\t<td width=\"40%\">";
        cout << _info;
        cout << "</td>";
        cout << "\n</tr>";
        cout << "\n</table>";
	}
	catch(exception &e)
    {
        cout << e.what();
    }
}
void Message::trace(const char *_msg)
{
    if(debug)
        cout << _msg << endl;
}