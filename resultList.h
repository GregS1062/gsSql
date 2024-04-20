#include <list>
#include <vector>
#include <bits/stdc++.h> 
#include "sqlCommon.h"
/*----------------------------------------------------------------------
    Case: return results in HTML form
    Case: return results in text form
    Case: sort results before return
    Case: sort and group results before return
    Case: retain result for join
------------------------------------------------------------------------*/
using namespace std;
/******************************************************
 * Result List
 ******************************************************/
class resultList
{
    vector<vector<TempColumn*>> rows{};
   // list<Column*>               orderBy;
    static bool                 compare(vector<TempColumn*>,vector<TempColumn*>);

public:
	RETURNACTION				returnAction;
    int                         rowCount = 0;
    PRESENTATION                presentation;
    ParseResult                 addRow(vector<TempColumn*>);
	void						printRow(vector<TempColumn*>);
    string                      formatColumn(TempColumn*);
	void						printColumn(TempColumn*);

	void						print();
    ParseResult                 Sort();


	//vector<vector<Column*>>     getResults();
    //ParseResult                 addOrderBy(Column*);
    
};
/******************************************************
 * Add Row
 ******************************************************/
ParseResult resultList::addRow(vector<TempColumn*> _row)
{
	rows.push_back(_row);
    return ParseResult::SUCCESS;
}

/******************************************************
 * Print
 ******************************************************/
void resultList::print()
{

	for (size_t i = 0; i < rows.size(); i++) { 
		vector<TempColumn*> row = (vector<TempColumn*>)rows[i];
		printRow(row);
	}
}

/******************************************************
 * Print Row Vector
 ******************************************************/
void resultList::printRow(vector<TempColumn*> _row)
{
	string result;
	result.append("\n");
	if(presentationType == PRESENTATION::HTML)
	{
		//newline and tabs aid the reading of html source
		result.append("\t\t");
		result.append(rowBegin);
	}
	 for (auto it = begin (_row); it != end (_row); ++it) 
	 {
 		TempColumn* col = (TempColumn*)*it;
		printColumn(col);
    }

	if(presentationType == PRESENTATION::HTML)
		result.append(rowEnd);

	returnResult.resultTable.append(result);

}

/******************************************************
 * sort
 ******************************************************/
ParseResult resultList::Sort()
{
    sort(rows.begin(), rows.end(),compare); 
    return ParseResult::SUCCESS;
}
/******************************************************
 * compare
 ******************************************************/
bool resultList::compare(vector<TempColumn*> row1,vector<TempColumn*> row2)
{
     int x = strcmp(row1.at(4)->charValue, row2.at(4)->charValue);
	 if(x == 0)
	 {
		x = strcmp(row1.at(2)->charValue, row2.at(2)->charValue);
	 }
     if(x < 0)
      return true;
     else
        return false;
}
/******************************************************
 * Format Ouput
 ******************************************************/
void resultList::printColumn(TempColumn* col)
{
	size_t pad = 0;
	string result;

	if(presentationType == PRESENTATION::HTML)
	{
		result.append(cellBegin);
		result.append(formatColumn(col));
		result.append(cellEnd);
	}
	else
	{
		string out = formatColumn(col);
		result.append(out);
		if((size_t)col->length > out.length())
		{
			pad = col->length - out.length();
			for(size_t i =0;i<pad;i++)
			{
				result.append(" ");
			}
		}
	}

	returnResult.resultTable.append(result);
	return;
}
/******************************************************
 * Format Ouput
 ******************************************************/
string resultList::formatColumn(TempColumn* _col)
{

	std::stringstream ss;
	string formatString;


	switch(_col->edit)
	{
		case t_edit::t_char:
		{
			return formatString.append(_col->charValue);
			break;
		}
		case t_edit::t_bool:
		{
			if(_col->boolValue)
				return "T";
			else
				return "F";
			break;
		}
		case t_edit::t_int:
		{
			return formatString.append(std::to_string(_col->intValue));
			break;
		}
		case t_edit::t_double:
		{
			if(_col->doubleValue < 0){
				ss << "-$" << std::fixed << std::setprecision(2) << -_col->doubleValue; 
				} else {
				ss << "$" << std::fixed << std::setprecision(2) << _col->doubleValue; 
			}
			return ss.str();
			break;
		}
		case t_edit::t_date:
		{
			formatString.append(std::to_string(_col->dateValue.month));
			formatString.append("/");
			formatString.append(std::to_string(_col->dateValue.day));
			formatString.append("/");
			formatString.append(std::to_string(_col->dateValue.year));
			return formatString;
		}
		
	}
	return "e";
}

