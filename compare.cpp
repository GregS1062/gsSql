#pragma once
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
 * Compare Integer To Integer
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
 ParseResult compareDateToConditionValue(Condition* _condition, char* _line)
{
	t_tm recordDate;
	Column* column = _condition->col;
	memcpy(&recordDate, _line+column->position, column->length);

	int compareResult = 0;

	if( recordDate.yearMonthDay == _condition->dateValue.yearMonthDay)
		compareResult = 0;

	if(recordDate.yearMonthDay != _condition->dateValue.yearMonthDay)
		compareResult = NEGATIVE;

	if(recordDate.yearMonthDay > _condition->dateValue.yearMonthDay)
		compareResult = 1;

	if(recordDate.yearMonthDay < _condition->dateValue.yearMonthDay)
		compareResult = NEGATIVE;
	
	return evaluateComparisons(_condition->op, compareResult);
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



