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
	ParseResult					printHeader		(vector<TempColumn*>);
	string  					htmlHeader		(vector<TempColumn*>,int32_t);
	string  					textHeader		(vector<TempColumn*>);
	void						printRow		(vector<TempColumn*>);
	vector<vector<TempColumn*>> tempRows{};

public:
	resultList();

	vector<vector<TempColumn*>> rows{};
	RETURNACTION				returnAction;
    int                         rowCount = 0;
    PRESENTATION                presentation;
	list<int>					lstSort;
    ParseResult                 addRow			(vector<TempColumn*>);
    string                      formatColumn	(TempColumn*);
	void						printColumn		(TempColumn*);
	TempColumn*					getCountColumn	(int);
    ParseResult                 Sort			(list<int>,bool);
	ParseResult					orderBy(OrderBy*);
	ParseResult					groupBy(GroupBy*);
	void						print();
    
};
resultList::resultList()
{

}
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
	if(rows.size() == 0)
		return;

	vector<TempColumn*> row = rows.front();
	printHeader(row);
	for (size_t i = 0; i < rows.size(); i++) { 
		row = (vector<TempColumn*>)rows[i];
		if(row.size() > 0)
			printRow(row);
	}
}

/******************************************************
 * Print Row Vector
 ******************************************************/
void resultList::printRow(vector<TempColumn*> _row)
{
	if(_row.size() == 0)
		return;
		
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
		if(col != nullptr)
			printColumn(col);
    }

	if(presentationType == PRESENTATION::HTML)
		result.append(rowEnd);

	returnResult.resultTable.append(result);

}

/******************************************************
 * sort
 ******************************************************/
ParseResult resultList::Sort(list<int> _n, bool _ascending)
{
	if(_n.size() == 0)
		return ParseResult::FAILURE;

     std::sort(rows.begin(), rows.end(),
            [&](const vector<TempColumn*> row1,const vector<TempColumn*> row2) {
                // compare last names first
              int x = 0;
				for(int nbr : _n)
				{
					switch(row1.at(nbr)->edit)
					{
						case t_edit::t_char:
						{
							x = strcmp(row1.at(nbr)->charValue, row2.at(nbr)->charValue);
							break;
						}
						case t_edit::t_int:
						{
							if(row1.at(nbr)->intValue == row2.at(nbr)->intValue)
								x = 0;
							else
							if(row1.at(nbr)->intValue > row2.at(nbr)->intValue)
							   x = 1;
							else
								x = -1;
							break;
						}
						case t_edit::t_double:
						{
							if(row1.at(nbr)->doubleValue == row2.at(nbr)->doubleValue)
								x = 0;
							else
							if(row1.at(nbr)->doubleValue > row2.at(nbr)->doubleValue)
							   x = 1;
							else
								x = -1;
							break;
						}
						case t_edit::t_date:
						{
							if(row1.at(nbr)->dateValue.yearMonthDay == row2.at(nbr)->dateValue.yearMonthDay)
								x = 0;
							else
							if(row1.at(nbr)->dateValue.yearMonthDay > row2.at(nbr)->dateValue.yearMonthDay)
							   x = 1;
							else
								x = -1;
							break;
						}
						case t_edit::t_bool:
							if(row1.at(nbr)->boolValue
							&& row2.at(nbr)->boolValue)
								x = 0;
							else														
							if(row1.at(nbr)->boolValue
							&& !row2.at(nbr)->boolValue)
								x = 1;
							else
								x = -1;
					}
					
					if(x != 0)
					{
						if(_ascending)
						{
							if(x < 0)
								return true;
							else
								return false;
						}
						else
						{
							if(x > 0)
								return true;
							else
								return false;
						}
					}

					
				}
				if(_ascending)
				{
					if(x < 0)
						return true;
					else
						return false;
				}
				else
				{
					if(x > 0)
						return true;
					else
						return false;
				}
            });
    return ParseResult::SUCCESS;
}
/******************************************************
 * Order By
 ******************************************************/
ParseResult resultList::orderBy(OrderBy* _orderBy)
{
	list<int> n;
	for(OrderOrGroup order : _orderBy->order)
	{
		if(debug)
			fprintf(traceFile,"\n sorting on column# %d", order.columnNbr);
		n.push_back(order.columnNbr);
	}
	Sort(n,_orderBy->asc);
	return ParseResult::SUCCESS;
}
/******************************************************
 * Group By
 ******************************************************/
