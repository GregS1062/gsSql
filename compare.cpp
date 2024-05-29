#pragma once
#include <vector>
#include <list>
#include "sqlCommon.h"

/******************************************************
 * Evaluate Comparisons
 ******************************************************/
ParseResult evaluateComparisons(char* _op, int compareResult)
{
	if(strcasecmp(_op,sqlTokenNotEqual) == 0)
	{
		if(compareResult != 0)
			return ParseResult::SUCCESS;
		else
			return ParseResult::FAILURE;
	}
	
	if(strcasecmp(_op,sqlTokenEqual) == 0
	|| strcasecmp(_op,sqlTokenLike) == 0)
	{
		if (compareResult == 0)
			return ParseResult::SUCCESS;
		else
			return ParseResult::FAILURE;
	}

	if(strcasecmp(_op,sqlTokenGreater) == 0)
	{
		if (compareResult > 0)
			return ParseResult::SUCCESS;
		else
			return ParseResult::FAILURE;
	}

	if(strcasecmp(_op,sqlTokenLessThan) == 0)
	{
		if (compareResult < 0)
			return ParseResult::SUCCESS;
		else
			return ParseResult::FAILURE;
	}

	if(strcasecmp(_op,sqlTokenLessOrEqual) == 0)
	{
		if(compareResult == 0)
			return ParseResult::SUCCESS;
		
		if(compareResult < 0)
			return ParseResult::SUCCESS;

		return ParseResult::FAILURE;
	}

	if(strcasecmp(_op,sqlTokenGreaterOrEqual) == 0)
	{
		if(compareResult == 0)
			return ParseResult::SUCCESS;
		
		if(compareResult > 0)
			return ParseResult::SUCCESS;

		return ParseResult::FAILURE;
	}

	return ParseResult::FAILURE;
}
/******************************************************
 * Compare like
 ******************************************************/
ParseResult compareLike(Condition* _condition, char* _line)
{
	//Like condition compares the condition value length to the record
	// so "sch" = "schiller" because only three characters are being compared
	
	char buffRecord[60];
	Column* column = _condition->col;
	strncpy(buffRecord, _line+column->position, strlen(_condition->value));
	buffRecord[strlen(_condition->value)] = '\0';
		
	if(strcasecmp(_condition->value,buffRecord) == 0)
	{
		return ParseResult::SUCCESS;
	}

	return ParseResult::FAILURE;
}
/******************************************************
 * Compare String To String
 ******************************************************/
ParseResult compareStringToString(char* _op, char* _str1, char* _str2)
{
	return evaluateComparisons(_op, strcasecmp(_str1,_str2));
}
/******************************************************
 * Compare String Condition
 ******************************************************/
ParseResult compareStringToConditionValue(Condition* _condition, char* _line)
{
	//Equal or Not Equal condition compares the full column length to the value to the record
	// so "sch" != "schiller" 

	//Everything else compares only the length of the value

	char buffRecord[60];
	Column* column = _condition->col;
	
	strncpy(buffRecord, _line+column->position, column->length);
	rTrim(buffRecord);
	
	if (strcasecmp(_condition->op,sqlTokenEqual) == 0
	|| strcasecmp(_condition->op,sqlTokenNotEqual) == 0)
		buffRecord[_condition->col->length] = '\0';
	else
		buffRecord[strlen(_condition->col->value)] = '\0';
		

	return compareStringToString(_condition->op, buffRecord,_condition->value);
}
/******************************************************
 * Compare Integer To Integer
 ******************************************************/
 ParseResult compareIntegerToInteger(char* _op, int _int1, int _int2 )
 {
	int compareResult = 0;

	if(_int1 == _int2)
		compareResult = 0;

	if(_int1 != _int2)
		compareResult = NEGATIVE;

	if(_int1 > _int2)
		compareResult = 1;

	if(_int1 < _int2)
		compareResult = NEGATIVE;

	return evaluateComparisons(_op, compareResult);
} 
/******************************************************
 * Compare Integer Condition
 ******************************************************/
 ParseResult compareIntegerToConditionValue(Condition* _condition, char* _line)
{
	int  recordInt;
	Column* column = _condition->col;
	memcpy(&recordInt, _line+column->position, column->length);

	return compareIntegerToInteger(_condition->op, recordInt, _condition->intValue);
}
/******************************************************
 * Compare Double To Double
 ******************************************************/
 ParseResult compareDoubleToDouble(char* _op, double _dbl1, double _dbl2 )
 {

	int compareResult = 0;

	if( _dbl1 == _dbl2)
		compareResult = 0;

	if(_dbl1 != _dbl2)
		compareResult = NEGATIVE;

	if( _dbl1 >  _dbl2)
		compareResult = 1;

	if(_dbl1 <  _dbl2)
		compareResult = NEGATIVE;

	return evaluateComparisons(_op, compareResult);
} 
/******************************************************
 * Compare double Condition
 ******************************************************/
 ParseResult compareDoubleToConditionValue(Condition* _condition, char* _line)
{
	double  recordf;
	Column* column = _condition->col;
	memcpy(&recordf, _line+column->position, column->length);

	return compareDoubleToDouble(_condition->op, recordf, _condition->doubleValue);
}

