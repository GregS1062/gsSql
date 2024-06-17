#include "sqlCommon.h"
#include "interfaces.h"
//#include "binding.cpp"
void printParts(std::shared_ptr<columnParts> parts)
{
    printf("\n\t name: %s alias: %s tableName: %s function: %s",parts->columnName.c_str(), parts->columnAlias.c_str(), parts->tableAlias.c_str(), parts->fuction.c_str());
}
void printAction(SQLACTION _action)
{
    switch(_action)
    {
        case SQLACTION::SELECT:
            printf("\nAction = Select");
            break;
        case SQLACTION::UPDATE:
            printf("\nAction = Update");
            break;
        case SQLACTION::DELETE:
            printf("\nAction = Delete");
            break;
        case SQLACTION::INSERT:
            printf("\nAction = Insert");
            break;
        case SQLACTION::JOIN:
            printf("\nAction = Join");
            break;
        default:
            break;
    }
}
void printTable(shared_ptr<sTable> tbl)
{
    printf("\n*******************************************");
    printf("\n Statement Table");
    printf("\n*******************************************");
    printf("\n table name %s", tbl->name.c_str());
    printf(" alias %s", tbl->alias.c_str());
    for(shared_ptr<Column> col : tbl->columns)
    {
        printf("\n\t column name %s", col->name.c_str());
        printf(" alias %s", col->alias.c_str());
        printf(" value %s", col->value.c_str());
        printf(" function %i", (int)col->functionType);
        if(col->primary)
            printf(" PRIMARY");
    }
    printf("\n\n Conditions");
    for(shared_ptr<Condition> con : tbl->conditions)
    {
        printParts(con->name);
        if(con->compareToName != nullptr)
            printParts(con->compareToName);
        printf("\n\t condition condition %s", con->condition.c_str());
        printf("\n\t condition op        %s", con->op.c_str());
        printf("\n\t condition value     %s", con->value.c_str());
    }
    printf("\n\n Indexes ");
    for(shared_ptr<sIndex> idx : tbl->indexes)
    {
        printf("\n\t index name %s", idx->name.c_str());
        printf("  file name %s", idx->fileName.c_str());
        for(shared_ptr<Column> col : idx->columns)
        {
            printf("\n\t\t index column name %s", col->name.c_str());
        }
    }
}
void printOrderBy(shared_ptr<OrderBy> _orderBy)
{
    printf("\n*******************************************");
    printf("\n Order By");
    printf("\n*******************************************");
    for(OrderOrGroup order :_orderBy->order)
    {
        printParts(order.name);
    }
    if(_orderBy->asc)
        printf("\n\tascending");
    else
        printf("\n\tdescending");
}
void printGroupBy(shared_ptr<GroupBy> _groupBy)
{
    printf("\n*******************************************");
    printf("\n Group By");
    printf("\n*******************************************");
    for(OrderOrGroup group :_groupBy->group)
    {
        printParts(group.name);
    }

    printf("\n*******************************************");
    printf("\n Having");
    printf("\n*******************************************");

    for(Condition* con :_groupBy->having)
    {
        printParts(con->name);
        printf("\n\t condition condition %s", con->condition.c_str());
        printf("\n\t condition op        %s", con->op.c_str());
        printf("\n\t condition value     %s", con->value.c_str());
    }

}
void printQuery(shared_ptr<iElements> _element)
{
    printf("\n\n\n*******************************************");
    printf("\n Print Diagnostics: Query Tables");
    printf("\n*******************************************");
    printAction(_element->sqlAction);
    printf("\nRow Count:%i",_element->rowsToReturn);
    printf("\nTable Name:%s",_element->tableName.c_str());
    if(_element->lstColumns.size() == 0)
    {
        printf("\n No column");
    }
    else
    {
        printf("\n\n--------------------------------------------");
        printf("\n column names");
        printf("\n\n--------------------------------------------");
        for(std::shared_ptr<columnParts> parts : _element->lstColumns)
        {
            printParts(parts);
        }
    }
    if(_element->lstValues.size() == 0)
    {
        printf("\n No column values");
    }
    else
    {
        printf("\n\n--------------------------------------------");
        printf("\n column values");
        printf("\n\n--------------------------------------------");
        for(string val : _element->lstValues)
        {
            printf("\n value:%s",val.c_str());
        }
    }
    if(_element->lstConditions.size() == 0)
    {
        printf("\n No Select conditions");
    }
    else
    {
        printf("\n\n--------------------------------------------");
        printf("\n Select Conditions");
        printf("\n\n--------------------------------------------");
        for(shared_ptr<Condition> con : _element->lstConditions)
        {
            printParts(con->name);
            printf("\n\t condition condition %s", con->condition.c_str());
            printf("\n\t condition op        %s", con->op.c_str());
            printf("\n\t condition value     %s", con->value.c_str());
        }
    }
    if(_element->lstJoinConditions.size() == 0)
    {
        printf("\n No Join conditions");
    }
    else
    {
        printf("\n\n--------------------------------------------");
        printf("\n Join Conditions");
        printf("\n\n--------------------------------------------");
        for(shared_ptr<Condition> con : _element->lstJoinConditions)
        {
            printParts(con->name);
            printf("\n\t condition condition %s", con->condition.c_str());
            printf("\n\t condition op        %s", con->op.c_str());
            printf("\n\t condition value     %s", con->value.c_str());
        }
    }
}

void printStatement(Statement* _statement)
{
    printTable(_statement->table);
    printOrderBy(_statement->orderBy);
    printGroupBy(_statement->groupBy);
}