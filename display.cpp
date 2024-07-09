
#pragma once
#include <list>
#include <vector>
#include "sqlCommon.h"
class display
{
    vector<vector<shared_ptr<TempColumn>>> rows{};
	list<shared_ptr<columnParts>>   reportColumns;

    ParseResult displayHeader(vector<shared_ptr<TempColumn>> _cols);
    string      textHeader      (vector<shared_ptr<TempColumn>>);
    string      htmlHeader      (vector<shared_ptr<TempColumn>>,size_t);
    string      formatColumn	(shared_ptr<TempColumn>);
	void	    displayColumn	(shared_ptr<TempColumn>);
    void        displayRow(vector<shared_ptr<TempColumn>> _row);

	
    public:

    void 							displayResponse(vector<vector<shared_ptr<TempColumn>>>,list<shared_ptr<columnParts>>);
	vector<shared_ptr<TempColumn>> 	orderColumns(vector<shared_ptr<TempColumn>>,list<int>);

};
/******************************************************
 * Display Response
 ******************************************************/
void display::displayResponse(vector<vector<shared_ptr<TempColumn>>> _rows,list<shared_ptr<columnParts>>   _reportColumns)
{
	if(_rows.size() == 0)
		return;

	/*
		Row columns are returned in table order, not in the order requested by the user
	*/

	reportColumns = _reportColumns;
	rows = _rows;
	vector<shared_ptr<TempColumn>> row = rows.front();
	list<int> orderMask;

	/*
		Build a mask list to put report columns in user order
	*/
	for(shared_ptr<columnParts> parts : reportColumns)
	{
		t_function ag = getFunctionType(parts->function);
		int i = 0;
		for( shared_ptr<TempColumn> col: row)
		{
			if (strcasecmp(parts->columnName.c_str(),col->name.c_str()) == 0
			and strcasecmp(parts->tableName.c_str(),col->tableName.c_str()) == 0
			and ag == col->functionType)
			{
				orderMask.push_back(i);
				break;
			}
			i++;
		}
	}

	vector<shared_ptr<TempColumn>> orderedRow = orderColumns(row,orderMask);
	displayHeader(orderedRow);

	for (size_t i = 0; i < rows.size(); i++) { 
		row = (vector<shared_ptr<TempColumn>>)rows[i];
		if(row.size() > 0)
		{
			orderedRow = orderColumns(row,orderMask);
			displayRow(orderedRow);
		}
	}
}

/******************************************************
 * Order columns: Arrange columns in order requested by the user
 ******************************************************/
vector<shared_ptr<TempColumn>> display::orderColumns(vector<shared_ptr<TempColumn>> _cols,list<int> _orderMask)
{
	vector<shared_ptr<TempColumn>> orderedRow;

	for(int order : _orderMask)
		orderedRow.push_back(_cols.at(order));

	return orderedRow;
}
/******************************************************
 * Display Header
 ******************************************************/
ParseResult display::displayHeader(vector<shared_ptr<TempColumn>> _cols)
{
	string header;
	size_t sumOfColumnSize = 0;
	for( shared_ptr<TempColumn> col: _cols)
	{
		if(!col->display)
			continue;
		
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
 * Text Header
 ******************************************************/
string display::textHeader(vector<shared_ptr<TempColumn>> _columns)
{
	size_t pad = 0;
	string header;
	header.append("\n");
	for (shared_ptr<Column> col : _columns) 
	{
		if(!col->display)
			continue;

		if(col->length > col->name.length())
		{
			if(!col->alias.empty())
			{
				header.append(col->alias);
				pad = col->length - col->alias.length();
			}
			else
			{
				header.append(col->name);
				pad = col->length - col->name.length();
			}
			for(size_t i =0;i<pad;i++)
			{
				header.append(" ");
			}
		}
		else
		{
			if(!col->alias.empty())
				header.append(col->alias);
			else
				header.append(col->name);
			header.append(" ");
		}
	}
	header.append("\n");
	return header;
}
/******************************************************
 * HTML Header
 ******************************************************/
string display::htmlHeader(vector<shared_ptr<TempColumn>> _columns, size_t _sumOfColumnSize)
{
	double percentage = 0;
	int pad = 0;
	string header;
	header.append(rowBegin);

	if(_sumOfColumnSize < 100)
	{
		pad = 100 - (int)_sumOfColumnSize;
		//fprintf(traceFile,"\n pad = %d",pad);
		_sumOfColumnSize = _sumOfColumnSize + pad;
	}
	for (shared_ptr<Column> col : _columns) 
	{
		if(!col->display)
			continue;

		header.append("\n\t");
		header.append(hdrBegin);
		header.append(" style="" width:");
		if(col->edit == t_edit::t_date)
		{
			percentage = 12 / (int)_sumOfColumnSize * 100;
		}
		else
			percentage = (double)col->length / (int)_sumOfColumnSize * 100;
		header.append(to_string((int)percentage));
		header.append("%"">");
		if(!col->alias.empty())
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
		percentage = (double)pad / (int)_sumOfColumnSize * 100;
		fprintf(traceFile,"\n sum of col = %li",_sumOfColumnSize);
		fprintf(traceFile,"\n pad percent = %f",percentage);
		header.append(to_string((int)percentage));
		header.append("%"">");
		header.append(hdrEnd);
	}
	header.append(rowEnd);
	return header;
}
/******************************************************
 * display Column
 ******************************************************/
void display::displayColumn(shared_ptr<TempColumn> col)
{
	size_t pad = 0;
	string result = "";

	if(col == nullptr)
	{
		return;
	}

	if(!col->display)
		return;

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
string display::formatColumn(shared_ptr<TempColumn> _col)
{

	std::stringstream ss;
	string formatString;

	if(_col == nullptr)
		return "";

	switch(_col->edit)
	{
		case t_edit::t_char:
		{
			if(_col->charValue.empty())
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
		default:
		 	break;
	}
	return "";
}
/******************************************************
 * Print Row Vector
 ******************************************************/
void display::displayRow(vector<shared_ptr<TempColumn>> _row)
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
 		shared_ptr<TempColumn> col = (shared_ptr<TempColumn>)*it;
		if(col != nullptr)
			displayColumn(col);
    }

	if(presentationType == PRESENTATION::HTML)
		result.append(rowEnd);

	returnResult.resultTable.append(result);

}





