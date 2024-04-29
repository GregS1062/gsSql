#pragma once

#include <ctime>
#include <cstring>
#include <string.h>
#include <cstdio>
#include "defines.h"

using namespace std;

/*******************************************************
   String Duplicate
*******************************************************/
char* dupString(const char* str)
{
	int len = (int)strlen(str);
	if(len == 0)
	{
		char* cpy = (char*)malloc(1);
		cpy[len] = '\0';
		return cpy;
	}
	char* cpy = (char*)malloc(len+1);
	memcpy(cpy, str, len);
	cpy[len] = '\0';
	return cpy;
}

/*******************************************************
   Message TEXT
*******************************************************/
void messageTEXT(MESSAGETYPE _type, bool _newLine, const char* _msg)
{
	string msg;
	if(_newLine)
		msg.append("\n");

	msg.append(_msg);

	if(_type == MESSAGETYPE::ERROR)
		errText.append(msg);
	else
		msgText.append(msg);
}
/*******************************************************
   Message HTML
*******************************************************/
void messageHTML(MESSAGETYPE _type, bool _newLine, const char* _msg)
{
	string msg;

	if(_type == MESSAGETYPE::ERROR)
	{
		if(errText.empty())
			_newLine = false;
		msg.append("<font color=\"crimson\">");
	}
	else
	{
		if(msgText.empty())
			_newLine = false;
		msg.append("<font color=\"navy\">");
	}

	msg.append(_msg);
	msg.append("</font>");

	if(_type == MESSAGETYPE::ERROR)
	{
		if(_newLine)
			errText.append("<br>");
		errText.append(msg);
	}
	else
	{
		if(_newLine)
			msgText.append("<br>");
		msgText.append(msg);
	}
};
/*******************************************************
   Send Message
*******************************************************/
void sendMessage( MESSAGETYPE _type, PRESENTATION _presentation,bool _newLine, const char* _msg)
{
	if(_msg == nullptr)
		_msg = "";

	if(_presentation == PRESENTATION::HTML)
		messageHTML(_type,_newLine,_msg);
	else
		messageTEXT(_type,_newLine,_msg);
}
/*******************************************************
   Pad Left
*******************************************************/
char* padLeft(char target[], int max)
{
    char buff[60];
	if (target == nullptr)
		return nullptr;
	if ((int)strlen(target) >= max)
	{
		char* tmp;
		tmp = strdup(target);
		return tmp;
	}

	int pad = max - (int)strlen(target);

	for (int i = 0; i < max; i++)
	{
		buff[i + pad] = target[i];
	}
	for (int i = 0; i < pad; i++)
	{
		buff[i] = '0';
	}
	buff[max] = '\0';
	char* tmp;
	tmp = strdup(buff);
	return tmp;
}
/*******************************************************
   Formate Date
*******************************************************/
void formatDate(char s[], t_tm _date)
{
	//Has time structure been initialized (conversion issue)
	if (_date.year < 1)
	{
		strcpy(s, "");
		return;
	}

	char str[11] = "";

	sprintf(str,"%d", _date.month);

	strcpy(s, padLeft(str, 2));
	strcat(s, "/");

	sprintf(str,"%d", _date.day);

	strcat(s, padLeft(str, 2));

	strcat(s, "/");

	sprintf(str, "%d", _date.year);

	strcat(s, str);
}

/*******************************************************
   upperCase  Convert lower to upper case
*******************************************************/
void upperCase(char s[])
{
	for (int i = 0; s[i] != '\0'; i++) {
		if (s[i] >= 'a' && s[i] <= 'z') {
			s[i] = (char)(s[i] - 32);
		}
	}
}
/*******************************************************
   SubString
*******************************************************/
void terminateString(char target[], int start, int end)
{
	for (int i = 0; i < end - start; i++)
	{
		target[i] = target[i + start];
	}
	target[end - start] = '\0';
}

/*******************************************************
   Left Trim
*******************************************************/
void lTrim(char _target[], char _ch)
{
	if(_target[0] != _ch)
		return;
		
	size_t len = strlen(_target);
	size_t i = 0;
	size_t i2 = 0;
	for (i = 0; i < len; i++)
	{
		if (_target[i] != _ch)
			break;
	}
	for (; i < len; i++)
	{
		_target[i2] = _target[i];
		i2++;
	}
	if (i2 < i)
		_target[i2] = '\0';
}

/*******************************************************
   SubString
*******************************************************/
void subString(char target[], int start, int end)
{
	for (int i = 0; i < end - start; i++)
	{
		target[i] = target[i + start];
	}
	target[end - start] = '\0';
}
/*******************************************************
   Scrub token
*******************************************************/
void scrub(char _target[])
{
	char c;
	size_t len = strlen(_target);
	size_t itr = 0;
	for(size_t i = 0;i<len;i++)
	{		
		c = _target[i];
		if((int)c == 0	//null
		|| c == NEWLINE
        || c == RETURN
        || c == FORMFEED
        || c == TAB
        || c == VTAB)
        {
            continue;
        }
		_target[itr] = c;
		itr++;
	}
	_target[itr] = '\0';
}
/*******************************************************
   Right Trim
-*******************************************************/
void rTrim(char _target[])
{
	int len = (int)strlen(_target)-1;
	char ch = ' ';
	for (int i=len; i > 0; i--)
	{
		if(_target[i] == ch)
		{
			_target[i] = '\0';
		}
		else
		{
			break;
		}
	}


}