/******************************************************
 * Compare Date Condition
 ******************************************************/
 int compareDateToDate(t_tm _dateValue, t_tm _date)
 {

	if( _date.yearMonthDay == _dateValue.yearMonthDay)
		return 0;

	if(_date.yearMonthDay != _dateValue.yearMonthDay)
		return NEGATIVE;

	if(_date.yearMonthDay > _dateValue.yearMonthDay)
		return 1;

	if(_date.yearMonthDay < _dateValue.yearMonthDay)
		return NEGATIVE;
	
	return -2;
} 

/******************************************************
 * Compare Date Condition
 ******************************************************/
 ParseResult compareDateToConditionValue(Condition* _condition, char* _line)
{
	t_tm recordDate;
	Column* column = _condition->col;
	memcpy(&recordDate, _line+column->position, column->length);

	return evaluateComparisons(_condition->op, compareDateToDate(_condition->dateValue, recordDate));
}

/******************************************************
 * Query Conditions Met
 ******************************************************/
ParseResult queryContitionsMet(list<Condition*> _conditions,char* _line)
{
	//Nothing to see, move along
	if(_conditions.size() == 0)
		return ParseResult::SUCCESS;

	ParseResult Queryesult = ParseResult::FAILURE;
	ParseResult Queryesults = ParseResult::FAILURE;

	for(Condition* condition : _conditions)
	{	
		switch(condition->col->edit)
		{
			case t_edit::t_char:
			{
				Queryesult = compareStringToConditionValue(condition,_line);
				break;
			}
			case t_edit::t_int:
			{
				Queryesult = compareIntegerToConditionValue(condition,_line);
				break;
			}
			case t_edit::t_double:
			{
				Queryesult = compareDoubleToConditionValue(condition,_line);
				break;
			}
			case t_edit::t_date:
			{	
				Queryesult = compareDateToConditionValue(condition,_line);
				break;
			}
				
			case t_edit::t_bool:
				break;
		}
		
		if(_conditions.size() == 1)
			return Queryesult;

		if(condition->condition != nullptr)
		{

			if(strcasecmp(condition->condition,(char*)sqlTokenAnd) != 0
			&& strcasecmp(condition->condition,(char*)sqlTokenOr) != 0)
			{
				if (_conditions.size() == 1
				&& Queryesult == ParseResult::FAILURE)
					return ParseResult::FAILURE;
			}

			if(strcasecmp(condition->condition,(char*)sqlTokenAnd) == 0
			&& (Queryesult == ParseResult::FAILURE
			|| Queryesults == ParseResult::FAILURE))
				return ParseResult::FAILURE;

			if(strcasecmp(condition->condition,(char*)sqlTokenOr)  == 0
			&& (Queryesult == ParseResult::SUCCESS
			|| Queryesults == ParseResult::SUCCESS))
				return ParseResult::SUCCESS;
		}
		Queryesults = Queryesult;		
	}

	if(Queryesults == ParseResult::SUCCESS)
		return ParseResult::SUCCESS;	

	return ParseResult::FAILURE;
}
/******************************************************
 * Compare Temp Column
 ******************************************************/
