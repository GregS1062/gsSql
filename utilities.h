#pragma once

#include <ctime>
#include <cstring>
#include <string.h>
#include <cstdio>
#include "global.h"

class utilities {
public:
	static void 	formatDate(char[], t_tm);
	static char* 	padLeft(char[], int);
	static void 	lTrim(char[], char);
	static void 	rTrim(char[]);
	static void 	subString(char target[], int start, int end);
	static bool		isNumeric(std::string);
	static void 	upperCase(char[]);
	static void 	terminateString(char[], int, int);
	static t_tm 	parseDate(char[]);
	static string   findAndReplace(string s,string _target, string _text);
};

/*---------------------------------------
   Formate Date
-----------------------------------------*/
void utilities::formatDate(char s[], t_tm _date)
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


/*---------------------------------------
   upperCase  Convert lower to upper case
-----------------------------------------*/
void utilities::upperCase(char s[])
{
	for (int i = 0; s[i] != '\0'; i++) {
		if (s[i] >= 'a' && s[i] <= 'z') {
			s[i] = (char)(s[i] - 32);
		}
	}
}
/*---------------------------------------
   SubString
-----------------------------------------*/
void utilities::terminateString(char target[], int start, int end)
{
	for (int i = 0; i < end - start; i++)
	{
		target[i] = target[i + start];
	}
	target[end - start] = '\0';
}

/*---------------------------------------
   Pad Left
-----------------------------------------*/
char* utilities::padLeft(char target[], int max)
{
    char buffer[60];
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
		buffer[i + pad] = target[i];
	}
	for (int i = 0; i < pad; i++)
	{
		buffer[i] = '0';
	}
	buffer[max] = '\0';
	char* tmp;
	tmp = strdup(buffer);
	return tmp;
}


/*---------------------------------------
   Left Trim
-----------------------------------------*/
void utilities::lTrim(char _target[], char _ch)
{
	int len = (int)strlen(_target);
	int i = 0;
	int i2 = 0;
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

/*---------------------------------------
   SubString
-----------------------------------------*/
void utilities::subString(char target[], int start, int end)
{
	for (int i = 0; i < end - start; i++)
	{
		target[i] = target[i + start];
	}
	target[end - start] = '\0';
}/*---------------------------------------
   Right Trim
-----------------------------------------*/
void utilities::rTrim(char _target[])
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

/*---------------------------------------
   Parse Date
-----------------------------------------*/
t_tm utilities::parseDate(char _date[])
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

/*---------------------------------------------*
	Find and replace
 ----------------------------------------------*/
string utilities::findAndReplace(string s,string _target, string _text)
{
	if(s.find(_target) < s.length())
	{
		s.replace(s.find(_target), _target.size(), _text);
	}
	return s;
};
		