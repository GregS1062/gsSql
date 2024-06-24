#pragma once
#include <vector>
#include "defines.h"
#include "sqlCommon.h"
/******************************************************
 * SUM 
 ******************************************************/
shared_ptr<TempColumn> Sum(shared_ptr<TempColumn> _reportCol, shared_ptr<TempColumn> _readCol)
{
    try
    {
        switch(_readCol->edit)
        {
            case t_edit::t_double:
                _reportCol->doubleValue = _reportCol->doubleValue + _readCol->doubleValue;
                break;
            case t_edit::t_int:
                _reportCol->intValue = _reportCol->intValue + _readCol->intValue;
                break;
            default:
                break;
        }
        return _reportCol;
    }
    catch_and_trace
}
/******************************************************
 * MAX
 ******************************************************/
shared_ptr<TempColumn> Max(shared_ptr<TempColumn> _reportCol, shared_ptr<TempColumn> _readCol)
{
    try
    {
        switch(_readCol->edit)
        {
            case t_edit::t_double:
                if(_readCol->doubleValue > _reportCol->doubleValue)
                    _reportCol->doubleValue = _readCol->doubleValue;
                break;
            case t_edit::t_int:
                if (_readCol->intValue > _reportCol->intValue)
                    _reportCol->intValue = _readCol->intValue;
                break;
            case t_edit::t_char:
                if(_readCol->charValue.compare( _reportCol->charValue) > 0)
                    _reportCol->charValue = _readCol->charValue;
                break;
            case t_edit::t_date:
                if(_readCol->dateValue.yearMonthDay > _reportCol->dateValue.yearMonthDay)
                    _reportCol->dateValue = _readCol->dateValue;
                break;
            default:
                break;
        }
        return _reportCol;
    }
    catch_and_trace
    return _reportCol;
}
/******************************************************
 * MIN
 ******************************************************/
shared_ptr<TempColumn> Min(shared_ptr<TempColumn> _reportCol, shared_ptr<TempColumn> _readCol)
{
    try
    {
        switch(_readCol->edit)
        {
            case t_edit::t_double:
                if(_readCol->doubleValue < _reportCol->doubleValue)
                    _reportCol->doubleValue = _readCol->doubleValue;
                break;
            case t_edit::t_int:
                if (_readCol->intValue < _reportCol->intValue)
                    _reportCol->intValue = _readCol->intValue;
                break;
            case t_edit::t_char:
                if(_readCol->charValue.compare( _reportCol->charValue) < 0)
                    _reportCol->charValue = _readCol->charValue;
                break;
            case t_edit::t_date:
                if(_readCol->dateValue.yearMonthDay < _reportCol->dateValue.yearMonthDay)
                    _reportCol->dateValue = _readCol->dateValue;
                break;
            default:
                break;
        }
        return _reportCol;
    }
    catch_and_trace
    return _reportCol;
}
/******************************************************
 * Call functions
 ******************************************************/
void callFunctions(vector<shared_ptr<TempColumn>> _reportRow, vector<shared_ptr<TempColumn>> _row)
{
    try
    {
        for (size_t nbr = 0;nbr < _row.size();nbr++) 
        {
            switch(_row.at(nbr)->functionType)
            {
                case t_function::NONE:
                    break;
                case t_function::COUNT:
                    _reportRow.at(nbr)->intValue++;
                    break;
                case t_function::SUM:
                    _reportRow.at(nbr) = Sum(_reportRow.at(nbr),_row.at(nbr));
                    break;
                case t_function::MAX:
                    _reportRow.at(nbr) = Max(_reportRow.at(nbr),_row.at(nbr));
                    break;
                case t_function::MIN:
                    _reportRow.at(nbr) = Min(_reportRow.at(nbr),_row.at(nbr));
                    break;  
                case t_function::AVG:
                    _reportRow.at(nbr) = Sum(_reportRow.at(nbr),_row.at(nbr));
                    break;  
                default:
                    break;
            }
        }
    }
    catch_and_trace
}
