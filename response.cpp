#pragma once
#include <list>
#include <vector>
#include <bits/stdc++.h> 
#include "compare.cpp"
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
class response
{
	vector<vector<shared_ptr<TempColumn>>> groupRows{};
	list<int>					lstSort;

public:
	response();
	vector<vector<shared_ptr<TempColumn>>> rows{};
    int                         rowCount = 0;
    PRESENTATION                presentation;
    ParseResult                 addRow			(vector<shared_ptr<TempColumn>>);
	shared_ptr<TempColumn>		getCountColumn	(int);
    ParseResult                 Sort			(list<int>,bool);
	ParseResult					orderBy(shared_ptr<OrderBy>);
	ParseResult					groupBy(shared_ptr<GroupBy>,bool);
    
};
response::response()
{

}
/******************************************************
 * Add Row
 ******************************************************/
ParseResult response::addRow(vector<shared_ptr<TempColumn>> _row)
{
	rows.push_back(_row);
    return ParseResult::SUCCESS;
}

/******************************************************
 * sort
 ******************************************************/
ParseResult response::Sort(list<int> _n, bool _ascending)
{
	if(_n.size() == 0)
		return ParseResult::FAILURE;

     std::sort(rows.begin(), rows.end(),
            [&](const vector<shared_ptr<TempColumn>> row1,const vector<shared_ptr<TempColumn>> row2) {
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
ParseResult response::orderBy(shared_ptr<OrderBy> _orderBy)
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
ParseResult response::groupBy(shared_ptr<GroupBy> _groupBy, bool _functions)
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
		if(group.col->name.empty())
			return ParseResult::FAILURE;

		n.push_back(group.columnNbr);
	}

	// 2)
	Sort(n,true);
	
	if(rows.size() == 0)
		return ParseResult::FAILURE;

	// 3)
	vector<shared_ptr<TempColumn>> priorRow = (vector<shared_ptr<TempColumn>>)rows[0];
	vector<shared_ptr<TempColumn>> reportRow = (vector<shared_ptr<TempColumn>>)rows[0];
	vector<shared_ptr<TempColumn>> tempRow;

	int avgCount = 1;  //includes first row
	bool controlBreak = false;

	// 4) Read 
	for (size_t i = 1; i < rows.size(); i++) { 
		vector<shared_ptr<TempColumn>> row = (vector<shared_ptr<TempColumn>>)rows[i];
		
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
shared_ptr<TempColumn> response::getCountColumn(int count)
{
	shared_ptr<TempColumn> countCol = make_shared<TempColumn>();
	countCol->name 		= (char*)"Count";
	countCol->edit 		= t_edit::t_int;
	countCol->intValue 	= count;
	countCol->length	= sizeof(int);
	return countCol;
}
