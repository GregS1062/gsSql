#pragma once
#include "global.h"
#include "sqlParser.h"

class Condition
{
    public:
        char*   name            = nullptr;  // described by user
        char*   op              = nullptr;  // operator is a reserved word
        char*   value           = nullptr;
        int     intValue        = 0;
        double  doubleValue     = 0;
        bool    boolValue       = false;
        t_tm    dateValue;
        char*   prefix          = nullptr; //  (
        char*   condition       = nullptr;
        char*   suffix          = nullptr;  // )
        column* col             = nullptr;  // actual column loaded by engine
};

class compareToCondition
{
    public:
        static  ParseResult queryContitionsMet(list<Condition*>,char*);
        static	ParseResult compareLike(Condition*,char*);
        static	ParseResult compareStringCondition(Condition*, char*);
        static	ParseResult compareIntegerCondition(Condition*,char*);
        static	ParseResult compareDoubleCondition(Condition*,char*);
        static	ParseResult compareDateCondition(Condition*,char*);
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
				if(strcasecmp(condition->op,"like") == 0)
				{
					queryResult = compareLike(condition,_line);
				}
				else
				{
					if(strcasecmp(condition->op,"=") == 0)
					{
						queryResult = compareStringCondition(condition,_line);
					}
				}
				break;
			}
			case t_edit::t_int:
			{
				queryResult = compareIntegerCondition(condition,_line);
				break;
			}
			case t_edit::t_double:
			{
				queryResult = compareDoubleCondition(condition,_line);
				break;
			}
			case t_edit::t_date:
			{	
				queryResult = compareDateCondition(condition,_line);
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
			&& queryResult == ParseResult::FAILURE)
				return ParseResult::FAILURE;
		
			if(strcasecmp(condition->condition,(char*)sqlTokenAnd)  == 0
			&& queryResult  == ParseResult::SUCCESS
			&& queryResults == ParseResult::SUCCESS)
				return ParseResult::SUCCESS;

			if(strcasecmp(condition->condition,(char*)sqlTokenOr)  == 0
			&& (queryResult == ParseResult::SUCCESS
			|| queryResults == ParseResult::SUCCESS))
				return ParseResult::SUCCESS;
		}
		queryResults = queryResult;		
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
	column* column = _condition->col;
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
ParseResult compareToCondition::compareStringCondition(Condition* _condition, char* _line)
{
	//Equal condition compares the full column length to the record
	// so "sch" != "schiller" 

	char buffRecord[60];
	column* column = _condition->col;
	strncpy(buffRecord, _line+column->position, column->length);
			buffRecord[column->length] = '\0';
	
	int compareResult = strcasecmp(_condition->value,buffRecord); 
	
	if(strcasecmp(_condition->op,sqlTokenEqual) == 0
	&& compareResult == 0)
	{
		return ParseResult::SUCCESS;
	}

	if(strcasecmp(_condition->op,sqlTokenNotEqual) == 0
	&& compareResult != 0)
	{
		return ParseResult::SUCCESS;
	}

	return ParseResult::FAILURE;
}
/******************************************************
 * Compare Integer Condition
 ******************************************************/
 ParseResult compareToCondition::compareIntegerCondition(Condition* _condition, char* _line)
{
	int  recordInt;
	column* column = _condition->col;
	memcpy(&recordInt, _line+column->position, column->length);


	if(strcasecmp(_condition->op,sqlTokenEqual) == 0
	&& _condition->intValue == recordInt)
	{
		return ParseResult::SUCCESS;
	}
	if(strcasecmp(_condition->op,sqlTokenNotEqual) == 0
	&& _condition->intValue != recordInt)
	{
		return ParseResult::SUCCESS;
	} 
	if(strcasecmp(_condition->op,sqlTokenGreater) == 0
	&& recordInt > _condition->intValue)
	{
		return ParseResult::SUCCESS;
	} 
	if(strcasecmp(_condition->op,sqlTokenLessThan) == 0
	&& recordInt < _condition->intValue)
	{
		return ParseResult::SUCCESS;
	}
	return ParseResult::FAILURE;
} 
/******************************************************
 * Compare Integer Condition
 ******************************************************/
 ParseResult compareToCondition::compareDoubleCondition(Condition* _condition, char* _line)
{
	double  recordf;
	column* column = _condition->col;
	memcpy(&recordf, _line+column->position, column->length);

	if(strcasecmp(_condition->op,sqlTokenEqual) == 0
	&& _condition->doubleValue == recordf)
	{
		return ParseResult::SUCCESS;
	}
	if(strcasecmp(_condition->op,sqlTokenNotEqual) == 0
	&& _condition->doubleValue != recordf)
	{
		return ParseResult::SUCCESS;
	} 
	if(strcasecmp(_condition->op,sqlTokenGreater) == 0
	&&  recordf > _condition->doubleValue)
	{
		return ParseResult::SUCCESS;
	} 
	if(strcasecmp(_condition->op,sqlTokenLessThan) == 0
	&&  recordf < _condition->doubleValue)
	{
		return ParseResult::SUCCESS;
	}
	return ParseResult::FAILURE;
} 
/******************************************************
 * Compare Date Condition
 ******************************************************/
 ParseResult compareToCondition::compareDateCondition(Condition* _condition, char* _line)
{
	t_tm recordDate;
	column* column = _condition->col;
	memcpy(&recordDate, _line+column->position, column->length);


	if(strcasecmp(_condition->op,sqlTokenEqual) == 0
	&& _condition->dateValue.yearMonthDay == recordDate.yearMonthDay)
	{
		return ParseResult::SUCCESS;
	}
	if(strcasecmp(_condition->op,sqlTokenNotEqual) == 0
	&& _condition->dateValue.yearMonthDay != recordDate.yearMonthDay)
	{
		return ParseResult::SUCCESS;
	} 
	if(strcasecmp(_condition->op,sqlTokenGreater) == 0
	&& recordDate.yearMonthDay > _condition->dateValue.yearMonthDay)
	{
		return ParseResult::SUCCESS;
	} 
	if(strcasecmp(_condition->op,sqlTokenLessThan) == 0
	&& recordDate.yearMonthDay < _condition->dateValue.yearMonthDay)
	{
		return ParseResult::SUCCESS;
	}
	return ParseResult::FAILURE;
} 


