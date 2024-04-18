#pragma once
#include "sqlCommon.h"
#include "sqlParser.h"


class compareToCondition
{
    public:
        static  ParseResult queryContitionsMet(list<Condition*>,char*);
        static	ParseResult compareLike(Condition*,char*);
        static	ParseResult compareStringToConditionValue(Condition*, char*);
        static	ParseResult compareIntegerToConditionValue(Condition*,char*);
        static	ParseResult compareDoubleToConditionValue(Condition*,char*);
        static	ParseResult compareDateToConditionValue(Condition*,char*);
		static	ParseResult compareStringToString(char*,char*, char*);
		static	ParseResult compareIntegerToInteger(char*,int, int);
		static	ParseResult compareDoubleToDouble(char*,double, double);
		static	ParseResult compareDateToDate(char*,t_tm, t_tm);

		static  ParseResult evaluateComparisons(char*, int);
};
/******************************************************
 * Query Conditions Met
 ******************************************************/
ParseResult compareToCondition::queryContitionsMet(list<Condition*> _conditions,char* _line)
{
	//Nothing to see, move along
	if(_conditions.size() == 0)
		return ParseResult::SUCCESS;

	ParseResult queryResult = ParseResult::FAILURE;
	ParseResult queryResults = ParseResult::FAILURE;

	for(Condition* condition : _conditions)
	{
		
		switch(condition->col->edit)
		{
			case t_edit::t_char:
			{
				queryResult = compareStringToConditionValue(condition,_line);
				break;
			}
			case t_edit::t_int:
			{
				queryResult = compareIntegerToConditionValue(condition,_line);
				break;
			}
			case t_edit::t_double:
			{
				queryResult = compareDoubleToConditionValue(condition,_line);
				break;
			}
			case t_edit::t_date:
			{	
				queryResult = compareDateToConditionValue(condition,_line);
				break;
			}
				
			case t_edit::t_bool:
				break;
		}
		
		if(_conditions.size() == 1)
			return queryResult;

		if(condition->condition != nullptr)
		{

			if(strcasecmp(condition->condition,(char*)sqlTokenAnd) != 0
			&& strcasecmp(condition->condition,(char*)sqlTokenOr) != 0)
			{
				if (_conditions.size() == 1
				&& queryResult == ParseResult::FAILURE)
					return ParseResult::FAILURE;
			}

			if(strcasecmp(condition->condition,(char*)sqlTokenAnd) == 0
			&& (queryResult == ParseResult::FAILURE
			|| queryResults == ParseResult::FAILURE))
				return ParseResult::FAILURE;

			if(strcasecmp(condition->condition,(char*)sqlTokenOr)  == 0
			&& (queryResult == ParseResult::SUCCESS
			|| queryResults == ParseResult::SUCCESS))
				return ParseResult::SUCCESS;
		}
		queryResults = queryResult;		
	}

	if(queryResults == ParseResult::SUCCESS)
		return ParseResult::SUCCESS;
	

	return ParseResult::FAILURE;
}
/******************************************************
 * Evaluate Comparisons
 ******************************************************/
ParseResult compareToCondition::evaluateComparisons(char* _op, int compareResult)
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
ParseResult compareToCondition::compareLike(Condition* _condition, char* _line)
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
 * Compare String Condition
 ******************************************************/
ParseResult compareToCondition::compareStringToConditionValue(Condition* _condition, char* _line)
{
	//Equal or Not Equal condition compares the full column length to the value to the record
	// so "sch" != "schiller" 

	//Everything else compares only the length of the value

	char buffRecord[60];
	Column* column = _condition->col;
	
	strncpy(buffRecord, _line+column->position, column->length);
	utilities::rTrim(buffRecord);
	
	if (strcasecmp(_condition->op,sqlTokenEqual) == 0
	|| strcasecmp(_condition->op,sqlTokenNotEqual) == 0)
		buffRecord[_condition->col->length] = '\0';
	else
		buffRecord[strlen(_condition->col->value)] = '\0';
		

	return compareStringToString(_condition->op, buffRecord,_condition->value);
}
/******************************************************
 * Compare String To String
 ******************************************************/
ParseResult compareToCondition::compareStringToString(char* _op, char* _str1, char* _str2)
{
	return evaluateComparisons(_op, strcasecmp(_str1,_str2));
}

/******************************************************
 * Compare Integer Condition
 ******************************************************/
 ParseResult compareToCondition::compareIntegerToConditionValue(Condition* _condition, char* _line)
{
	int  recordInt;
	Column* column = _condition->col;
	memcpy(&recordInt, _line+column->position, column->length);

	return compareIntegerToInteger(_condition->op, recordInt, _condition->intValue);
}
/******************************************************
 * Compare Integer To Integer
 ******************************************************/
 ParseResult compareToCondition::compareIntegerToInteger(char* _op, int _int1, int _int2 )
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
 * Compare double Condition
 ******************************************************/
 ParseResult compareToCondition::compareDoubleToConditionValue(Condition* _condition, char* _line)
{
	double  recordf;
	Column* column = _condition->col;
	memcpy(&recordf, _line+column->position, column->length);

	return compareDoubleToDouble(_condition->op, recordf, _condition->doubleValue);
}

/******************************************************
 * Compare Integer To Integer
 ******************************************************/
 ParseResult compareToCondition::compareDoubleToDouble(char* _op, double _dbl1, double _dbl2 )
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
 * Compare Date Condition
 ******************************************************/
 ParseResult compareToCondition::compareDateToConditionValue(Condition* _condition, char* _line)
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


