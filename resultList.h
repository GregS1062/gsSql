#pragma once
#include <list>
#include <vector>
#include <bits/stdc++.h> 
#include "sqlCommon.h"
#include "functions.cpp"
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
	vector<vector<TempColumn*>> groupRows{};

public:
	resultList();

	vector<vector<TempColumn*>> rows{};
    int                         rowCount = 0;
    PRESENTATION                presentation;
	list<int>					lstSort;
    ParseResult                 addRow			(vector<TempColumn*>);
    string                      formatColumn	(TempColumn*);
	void						printColumn		(TempColumn*);
	TempColumn*					getCountColumn	(int);
    ParseResult                 Sort			(list<int>,bool);
	ParseResult					orderBy(OrderBy*);
	ParseResult					groupBy(GroupBy*,bool);
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
					x = compareTempColumns(row1.at(nbr), row2.at(nbr));
					
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
ParseResult resultList::groupBy(GroupBy* _groupBy, bool _functions)
{

	if(debug)
		fprintf(traceFile,"\n-----------------------Group by-----------------------");

	/*
	Pre-condition: a table of rows input from the sqlEngine
		Logic:
			1) Get list of the positions of sort columns, call it (n)
			2) Use (n) list to sort the results
			3) Save the prior row to detect a control break
			4) Read sorted list
			5) Compare each column in current row to column in prior row to detect a control break
			6) Call functions logic
			7) Copy contents of prior row into a temporary row
			8) Test Having conditions
			9) Store temporary row in new output table
	*/

	// 1)
	list<int> n;
	for(OrderOrGroup group : _groupBy->group)
	{
		if(group.col->name == nullptr)
			return ParseResult::FAILURE;

		n.push_back(group.columnNbr);
	}

	// 2)
	Sort(n,true);
	
	if(rows.size() == 0)
		return ParseResult::FAILURE;

	// 3)
	vector<TempColumn*> priorRow = (vector<TempColumn*>)rows[0];
	vector<TempColumn*> reportRow = (vector<TempColumn*>)rows[0];
	vector<TempColumn*> tempRow;

	int avgCount = 1;  //includes first row
	bool controlBreak = false;

	// 4) Read 
	for (size_t i = 1; i < rows.size(); i++) { 
		vector<TempColumn*> row = (vector<TempColumn*>)rows[i];
		
		// 5)
		for(OrderOrGroup group : _groupBy->group)
		{	
			if(compareTempColumns(row.at(group.columnNbr),priorRow.at(group.columnNbr)) != 0)
			{
				controlBreak = true;
				// 7)
				for(size_t i2 = 0; i2 < reportRow.size(); i2++)
				{
					if(reportRow.at(i2)->functionType == t_function::AVG)
					{
						if(avgCount > 0)
						{
							if( reportRow.at(i2)->edit == t_edit::t_double)
								reportRow.at(i2)->doubleValue = reportRow.at(i2)->doubleValue / avgCount;

						}
						else
						{
							reportRow.at(i2)->intValue = 0;
							reportRow.at(i2)->doubleValue = 0;
						}
					}
					
					tempRow.push_back(reportRow.at(i2));
				}
				// 8)
				
				if(compareHavingConditions(tempRow,_groupBy->having) == ParseResult::SUCCESS)
					groupRows.push_back(tempRow);

				tempRow.clear();

				//The current row becomes the template row
				reportRow = row;
				avgCount = 0;
			}

			//Do not want to trip counts and sums on a control break
			if(_functions && !controlBreak)
			{
				callFunctions(reportRow,row);
			}
			avgCount++;
			controlBreak = false;
			priorRow = row;
		}
	}
	//End of file is a control break
	for(size_t i2 = 0; i2 < reportRow.size(); i2++)
	{
		if(reportRow.at(i2)->functionType == t_function::AVG)
		{
			if(avgCount > 0)
			{
				if( reportRow.at(i2)->edit == t_edit::t_double)
					reportRow.at(i2)->doubleValue = reportRow.at(i2)->doubleValue / avgCount;

			}
		}
	}

	rows = groupRows;

	bool sortByCount = false;
	int sortColumn = 0;
	for(size_t i2 = 0; i2 < reportRow.size(); i2++)
	{
		if(reportRow.at(i2)->functionType == t_function::COUNT)
		{
			sortColumn = (int)i2;
			sortByCount = true;
			break;
		}
	}
	if(sortByCount)
	{
		list<int> c;
		c.push_back(sortColumn);
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


