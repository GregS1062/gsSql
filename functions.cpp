#pragma once
#include <vector>
#include "defines.h"
#include "sqlCommon.h"
/******************************************************
 * SUM 
 ******************************************************/
TempColumn* Sum(TempColumn* _reportCol, TempColumn* _readCol)
{
    if(_readCol->edit == t_edit::t_double)
        _reportCol->doubleValue = _reportCol->doubleValue + _readCol->doubleValue;

    if(_readCol->edit == t_edit::t_int)
        _reportCol->intValue = _reportCol->intValue + _readCol->intValue;

    return _reportCol;
}
/******************************************************
 * MAX
 ******************************************************/
TempColumn* Max(TempColumn* _reportCol, TempColumn* _readCol)
{
    if(_readCol->edit == t_edit::t_double)
    {
        if(_readCol->doubleValue > _reportCol->doubleValue)
            _reportCol->doubleValue = _readCol->doubleValue;
    }
    if(_readCol->edit == t_edit::t_int)
    {
        if (_readCol->intValue > _reportCol->intValue)
             _reportCol->intValue = _readCol->intValue;
    }
    return _reportCol;
}
/******************************************************
 * MIN
 ******************************************************/
TempColumn* Min(TempColumn* _reportCol, TempColumn* _readCol)
{
    if(_readCol->edit == t_edit::t_double)
    {
        if(_readCol->doubleValue < _reportCol->doubleValue)
            _reportCol->doubleValue = _readCol->doubleValue;
    }

    if(_readCol->edit == t_edit::t_int)
    {
        if (_readCol->intValue < _reportCol->intValue)
             _reportCol->intValue = _readCol->intValue;
    }
    return _reportCol;
}
/******************************************************
 * Call functions
 ******************************************************/
void callFunctions(vector<TempColumn*> _reportRow, vector<TempColumn*> _row)
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
        }
    }
}