signed int compareTempColumns(TempColumn* col1,TempColumn* col2)
{
	//Assuming col1 and col2 are same edit
	if(col1->edit != col2->edit)
		return COMPAREERROR;

	signed int x;
	switch(col1->edit)
	{
		case t_edit::t_char:
		{
			x = strcmp(col1->charValue, col2->charValue);
			break;
		}
		case t_edit::t_int:
		{
			if(col1->intValue == col2->intValue)
				x = 0;
			else
			if(col1->intValue > col2->intValue)
				x = 1;
			else
				x = -1;
			break;
		}
		case t_edit::t_double:
		{
			if(col1->doubleValue == col2->doubleValue)
				x = 0;
			else
			if(col1->doubleValue > col2->doubleValue)
				x = 1;
			else
				x = -1;
			break;
		}
		case t_edit::t_date:
		{
			if(col1->dateValue.yearMonthDay == col2->dateValue.yearMonthDay)
				x = 0;
			else
			if(col1->dateValue.yearMonthDay > col2->dateValue.yearMonthDay)
				x = 1;
			else
				x = -1;
			break;
		}
		case t_edit::t_bool:
			if(col1->boolValue
			&& col2->boolValue)
				x = 0;
			else														
			if(col1->boolValue
			&& !col2->boolValue)
				x = 1;
			else
				x = -1;
	}
	return x;
}
/******************************************************
 * Compare Having Condition
 ******************************************************/
ParseResult compareHavingCondition(TempColumn* _rowCol, Condition* _con)
{
	// Assuming both rowCol and condition col have the proper edit values int, double, date etc
	if(_rowCol->edit != _con->col->edit)
		return ParseResult::FATAL;

	switch(_rowCol->edit)
	{
		case t_edit::t_char:
			return compareStringToString(_con->op, _rowCol->charValue, _con->value);
			break;
		case t_edit::t_int:
			return compareIntegerToInteger(_con->op, _rowCol->intValue, _con->intValue);
			break;
		case t_edit::t_double:
			return compareDoubleToDouble(_con->op, _rowCol->doubleValue, _con->doubleValue);
			break;
		case t_edit::t_date:
			return evaluateComparisons(_con->op, compareDateToDate(_con->dateValue, _rowCol->dateValue));
			break;
		case t_edit::t_bool: //NOTE: group by having - should not test a boolean
			break;
	}

	// error if we get here
	return ParseResult::FAILURE;
}
/******************************************************
 * Having Conditions Met
 ******************************************************/
ParseResult compareHavingConditions(vector<TempColumn*> _row, list<Condition*> _conditions)
{
	ParseResult ret;
	// 1) roll through all having conditions
	// 2) match condition column to result row column
	//		3) match on alias
	//		4) match on column name
	// 5) if match found compare values
	// 6) if values fail compare return FAILURE
	for(Condition* con : _conditions)
	{
		for(size_t i = 0;i < _row.size();i++)
		{
			if(con->col->alias != nullptr 
			&& _row.at(i)->alias != nullptr)
			{
				if(strcasecmp(con->col->alias,_row.at(i)->alias) == 0 )
				{
					ret = compareHavingCondition(_row.at(i),con);
					if(ret != ParseResult::SUCCESS)
						return ret;
				}
			}
			else
			{
				if(strcasecmp(con->col->name,_row.at(i)->name) == 0 )
				{
					ret = compareHavingCondition(_row.at(i),con);
					if(ret != ParseResult::SUCCESS)
						return ret;
				}
			}
		}
	}
	return ParseResult::SUCCESS;
}

