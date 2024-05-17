#include "sqlCommon.h"
#include "interfaces.h"
#include "binding.cpp"
void printTable(sTable* tbl)
{
    fprintf(traceFile,"\n*******************************************");
    fprintf(traceFile,"\n Statement Table");
    fprintf(traceFile,"\n*******************************************");
    fprintf(traceFile,"\n table name %s", tbl->name);
    fprintf(traceFile," alias %s", tbl->alias);
    for(Column* col : tbl->columns)
    {
        fprintf(traceFile,"\n\t column name %s", col->name);
        fprintf(traceFile," alias %s", col->tableName);
        fprintf(traceFile," value %s", col->value);
        if(col->primary)
            fprintf(traceFile," PRIMARY");
    }
    fprintf(traceFile,"\n\n Conditions");
    for(Condition* con : tbl->conditions)
    {
        fprintf(traceFile,"\n\n\t condition name      %s", con->name);
        fprintf(traceFile,"\n\t condition condition %s", con->condition);
        fprintf(traceFile,"\n\t condition op        %s", con->op);
        fprintf(traceFile,"\n\t condition value     %s", con->value);
    }
    for(sIndex* idx : tbl->indexes)
    {
        fprintf(traceFile,"\n\t index name %s", idx->name);
        fprintf(traceFile,"  file name %s", idx->fileName);
        for(Column* col : idx->columns)
        {
            fprintf(traceFile,"\n\t\t index column name %s", col->name);
        }
    }
}
void printOrderBy(OrderBy* _orderBy)
{
    fprintf(traceFile,"\n*******************************************");
    fprintf(traceFile,"\n Order By");
    fprintf(traceFile,"\n*******************************************");
    for(OrderOrGroup order :_orderBy->order)
    {
        fprintf(traceFile,"\n\t\t index column name %s", order.name);
    }
    if(_orderBy->asc)
        fprintf(traceFile,"\n\tascending");
    else
        fprintf(traceFile,"\n\tdescending");
}
void printGroupBy(GroupBy* _groupBy)
{
    fprintf(traceFile,"\n*******************************************");
    fprintf(traceFile,"\n Order By");
    fprintf(traceFile,"\n*******************************************");
    for(OrderOrGroup group :_groupBy->group)
    {
        fprintf(traceFile,"\n\t\t index column name %s", group.name);
    }
}
void printQuery(iElements* _it,Binding* bind)
{
    fprintf(traceFile,"\n*******************************************");
    fprintf(traceFile,"\n Query Tables");
    fprintf(traceFile,"\n*******************************************");
    for(sTable* tbl : bind->lstTables)
    {
        printTable(tbl);
    }
    return;
    fprintf(traceFile,"\n\n--------------------------------------------");
    fprintf(traceFile,"\n column names");
    fprintf(traceFile,"\n\n--------------------------------------------");
    for(char* c : _it->lstColName)
    {
        fprintf(traceFile,"\n column:%s",c);
    }
    fprintf(traceFile,"\n\n--------------------------------------------");
    fprintf(traceFile,"\n column values");
    fprintf(traceFile,"\n\n--------------------------------------------");
    for(char* c : _it->lstValues)
    {
        fprintf(traceFile,"\n value:%s",c);
    }
    fprintf(traceFile,"\n\n--------------------------------------------");
    fprintf(traceFile,"\n Column Name Values");
    fprintf(traceFile,"\n\n--------------------------------------------");
    for(ColumnNameValue* nv : _it->lstColNameValue)
    {
        fprintf(traceFile,"\n column:%s value:%s",nv->name,nv->value);
    }
    if(_it->lstConditions.size() == 0)
    {
        fprintf(traceFile,"\n No conditions");
        return;
    }
    fprintf(traceFile,"\n\n--------------------------------------------");
    fprintf(traceFile,"\n Conditions");
    fprintf(traceFile,"\n\n--------------------------------------------");
    for(Condition* con : _it->lstConditions)
    {
        fprintf(traceFile,"\n\t condition name      %s", con->name);
        fprintf(traceFile,"\n\t condition condition %s", con->condition);
        fprintf(traceFile,"\n\t condition op        %s", con->op);
        fprintf(traceFile,"\n\t condition value     %s", con->value);
    }
}

void printStatement(Statement* _statement)
{
    printTable(_statement->table);
    printOrderBy(_statement->orderBy);
    printGroupBy(_statement->groupBy);
}