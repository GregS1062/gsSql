#pragma once
#include "sqlCommon.h"
#include "interfaces.h"
#include "binding.cpp"
void printParts(std::shared_ptr<columnParts> parts)
{
    fprintf(traceFile,"\n\t name: %s alias: %s tableName: %s function: %s",parts->columnName.c_str(), parts->columnAlias.c_str(), parts->tableAlias.c_str(), parts->function.c_str());
}
void printAction(SQLACTION _action)
{
    switch(_action)
    {
        case SQLACTION::SELECT:
            fprintf(traceFile,"\nAction = Select");
            break;
        case SQLACTION::UPDATE:
            fprintf(traceFile,"\nAction = Update");
            break;
        case SQLACTION::DELETE:
            fprintf(traceFile,"\nAction = Delete");
            break;
        case SQLACTION::INSERT:
            fprintf(traceFile,"\nAction = Insert");
            break;
        case SQLACTION::JOIN:
            fprintf(traceFile,"\nAction = Join");
            break;
        default:
            break;
    }
}
void printTable(shared_ptr<sTable> tbl)
{
    fprintf(traceFile,"\n*******************************************");
    fprintf(traceFile,"\n Statement Table");
    fprintf(traceFile,"\n*******************************************");
    fprintf(traceFile,"\n table name %s", tbl->name.c_str());
    fprintf(traceFile," alias %s", tbl->alias.c_str());
    fprintf(traceFile," record length %i", tbl->recordLength);
    for(shared_ptr<Column> col : tbl->columns)
    {
        fprintf(traceFile,"\n\t column name %s", col->name.c_str());
        fprintf(traceFile," alias %s", col->alias.c_str());
        fprintf(traceFile," value %s", col->value.c_str());
        fprintf(traceFile," function %i", (int)col->functionType);
        if(col->primary)
            fprintf(traceFile," PRIMARY");
    }
    fprintf(traceFile,"\n\n Conditions");
    for(shared_ptr<Condition> con : tbl->conditions)
    {
        printParts(con->name);
        if(con->compareToName != nullptr)
            printParts(con->compareToName);
        fprintf(traceFile,"\n\t condition condition %s", con->condition.c_str());
        fprintf(traceFile,"\n\t condition op        %s", con->op.c_str());
        fprintf(traceFile,"\n\t condition value     %s", con->value.c_str());
    }
    fprintf(traceFile,"\n\n Indexes ");
    for(shared_ptr<sIndex> idx : tbl->indexes)
    {
        fprintf(traceFile,"\n\t index name %s", idx->name.c_str());
        fprintf(traceFile,"  file name %s", idx->fileName.c_str());
        for(shared_ptr<Column> col : idx->columns)
        {
            fprintf(traceFile,"\n\t\t index column name %s", col->name.c_str());
        }
    }
}
void printOrderBy(shared_ptr<OrderBy> _orderBy)
{
    fprintf(traceFile,"\n*******************************************");
    fprintf(traceFile,"\n Order By");
    fprintf(traceFile,"\n*******************************************");
    for(OrderOrGroup order :_orderBy->order)
    {
        printParts(order.name);
    }
    if(_orderBy->asc)
        fprintf(traceFile,"\n\tascending");
    else
        fprintf(traceFile,"\n\tdescending");
}
void printGroupBy(shared_ptr<GroupBy> _groupBy)
{
    fprintf(traceFile,"\n*******************************************");
    fprintf(traceFile,"\n Group By");
    fprintf(traceFile,"\n*******************************************");
    for(OrderOrGroup group :_groupBy->group)
    {
        printParts(group.name);
    }

    fprintf(traceFile,"\n*******************************************");
    fprintf(traceFile,"\n Having");
    fprintf(traceFile,"\n*******************************************");

    for(shared_ptr<Condition> con :_groupBy->having)
    {
        printParts(con->name);
        fprintf(traceFile,"\n\t condition condition %s", con->condition.c_str());
        fprintf(traceFile,"\n\t condition op        %s", con->op.c_str());
        fprintf(traceFile,"\n\t condition value     %s", con->value.c_str());
    }

}
void printQuery(unique_ptr<iElements> _element)
{
    fprintf(traceFile,"\n\n\n*******************************************");
    fprintf(traceFile,"\n Print Diagnostics: Query Tables");
    fprintf(traceFile,"\n*******************************************");
    if(_element == nullptr)
    {
       fprintf(traceFile,"\n_element is null"); 
       return;
    }
    printAction(_element->sqlAction);
    fprintf(traceFile,"\nRow Count:%i",_element->rowsToReturn);
    fprintf(traceFile,"\nTable Name:%s",_element->tableName.c_str());
    if(_element->lstColumns.size() == 0)
    {
        fprintf(traceFile,"\n No column");
    }
    else
    {
        fprintf(traceFile,"\n\n--------------------------------------------");
        fprintf(traceFile,"\n column names");
        fprintf(traceFile,"\n\n--------------------------------------------");
        for(std::shared_ptr<columnParts> parts : _element->lstColumns)
        {
            printParts(parts);
        }
    }
    if(_element->lstValues.size() == 0)
    {
        fprintf(traceFile,"\n No column values");
    }
    else
    {
        fprintf(traceFile,"\n\n--------------------------------------------");
        fprintf(traceFile,"\n column values");
        fprintf(traceFile,"\n\n--------------------------------------------");
        for(string val : _element->lstValues)
        {
            fprintf(traceFile,"\n value:%s",val.c_str());
        }
    }
    if(_element->lstConditions.size() == 0)
    {
        fprintf(traceFile,"\n No Select conditions");
    }
    else
    {
        fprintf(traceFile,"\n\n--------------------------------------------");
        fprintf(traceFile,"\n Select Conditions");
        fprintf(traceFile,"\n\n--------------------------------------------");
        for(shared_ptr<Condition> con : _element->lstConditions)
        {
            printParts(con->name);
            fprintf(traceFile,"\n\t condition condition %s", con->condition.c_str());
            fprintf(traceFile,"\n\t condition op        %s", con->op.c_str());
            fprintf(traceFile,"\n\t condition value     %s", con->value.c_str());
        }
    }
    if(_element->lstJoinConditions.size() == 0)
    {
        fprintf(traceFile,"\n No Join conditions");
    }
    else
    {
        fprintf(traceFile,"\n\n--------------------------------------------");
        fprintf(traceFile,"\n Join Conditions");
        fprintf(traceFile,"\n\n--------------------------------------------");
        for(shared_ptr<Condition> con : _element->lstJoinConditions)
        {
            printParts(con->name);
            fprintf(traceFile,"\n\t condition condition %s", con->condition.c_str());
            fprintf(traceFile,"\n\t condition op        %s", con->op.c_str());
            fprintf(traceFile,"\n\t condition value     %s", con->value.c_str());
        }
    }
}

void printStatement(shared_ptr<Statement> _statement)
{

    if(_statement == nullptr)
    {
       fprintf(traceFile,"\n_statement is null"); 
       return;
    }
    fprintf(traceFile,"\nrow count = %i",_statement->rowsToReturn);
    if(_statement->table != nullptr)
        printTable(_statement->table);
    if(_statement->orderBy != nullptr)
        printOrderBy(_statement->orderBy);
    if(_statement->groupBy != nullptr)
        printGroupBy(_statement->groupBy);
}