ParseResult resultList::groupBy(GroupBy* _groupBy)
{

	if(debug)
		fprintf(traceFile,"\n-----------------------Group by-----------------------");

	list<int> n;
	bool sortByCount = true;
	for(OrderOrGroup group : _groupBy->group)
	{
		if(group.col->name == nullptr)
			return ParseResult::FAILURE;
		
		if(strcasecmp(group.col->name,(char*)sqlTokenCount) == 0)
		{
			if(debug)
				fprintf(traceFile,"\nsort by count = true");
			sortByCount = true;
		}
		else
		{
			if(debug)
				fprintf(traceFile,"\n grouping on column# %d", group.columnNbr);
			n.push_back(group.columnNbr);
		}
	}

	Sort(n,true);

	vector<TempColumn*> priorRow;
	vector<TempColumn*> sortRow;
	bool first = true;
	bool controlBreak = true;
	int count = 0;
	int newRowSize = 0;
	
	for (size_t i = 0; i < rows.size(); i++) { 
		vector<TempColumn*> row = (vector<TempColumn*>)rows[i];
		if(first)
		{
			priorRow = row;
			first = false;
		}
		for(OrderOrGroup group : _groupBy->group)
		{	
			count++;
			if(row.at(group.columnNbr)->edit == t_edit::t_char)
			{
				if(strcasecmp(row.at(group.columnNbr)->charValue,priorRow.at(group.columnNbr)->charValue ) != 0)
					controlBreak = true;
			}
			else
			if(row.at(group.columnNbr) != priorRow.at(group.columnNbr))
				controlBreak = true;

			if(controlBreak)
			{
				sortRow.push_back(getCountColumn(count));
				for(int i2 : n)
				{
					sortRow.push_back(priorRow.at(i2));
				}
				tempRows.push_back(sortRow);

				//Get position of count in the row for sorting by count
				if(newRowSize == 0)
					newRowSize = (int)sortRow.size();
				sortRow.clear();
				count = 0;
				controlBreak = false;
			}
		}	
		priorRow = row;
	}
	rows = tempRows;
	if(sortByCount)
	{
		list<int> c;
		c.push_back(0);
		if(debug)
			fprintf(traceFile,"\n group by sorting count at %d", newRowSize);
		Sort(c,false);
	}
	return ParseResult::SUCCESS;
}
/******************************************************
 * Get Group Row
 ******************************************************/
TempColumn* resultList::getCountColumn(int count)
{
	TempColumn* countCol = new TempColumn();
	countCol->name 		= (char*)"Count";
	countCol->edit 		= t_edit::t_int;
	countCol->intValue 	= count;
	countCol->length	= sizeof(int);
	return countCol;
}
/******************************************************
 * print Column
 ******************************************************/
void resultList::printColumn(TempColumn* col)
{
	size_t pad = 0;
	string result = "";

	if(col == nullptr)
	{
		return;
	}

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

	if(_col == nullptr)
		return "";

	switch(_col->edit)
	{
		case t_edit::t_char:
		{
			if(_col->charValue == nullptr)
				return "";
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
	return "";
}
/******************************************************
 * Pring Header
 ******************************************************/
ParseResult resultList::printHeader(vector<TempColumn*> _cols)
{
	string header;
	int sumOfColumnSize = 0;
	for( TempColumn* col: _cols)
	{
		if(col->edit == t_edit::t_date)
		{
			sumOfColumnSize = sumOfColumnSize + 16;
		}
		else
		{
			sumOfColumnSize = sumOfColumnSize + col->length;
		}
    }
	if(presentationType == PRESENTATION::HTML)
		header = htmlHeader(_cols,sumOfColumnSize);
	else
		header = textHeader(_cols);

	returnResult.resultTable.append(header);
	return ParseResult::SUCCESS;
}
/******************************************************
 * HTML Header
 ******************************************************/
string resultList::htmlHeader(vector<TempColumn*> _columns, int32_t _sumOfColumnSize)
{
	double percentage = 0;
	int pad = 0;
	string header;
	header.append(rowBegin);

	if(_sumOfColumnSize < 100)
	{
		pad = 100 - _sumOfColumnSize;
		//fprintf(traceFile,"\n pad = %d",pad);
		_sumOfColumnSize = _sumOfColumnSize + pad;
	}
	for (Column* col : _columns) 
	{
		header.append("\n\t");
		header.append(hdrBegin);
		header.append(" style="" width:");
		if(col->edit == t_edit::t_date)
		{
			percentage = 12 / _sumOfColumnSize * 100;
		}
		else
			percentage = (double)col->length / _sumOfColumnSize * 100;
		header.append(to_string((int)percentage));
		header.append("%"">");
		if(col->alias != nullptr)
		{
			header.append(col->alias);
		}
		else
			header.append(col->name);
		header.append(hdrEnd);
	}
	if(pad > 0)
	{
		header.append("\n\t");
		header.append(hdrBegin);
		header.append(" style="" width:");
		percentage = (double)pad / _sumOfColumnSize * 100;
		fprintf(traceFile,"\n sum of col = %d",_sumOfColumnSize);
		fprintf(traceFile,"\n pad percent = %f",percentage);
		header.append(to_string((int)percentage));
		header.append("%"">");
		header.append(hdrEnd);
	}
	header.append(rowEnd);
	return header;
}
/******************************************************
 * Text Header
 ******************************************************/
string resultList::textHeader(vector<TempColumn*> _columns)
{
	size_t pad = 0;
	string header;
	header.append("\n");
	for (Column* col : _columns) 
	{
		if((size_t)col->length > strlen(col->name))
		{
			if(col->alias != nullptr)
			{
				header.append(col->alias);
				pad = col->length - strlen(col->alias);
			}
			else
			{
				header.append(col->name);
				pad = col->length - strlen(col->name);
			}
			for(size_t i =0;i<pad;i++)
			{
				header.append(" ");
			}
		}
		else
		{
			if(col->alias != nullptr)
				header.append(col->alias);
			else
				header.append(col->name);
			header.append(" ");
		}
	}

	return header;
}


