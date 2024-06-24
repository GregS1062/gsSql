#pragma once
#include <vector>
#include <list>
#include "sqlCommon.h"
#include "utilities.cpp"

/******************************************************
 * Evaluate Comparisons
 ******************************************************/
ParseResult evaluateComparisons(string _op, int compareResult)
{
	char* op = (char*)_op.c_str();
	if(strcasecmp(op,sqlTokenNotEqual) == 0)
	{
		if(compareResult != 0)
			return ParseResult::SUCCESS;
		else
			return ParseResult::FAILURE;
	}
	
	if(strcasecmp(op,sqlTokenEqual) == 0
	|| strcasecmp(op,sqlTokenLike) == 0)
	{
		if (compareResult == 0)
			return ParseResult::SUCCESS;
		else
			return ParseResult::FAILURE;
	}

	if(strcasecmp(op,sqlTokenGreater) == 0)
	{
		if (compareResult > 0)
			return ParseResult::SUCCESS;
		else
			return ParseResult::FAILURE;
	}

	if(strcasecmp(op,sqlTokenLessThan) == 0)
	{
		if (compareResult < 0)
			return ParseResult::SUCCESS;
		else
			return ParseResult::FAILURE;
	}

	if(strcasecmp(op,sqlTokenLessOrEqual) == 0)
	{
		if(compareResult == 0)
			return ParseResult::SUCCESS;
		
		if(compareResult < 0)
			return ParseResult::SUCCESS;

		return ParseResult::FAILURE;
	}

	if(strcasecmp(op,sqlTokenGreaterOrEqual) == 0)
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
ParseResult compareLike(shared_ptr<Condition> _condition, char* _line)
{
	//Like condition compares the condition value length to the record
	// so "sch" = "schiller" because only three characters are being compared
	
	char buffRecord[60];
	shared_ptr<Column> column = _condition->col;
	strncpy(buffRecord, _line+column->position, _condition->value.length());
	buffRecord[_condition->value.length()] = '\0';
		
	if(strcasecmp(_condition->value.c_str(),buffRecord) == 0)
	{
		return ParseResult::SUCCESS;
	}

	return ParseResult::FAILURE;
}
/******************************************************
 * Compare String To String
 ******************************************************/
ParseResult compareStringToString(string _op, string _str1, string _str2)
{
	return evaluateComparisons(_op, strcasecmp(_str1.c_str(),_str2.c_str()));
}
/******************************************************
 * Compare String Condition
 ******************************************************/
ParseResult compareStringToConditionValue(shared_ptr<Condition> _condition, char* _line)
{
	//Equal or Not Equal condition compares the full column length to the value to the record
	// so "sch" != "schiller" 

	//Everything else compares only the length of the value

	char buffRecord[60];
	shared_ptr<Column> column = _condition->col;
	
	strncpy(buffRecord, _line+column->position, column->length);
	
	if (strcasecmp(_condition->op.c_str(),sqlTokenEqual) == 0
	|| strcasecmp(_condition->op.c_str(),sqlTokenNotEqual) == 0)
	{
		buffRecord[_condition->col->length] = '\0';
		rTrim(buffRecord);
	}
	else
		buffRecord[_condition->value.length()] = '\0';
		

	return compareStringToString(_condition->op, buffRecord,_condition->value);
}
/******************************************************
 * Compare Integer To Integer
 ******************************************************/
 ParseResult compareIntegerToInteger(string _op, int _int1, int _int2 )
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
 ParseResult compareIntegerToConditionValue(shared_ptr<Condition> _condition, char* _line)
{
	int  recordInt;
	shared_ptr<Column> column = _condition->col;
	memcpy(&recordInt, _line+column->position, column->length);

	return compareIntegerToInteger(_condition->op.c_str(), recordInt, _condition->intValue);
}
/******************************************************
 * Compare Double To Double
 ******************************************************/
 ParseResult compareDoubleToDouble(string _op, double _dbl1, double _dbl2 )
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
 ParseResult compareDoubleToConditionValue(shared_ptr<Condition> _condition, char* _line)
{
	double  recordf;
	shared_ptr<Column> column = _condition->col;
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
 ParseResult compareDateToConditionValue(shared_ptr<Condition> _condition, char* _line)
{
	t_tm recordDate;
	shared_ptr<Column> column = _condition->col;
	memcpy(&recordDate, _line+column->position, column->length);

	return evaluateComparisons(_condition->op, compareDateToDate(_condition->dateValue, recordDate));
}
/******************************************************
 * Get String
 ******************************************************/
string getString(shared_ptr<Column> _col, char* _line)
{
	char buffRecord[60];
	
	strncpy(buffRecord, _line+_col->position, _col->length);
	buffRecord[_col->length] = '\0';
	return string(buffRecord);
} 
/******************************************************
 * Compare Columns
 ******************************************************/
ParseResult compareColumns(shared_ptr<Condition> _con,char* _line)
{

	switch(_con->col->edit)
	{
		case t_edit::t_char:
		{
			string str1 = getString(_con->col,_line);
			string str2 = getString(_con->compareToColumn,_line);
			return compareStringToString(_con->op, str1,str2);
			break;
		}
		case t_edit::t_int:
		{
			int i1 = 0;
			int i2 = 0;
			memcpy(&i1, _line+_con->col->position, _con->col->length);
			memcpy(&i2, _line+_con->compareToColumn->position, _con->compareToColumn->length);
			return compareIntegerToInteger(_con->op,i1,i2);
			break;
		}
		case t_edit::t_double:
		{
			double d1 = 0;
			double d2 = 0;
			memcpy(&d1, _line+_con->col->position, _con->col->length);
			memcpy(&d2, _line+_con->compareToColumn->position, _con->compareToColumn->length);
			return compareDoubleToDouble(_con->op,d1,d2);
			break;
		}
		case t_edit::t_date:
		{
			t_tm d1;
			t_tm d2;
			memcpy(&d1, _line+_con->col->position, _con->col->length);
			memcpy(&d2, _line+_con->compareToColumn->position, _con->compareToColumn->length);
			return evaluateComparisons(_con->op,compareDateToDate(d1,d2));
			break;
		}
		case t_edit::t_bool:
			sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Boolean comparison not implemented.");
            return ParseResult::FAILURE;
			break;
		default:
			sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Comparison column edit unknown.");
            return ParseResult::FAILURE;
			break;
	}
	return ParseResult::FAILURE;
}

/******************************************************
 * Query Conditions Met
 ******************************************************/
ParseResult queryContitionsMet(list<shared_ptr<Condition>> _conditions,char* _line)
{
	//Nothing to see, move along
	if(_conditions.size() == 0)
		return ParseResult::SUCCESS;

	ParseResult queryResult = ParseResult::FAILURE;
	ParseResult queryResults = ParseResult::FAILURE;

	for(shared_ptr<Condition> condition : _conditions)
	{	
		if(condition->compareToColumn != nullptr )
		{
			queryResult = compareColumns(condition,_line);
		}
		else
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
				default:
					break;
			}
		}

		
		if(_conditions.size() == 1)
			return queryResult;

		if(!condition->condition.empty())
		{

			if(strcasecmp(condition->condition.c_str(),(char*)sqlTokenAnd) != 0
			&& strcasecmp(condition->condition.c_str(),(char*)sqlTokenOr) != 0)
			{
				if (_conditions.size() == 1
				&& queryResult == ParseResult::FAILURE)
					return ParseResult::FAILURE;
			}

			if(strcasecmp(condition->condition.c_str(),(char*)sqlTokenAnd) == 0
			&& (queryResult == ParseResult::FAILURE
			|| queryResults == ParseResult::FAILURE))
				return ParseResult::FAILURE;

			if(strcasecmp(condition->condition.c_str(),(char*)sqlTokenOr)  == 0
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
 * Compare Temp Column
 ******************************************************/
signed int compareTempColumns(shared_ptr<TempColumn> col1,shared_ptr<TempColumn> col2)
{
	//Assuming col1 and col2 are same edit
	if(col1->edit != col2->edit)
		return COMPAREERROR;

	signed int x;
	switch(col1->edit)
	{
		case t_edit::t_char:
		{
			x = strcmp(col1->charValue.c_str(), col2->charValue.c_str());
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
		default:
			break;
	}
	return x;
}
/******************************************************
 * Compare Having Condition
 ******************************************************/
ParseResult compareHavingCondition(shared_ptr<TempColumn> _rowCol, shared_ptr<Condition> _con)
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
		default:
			break;
	}

	// error if we get here
	return ParseResult::FAILURE;
}
/******************************************************
 * Having Conditions Met
 ******************************************************/
ParseResult compareHavingConditions(vector<shared_ptr<TempColumn>> _row, list<shared_ptr<Condition>> _conditions)
{
	ParseResult ret;
	// 1) roll through all having conditions
	// 2) match condition column to result row column
	//		3) match on alias
	//		4) match on column name
	// 5) if match found compare values
	// 6) if values fail compare return FAILURE
	for(shared_ptr<Condition> con : _conditions)
	{
		for(size_t i = 0;i < _row.size();i++)
		{
			if(!con->col->alias.empty() 
			&& !_row.at(i)->alias.empty())
			{
				if(strcasecmp(con->col->alias.c_str(),_row.at(i)->alias.c_str()) == 0 )
				{
					ret = compareHavingCondition(_row.at(i),con);
					if(ret != ParseResult::SUCCESS)
						return ret;
				}
			}
			else
			{
				if(strcasecmp(con->col->name.c_str(),_row.at(i)->name.c_str()) == 0 )
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