/*******************************************************
   Parse Date
*******************************************************/
t_tm parseDate(char _date[])
{
	// pre-condition:	date has been validated
	//					date is in the form MM/DD/YYYY
	t_tm _d;
    _d.year     = 0;
    _d.month    = 0;
    _d.day      = 0;

	lTrim(_date, ' ');
	if (strlen(_date) == 0)
		return _d;

	char digits[10];
	int i2 = 0;
	for (int i = 0; i < 10; i++)
	{
		if (isdigit(_date[i]))
		{
			digits[i2] = _date[i];
			i2++;
		}
	}
	digits[i2] = '\0';

	char str[11];

	//strcpy_s(str, 11, digits);
	strcpy(str, digits);
	subString(str, 0, 2);
	_d.month = atoi(str);


	strcpy(str, digits);
	subString(str, 2, 4);
	_d.day = atoi(str);


	strcpy(str, digits);
	subString(str, 4, 8);
	_d.year = atoi(str);

	_d.yearMonthDay = _d.year * 10000;
	_d.yearMonthDay = _d.yearMonthDay + (_d.month * 100);
	_d.yearMonthDay = _d.yearMonthDay + _d.day;
	return _d;
}

/********************************************************
	Find and replace
 ******************************************************/
string findAndReplace(string s,string _target, string _text)
{
	if(s.find(_target) < s.length())
	{
		s.replace(s.find(_target), _target.size(), _text);
	}
	return s;
};
/*******************************************************
   Is Date Valid
*******************************************************/
bool isDateValid(char* _cdate)
{
	//Blank date is valid
	if(_cdate == nullptr)
		return true;
		
	if(strlen(_cdate) == 0)
		return true;

	string _date;
	_date.append(_cdate);
	//if blank, return without error

	if (_date.length() != 10)
	{
		sendMessage(MESSAGETYPE::ERROR,presentationType,false,_cdate);
		sendMessage(MESSAGETYPE::ERROR,presentationType,false," Invalid date, format must be MM/DD/YYYY");
		return false;
	}
	char digits[10];
	int i2 = 0;
	for (int i = 0; i < 10; i++)
	{
		if (isdigit(_date[i]))
		{
			digits[i2] = _date[i];
			i2++;
		}
	}
	digits[i2] = '\0';

	if (strlen(digits) != 8)
	{
		sendMessage(MESSAGETYPE::ERROR,presentationType,false,_cdate);
		sendMessage(MESSAGETYPE::ERROR,presentationType,false," Invalid date, format must be MM/DD/YYYY");
		return false;
	}

	int month = atoi(_date.substr(0,2).c_str());
	int day = atoi(_date.substr(3,2).c_str());
    int year = atoi(_date.substr(6,4).c_str());

    const int lookup_table[12] = {31,29,31,30,31,30,31,31,30,31,30,31};
    if ((month < 1 || month > 12))
    {
		sendMessage(MESSAGETYPE::ERROR,presentationType,false,_cdate);
        sendMessage(MESSAGETYPE::ERROR,presentationType,false," Invalid date, format must be MM/DD/YYYY: Month is invalid ");
        return false;
    }
    if (!(day >= 1 && day <= lookup_table[month-1]))
    {
		sendMessage(MESSAGETYPE::ERROR,presentationType,false,_cdate);
        sendMessage(MESSAGETYPE::ERROR,presentationType,false," Invalid date, format must be MM/DD/YYYY: Day is invalid ");
        return false;
    }
    if (year < 1900 || year > 2033)
    {
		sendMessage(MESSAGETYPE::ERROR,presentationType,false,_cdate);
        sendMessage(MESSAGETYPE::ERROR,presentationType,false," Invalid date, format must be MM/DD/YYYY: Year is invalid ");
        return false;
    }

	return true;
}
/******************************************************
 * Is Numeric
 ******************************************************/
bool isNumeric(char* _token)
{

    if(_token == nullptr)
        return false;

    for(size_t i=0;i<strlen(_token);i++)
    {
        if(_token[i] == '.')
            continue;
        if(!isdigit(_token[i]))
            return false;
    }
    return true;
}

/*******************************************************
   Strip quotes from token
*******************************************************/
void stripQuotesFromToken(char _token[])
{
	//strip quotes from value
	size_t s = 0;
	size_t len = strlen(_token);
	for(size_t i = 0;i< len; i++)
	{
		if(_token[i] == QUOTE)
			s++;

		if(_token[s] == QUOTE)
		{
			_token[i] = '\0';
		}
		else{
			_token[i] = _token[s];
		}
		s++;  
	}
}
		