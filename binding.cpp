#pragma once
#include "sqlCommon.h"
#include "interfaces.h"
#include "lookup.cpp"

/******************************************************
 * Binding
 ******************************************************/
class Binding
{
    private:

    sTable*          defaultTable;    
    sTable*          defaultSQLTable;
    tokenParser*     tok;
    iSQLTables*      isqlTables;
    iElements*       ielements;
    
    ParseResult     bindColumnList();
    ParseResult     bindFunctionColumn(columnParts*);
    sTable*         assignTable(columnParts*);
    Column*         assignTemplateColumn(columnParts*,char*);
    ParseResult     bindValueList();
    ParseResult     bindConditions();
    ParseResult     bindCondition(Condition*);
    ParseResult     bindHaving();
    ParseResult     bindOrderBy();
    ParseResult     bindGroupBy();
    ParseResult     populateTable(sTable*,sTable*);
    bool            valueSizeOutofBounds(char*, Column*);
    ParseResult     editColumn(Column*,char*);
    ParseResult     editCondition(Condition*);

    public:

    OrderBy*         orderBy    = new OrderBy();
    GroupBy*         groupBy    = new GroupBy();  
    ParseResult      bindTableList(list<char*>);
    list<sTable*>    lstTables;  //public for diagnostic purposes

    Binding(iSQLTables*);
    Statement*     bind(iElements*);
};
/******************************************************
 * Constructor
 ******************************************************/
Binding::Binding(iSQLTables* _isqlTables) 
{
    isqlTables = _isqlTables;
};
/******************************************************
 * Bind
 ******************************************************/
Statement* Binding::bind(iElements* _ielements)
{

    if(debug)
        fprintf(traceFile,"\n\n-------------------------BEGIN BIND-------------------------------------------");
    
    ielements = _ielements;

    if(lstTables.size() == 0)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,true,"bind empty table list ");
        return nullptr;
    }
    
    if(debug)
            fprintf(traceFile,"\nstatement table name %s",ielements->tableName);

    TokenPair* tp = lookup::tokenSplit(ielements->tableName,(char*)" ");

    if(tp == nullptr)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Table name is null");
        return nullptr;
    }

    Statement* statement = new Statement();
    statement->table = lookup::getTableByName(lstTables,tp->one);
    if(statement->table == nullptr)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,true,"statement table name is null");
        delete tp;
        return nullptr;
    }

    delete tp;

    statement->action = ielements->sqlAction;
    statement->rowsToReturn = ielements->rowsToReturn;


    defaultTable = statement->table;
    if(defaultTable == nullptr)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,false," Binding: unable to load default table");
        return nullptr;
    }
    defaultSQLTable = lookup::getTableByName(isqlTables->tables,defaultTable->name);
    if(defaultSQLTable == nullptr)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,false," Binding: unable to load default SQL table");
        return nullptr;
    }


    if(bindColumnList() == ParseResult::FAILURE)
    {
        if(debug)
            fprintf(traceFile,"\n column Binding failure");
        return nullptr;
    }

    if(bindValueList() == ParseResult::FAILURE)
        return nullptr;

    if(bindConditions() == ParseResult::FAILURE)
       return nullptr;

    if(bindOrderBy() == ParseResult::FAILURE)
       return nullptr;

    if(bindGroupBy() == ParseResult::FAILURE)
       return nullptr;

    return statement;
}
/******************************************************
 * Bind Table List
 ******************************************************/
ParseResult Binding::bindTableList(list<char*> _lstDeclaredTables)
{
    TokenPair* tp;
    sTable* temp;
    sTable* table;
    for(char* token : _lstDeclaredTables)
    {
        if(debug)
            fprintf(traceFile,"\n table name %s|| \n",token);

        table = new sTable();

        tp = lookup::tokenSplit(token,(char*)" ");

        if(tp == nullptr)
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Table name is null");
            return ParseResult::FAILURE;
        }

        if(debug)
            fprintf(traceFile,"\n tp1:%s| tp2:%s|",tp->one, tp->two);

        temp = lookup::getTableByName(isqlTables->tables,tp->one);
        if(temp != nullptr)
        {
            table->name         = temp->name;
            table->fileName     = temp->fileName;
            if(tp->two != nullptr)
                table->alias        = dupString(tp->two);
            table->recordLength = temp->recordLength;
            for(sIndex* idx : temp->indexes)
            {
                table->indexes.push_back(idx);
            }
            lstTables.push_back(table);
            delete tp;
            continue;
        }
    }

    if(lstTables.size() == 0)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,true," Binding expecting at least one table");
        return ParseResult::FAILURE;
    }

    return ParseResult::SUCCESS;
}
/******************************************************
 * Bind Column List
 ******************************************************/
ParseResult Binding::bindColumnList()
{
    if(debug)
        fprintf(traceFile,"\n-------------------- bind column list ----------------");
    // case Insert into table values(....)
    if(ielements->lstColumns.size() == 0)
    {
        if(populateTable(defaultTable,defaultSQLTable) == ParseResult::FAILURE)
            return ParseResult::FAILURE;
        return ParseResult::SUCCESS;
    }

    sTable* tbl;
    Column* col;
    for(columnParts* parts : ielements->lstColumns)
    {
        //Case 1: function
        if(parts->fuction != nullptr)
        {
           if(bindFunctionColumn(parts) == ParseResult::FAILURE)
                return ParseResult::FAILURE;
            continue;
        }

        //Case 2: Name = *
        if(strcmp(parts->columnName,(char*)sqlTokenAsterisk) == 0)
        {
            if(populateTable(defaultTable,defaultSQLTable) == ParseResult::FAILURE)
                return ParseResult::FAILURE;
           continue;
        }

        //Bind column to table
        tbl = assignTable(parts);
        if(tbl == nullptr)
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Cannot assign column to table: ");
            sendMessage(MESSAGETYPE::ERROR,presentationType,false,parts->fullName);
            return ParseResult::FAILURE;
        }

        col = assignTemplateColumn(parts,tbl->name);
        if(col == nullptr)
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Cannot assign column to table: ");
            sendMessage(MESSAGETYPE::ERROR,presentationType,false,parts->fullName);
            return ParseResult::FAILURE;
        }
        
        if(parts->columnAlias != nullptr)
            col->alias = parts->columnAlias;

        if(parts->value != nullptr)
        {
            if(editColumn(col,parts->value) == ParseResult::FAILURE)
                return ParseResult::FAILURE;
            col->value = parts->value;
        }
        tbl->columns.push_back(col);  
    }
    return ParseResult::SUCCESS;
}

/******************************************************
 * Assign Table
 ******************************************************/
sTable* Binding::assignTable(columnParts* _parts)
{
    sTable* tbl;
    
    if(_parts->tableAlias != nullptr)
    {
        // case 1:  theTable.theColumn  prefix is table name
        tbl = lookup::getTableByName(lstTables,_parts->tableAlias);
        if(tbl != nullptr)
            return tbl;

        // case 2: t.theColumn  prefix is an alias
        tbl = lookup::getTableByAlias(lstTables,_parts->tableAlias);
        if(tbl != nullptr)
            return tbl;
    }

    // case 3: order by and group by can reference a column alias   tax AS ripoff   order by ripoff
    Column* col = lookup::getColumnByAlias(defaultTable->columns,_parts->columnName);
    if(col != nullptr)
        return defaultTable;

    // case 4: order by and group by can reference a function   count(*)   order by count
    tbl = lookup::getTableByName(isqlTables->tables,(char*)sqlTokenFumctionTable);
    
    if(tbl == nullptr)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Cannot retrieve function table: ");
        return nullptr;
    }

    col = lookup::getColumnByName(tbl->columns,_parts->columnName);
    if(col != nullptr)
        return tbl;

    // case 5: Most common form of all  theColumn No table reference/no table alias
    return defaultTable;
}
/******************************************************
 * Assign Template Column
 ******************************************************/
Column*  Binding::assignTemplateColumn(columnParts* _parts,char* _tableName)
{
    sTable* tbl = lookup::getTableByName(isqlTables->tables, _tableName);
    if(tbl == nullptr)
        return nullptr;

    Column* col;
    Column* newCol = new Column();
    col = lookup::getColumnByName(tbl->columns,_parts->columnName);

    if(col == nullptr)
        col = lookup::getColumnByAlias(tbl->columns,_parts->columnName);

    if(col == nullptr)
        return nullptr;

    newCol->edit        = col->edit;
    newCol->length      = col->length;
    newCol->position    = col->position;
    newCol->name        = col->name;
    newCol->primary     = col->primary;
    newCol->tableName   = col->tableName;
    return newCol;
};
/******************************************************
 * Bind Function Columns
 ******************************************************/
 ParseResult Binding::bindFunctionColumn(columnParts* _parts)
{
    if(debug)
        fprintf(traceFile,"\n------------------bind function------------");

    t_function ag = t_function::NONE;

    if(lookup::findDelimiter(_parts->fuction,(char*)sqlTokenCount) > NEGATIVE)
        ag = t_function::COUNT;
    else
        if(lookup::findDelimiter(_parts->fuction,(char*)sqlTokenSum) > NEGATIVE)
            ag = t_function::SUM;
        else
            if(lookup::findDelimiter(_parts->fuction,(char*)sqlTokenMax) > NEGATIVE)
                ag = t_function::MAX;
            else
                if(lookup::findDelimiter(_parts->fuction,(char*)sqlTokenMin) > NEGATIVE)
                    ag = t_function::MIN;
                else
                    if(lookup::findDelimiter(_parts->fuction,(char*)sqlTokenAvg) > NEGATIVE)
                        ag = t_function::AVG;


    if(ag == t_function::NONE)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,true,"unable to identify function: ");
        sendMessage(MESSAGETYPE::ERROR,presentationType,false,_parts->fuction);
        return ParseResult::FAILURE;
    }

    //  Create a new column
    Column* col = new Column();
    if(ag == t_function::COUNT)
    {
       if(lookup::findDelimiter(_parts->columnName,(char*)sqlTokenAsterisk) > NEGATIVE)
       {
            col->functionType = t_function::COUNT;
            col->name = (char*)"COUNT";
            col->edit   = t_edit::t_int;
            if(_parts->columnAlias != nullptr)
                col->alias  = _parts->columnAlias;
            defaultTable->columns.push_front(col);
            return ParseResult::SUCCESS;
       }
    }

    // all other function function must reference a column
    // Function pattern looks like this: FUNC(column table alias/column name) AS alias
    
    //Bind column to table
    sTable* tbl = assignTable(_parts);
    if(tbl == nullptr)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Cannot assign column table: ");
        sendMessage(MESSAGETYPE::ERROR,presentationType,false,_parts->fullName);
        return ParseResult::FAILURE;
    }

    col = assignTemplateColumn(_parts,tbl->name);
    if(col == nullptr)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Cannot find template column: ");
        sendMessage(MESSAGETYPE::ERROR,presentationType,false,_parts->fullName);
        return ParseResult::FAILURE;
    }
    
    col->functionType = ag;

    char* altAlias{};
    switch(ag)
    {
        case t_function::AVG:
            altAlias = (char*)sqlTokenAvg;
            break;
        case t_function::COUNT:
            altAlias = (char*)sqlTokenCount;
            break;
        case t_function::MAX:
            altAlias = (char*)sqlTokenMax;
            break;
        case t_function::MIN:
            altAlias = (char*)sqlTokenMin;
            break;
        case t_function::SUM:
            altAlias = (char*)sqlTokenSum;
            break;
        case t_function::NONE:
            fprintf(traceFile,"\nCounld not find function type");
            return ParseResult::FAILURE;
    }
    if(_parts->columnAlias)
        col->alias = _parts->columnAlias;

    if(col->alias == nullptr)
        col->alias = altAlias;

    tbl = lookup::getTableByName(lstTables,col->tableName);
    
    tbl->columns.push_back(col);

    return ParseResult::SUCCESS;
}; 

/******************************************************
 * Bind Value List
 ******************************************************/
ParseResult Binding::bindValueList()
{
    if(ielements->lstValues.size() == 0)
        return ParseResult::SUCCESS;

    //TODO account for case: update customer c OR insert into customer c ....select..
    sTable* table = lstTables.front();
    if(table == nullptr)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,true,"bind value list table null ");
        return ParseResult::FAILURE;
    }
    if(table->columns.size() != ielements->lstValues.size())
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,true,"The count of columns ");
        sendMessage(MESSAGETYPE::ERROR,presentationType,false,(char*)std::to_string(table->columns.size()).c_str());
        sendMessage(MESSAGETYPE::ERROR,presentationType,false," and values ");
        sendMessage(MESSAGETYPE::ERROR,presentationType,false,(char*)std::to_string(ielements->lstValues.size()).c_str());
        sendMessage(MESSAGETYPE::ERROR,presentationType,false," do not match ");
        return ParseResult::FAILURE;
    }

    for(Column* col : table->columns)
    {
        if(editColumn(col,ielements->lstValues.front()) == ParseResult::FAILURE)
            return ParseResult::FAILURE;
        ielements->lstValues.pop_front();
    }
    return ParseResult::SUCCESS;
}
/******************************************************
 * Populate table
 ******************************************************/
ParseResult Binding::populateTable(sTable* _table,sTable* _sqlTbl)
{
    if(debug)
        fprintf(traceFile,"\n-------------------- Populate table  ----------------");
    for (Column* col : _sqlTbl->columns) 
    {
        _table->columns.push_back(col);
    }
    return ParseResult::SUCCESS;
}
/******************************************************
 * Value Size Out Of Bounds
 ******************************************************/
bool Binding::valueSizeOutofBounds(char* value, Column* col)
{
    if(value == nullptr)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,false,col->name);
        sendMessage(MESSAGETYPE::ERROR,presentationType,false," : value is null ");
        return true;
    }

    if(col == nullptr)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,false," cannot find column for ");
        sendMessage(MESSAGETYPE::ERROR,presentationType,false,value);
        sendMessage(MESSAGETYPE::ERROR,presentationType,false," ");
        return true;
    }

    if(strlen(value) > (size_t)col->length
    && col->edit == t_edit::t_char)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,false,col->name);
        sendMessage(MESSAGETYPE::ERROR,presentationType,false," value length ");
        sendMessage(MESSAGETYPE::ERROR,presentationType,false,(char*)std::to_string(strlen(value)).c_str());
        sendMessage(MESSAGETYPE::ERROR,presentationType,false," > edit length ");
        sendMessage(MESSAGETYPE::ERROR,presentationType,false,(char*)std::to_string(col->length).c_str());
        sendMessage(MESSAGETYPE::ERROR,presentationType,false," ");
        return true;
    }

    //value is okay
    return false;
}
/******************************************************
 * Bind Conditions
 ******************************************************/
ParseResult Binding::bindConditions()
{
    sTable* tbl;
    for(Condition* con : ielements->lstConditions)
    {
        if(bindCondition(con) == ParseResult::FAILURE)
            return ParseResult::FAILURE;
        tbl = lookup::getTableByName(lstTables,con->col->tableName);
        tbl->conditions.push_back(con);
    }
     
    return ParseResult::SUCCESS;
}
/******************************************************
 * Bind Condition
 ******************************************************/
ParseResult Binding::bindCondition(Condition* _con)
{
    sTable* tbl;
    Column* col;
    if(_con->name == nullptr)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Condition column name is null ");
        return ParseResult::FAILURE;
    }

    //Bind column to table
    tbl = assignTable(_con->name);
    if(tbl == nullptr)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Cannot find table");
        sendMessage(MESSAGETYPE::ERROR,presentationType,false,_con->name->fullName);
        return ParseResult::FAILURE;
    }

    col = assignTemplateColumn(_con->name,tbl->name);
    if(col == nullptr)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Cannot find column name");
        sendMessage(MESSAGETYPE::ERROR,presentationType,false,_con->name->fullName);
        return ParseResult::FAILURE;
    }

    _con->col = col;
    _con->col->tableName = tbl->name;
    if(editCondition(_con) == ParseResult::FAILURE)
        return ParseResult::FAILURE;

    return ParseResult::SUCCESS;
}
/******************************************************
 * Bind Order By
 ******************************************************/
ParseResult Binding::bindOrderBy()
{
    if(debug)
        fprintf(traceFile,"\n----------------------- bind order by -----------------------------------");
    sTable* tbl;
    Column* col;

    if (ielements->orderBy == nullptr)
    {
        orderBy = nullptr;
        return ParseResult::SUCCESS;
    }
        

    orderBy->asc = ielements->orderBy->asc;
    
    for(OrderOrGroup order : ielements->orderBy->order)
    {
        //Bind column to table
        tbl = assignTable(order.name);
        if(tbl == nullptr)
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Cannot bind Order By column to table: ");
            sendMessage(MESSAGETYPE::ERROR,presentationType,false,order.name->fullName);
            return ParseResult::FAILURE;
        }

        col = assignTemplateColumn(order.name,tbl->name);
        if(col == nullptr)
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Cannot bind Order By template column to table: ");
            sendMessage(MESSAGETYPE::ERROR,presentationType,false,order.name->fullName);
            return ParseResult::FAILURE;
        }
        int columnNbr   = NEGATIVE;
        int count       = 0;
        for(Column* column : tbl->columns)
        {
            if(strcasecmp(column->name, col->name) == 0)
            {
                columnNbr = count;
                break;
            }
            count++;
        }
        if(columnNbr == NEGATIVE)
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Could not find order by column in the list of reporting columns ");
            return ParseResult::FAILURE;
        }
        order.columnNbr = columnNbr;
        order.col = col;

        orderBy->order.push_back(order);
        
        if(debug)
            fprintf(traceFile,"\n bind order Column name:%s table: %s  sort# %d",col->name, col->tableName, order.columnNbr);
        
    }
    return ParseResult::SUCCESS;
}
/******************************************************
 * Bind Group By
 ******************************************************/
ParseResult Binding::bindGroupBy()
{
    if(debug)
        fprintf(traceFile,"\n----------------------- bind group by -----------------------------------");

    if(ielements->groupBy == nullptr)
    {
        groupBy = nullptr;
        return ParseResult::SUCCESS;
    }

    sTable* tbl;
    Column* col;

    for(OrderOrGroup group : ielements->groupBy->group)
    {
        //Bind column to table
        tbl = assignTable(group.name);
        if(tbl == nullptr)
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Cannot bind Group By column to table: ");
            sendMessage(MESSAGETYPE::ERROR,presentationType,false,group.name->fullName);
            return ParseResult::FAILURE;
        }

        col = assignTemplateColumn(group.name,tbl->name);
        if(col == nullptr)
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Cannot bind Group By template column to table: ");
            sendMessage(MESSAGETYPE::ERROR,presentationType,false,group.name->fullName);
            return ParseResult::FAILURE;
        }
        int columnNbr   = NEGATIVE;
        int count       = 0;
        for(Column* column : tbl->columns)
        {
            if(strcasecmp(column->name, col->name) == 0)
            {
                columnNbr = count;
                break;
            }
            count++;
        }
        if(columnNbr == NEGATIVE)
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Could not find group by column in the list of reporting columns ");
            return ParseResult::FAILURE;
        }
        group.columnNbr = columnNbr;
        group.col = col;
        groupBy->group.push_back(group);
        

        if(debug)
            fprintf(traceFile,"\n bind group Column name:%s table: %s  sort# %d",col->name, col->tableName, group.columnNbr);
        
        bindHaving();
    }

    return ParseResult::SUCCESS;
}

/******************************************************
 * bind Having
 ******************************************************/
ParseResult Binding::bindHaving()
{

    //  Condition must apply to an item in the report line

    Column* col;
    
    // case 1: group by zipcode Having Count(*) > 10                    Count   = function
    // case 2: group by order Having Max(tax) < 100                     tax     = column name
    // case 3: Max(tax) as maxtax group by order Having maxtax > 50     maxtax  = column alias
    
    for(Condition* con : ielements->lstHavingConditions)
    {
        col = lookup::getColumnByName(defaultTable->columns,con->name->columnName);
        
        // Notice what is going on here. Since a HAVING statement does not have an alias, we test the HAVING column name against the default column alias.
        if(col == nullptr)
            col = lookup::getColumnByAlias(defaultTable->columns,con->name->columnName);
        
        if(col == nullptr)
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Cannot find 'Having' column ");
            return ParseResult::FAILURE;
        }
        con->col = col;
        if( editCondition(con) == ParseResult::FAILURE)
            return ParseResult::FAILURE;
        groupBy->having.push_back(con);
    }
    return ParseResult::SUCCESS;
}

/******************************************************
 * edit column
 ******************************************************/
ParseResult Binding::editColumn(Column* _col,char* _value)
{
    if(_col == nullptr)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,true,"edit column null ");
        return ParseResult::FAILURE;
    }

    if(_value == nullptr)
        return ParseResult::SUCCESS;

    switch(_col->edit)
    {
        case t_edit::t_bool: 
        {
            if(strcasecmp(_value,"T") == 0
            || strcasecmp(_value,"F") == 0)
                _col->value = _value;
            else
            {
                fprintf(traceFile,"\n bool value %s",_value);
                return ParseResult::FAILURE;
            }
            break; 
        }  
        case t_edit::t_char:    //do nothing
        {
            if(valueSizeOutofBounds(_value,_col))
                return ParseResult::FAILURE;
            _col->value = _value;
            break; 
        }  
        case t_edit::t_date:    //do nothing
        {
            if(!isDateValid(_col->value))
                return ParseResult::FAILURE;
            _col->value = _value;
            break; 
        } 
        case t_edit::t_double:
        {
            if(!isNumeric(_value))
            {
                sendMessage(MESSAGETYPE::ERROR,presentationType,false," column ");
                sendMessage(MESSAGETYPE::ERROR,presentationType,false,_col->name); 
                sendMessage(MESSAGETYPE::ERROR,presentationType,false,"  value not numeric |");
                sendMessage(MESSAGETYPE::ERROR,presentationType,false,_value);
                sendMessage(MESSAGETYPE::ERROR,presentationType,false,"| ");
                return ParseResult::FAILURE;
            }
            _col->value = _value;
            break;
        }
        case t_edit::t_int:
        {
            if(!isNumeric(_value))
            {
                sendMessage(MESSAGETYPE::ERROR,presentationType,false," column ");
                sendMessage(MESSAGETYPE::ERROR,presentationType,false,_col->name); 
                sendMessage(MESSAGETYPE::ERROR,presentationType,false,"  value not numeric |");
                sendMessage(MESSAGETYPE::ERROR,presentationType,false,_value);
                sendMessage(MESSAGETYPE::ERROR,presentationType,false,"| ");
                return ParseResult::FAILURE;
            }
            _col->value = _value;
            break;
        }
    }
    return ParseResult::SUCCESS;
}
/******************************************************
 * Edit Condition Column
 ******************************************************/
ParseResult Binding::editCondition(Condition* _con)
{
    if(_con == nullptr)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,true,"condition edit is null ");
        return ParseResult::FAILURE;
    }

    if(_con->value == nullptr
    && _con->compareToName != nullptr)
       return ParseResult::SUCCESS;

    switch(_con->col->edit)
    {
        case t_edit::t_bool:    //do nothing
        {
            break; 
        }  
        case t_edit::t_char:    //do nothing
        {

            _con->value   = _con->value;
            break; 
        }  
        case t_edit::t_date:    //do nothing
        {
            if(!isDateValid(_con->value))
                return ParseResult::FAILURE;

            _con->dateValue = parseDate(_con->value);
            break; 
        } 
        case t_edit::t_double:
        {
            if(!isNumeric(_con->value))
            {
                sendMessage(MESSAGETYPE::ERROR,presentationType,false," condition column ");
                sendMessage(MESSAGETYPE::ERROR,presentationType,false,_con->name->fullName); 
                sendMessage(MESSAGETYPE::ERROR,presentationType,false,"  value not numeric ");
                sendMessage(MESSAGETYPE::ERROR,presentationType,false,_con->value);
                sendMessage(MESSAGETYPE::ERROR,presentationType,false," ");
                return ParseResult::FAILURE;
            }
            _con->doubleValue = atof(_con->value);
            break;
        }
        case t_edit::t_int:
        {
            if(!isNumeric(_con->value))
            {
                sendMessage(MESSAGETYPE::ERROR,presentationType,false," condition column ");
                sendMessage(MESSAGETYPE::ERROR,presentationType,false,_con->name->fullName); 
                sendMessage(MESSAGETYPE::ERROR,presentationType,false,"  value not numeric |");
                sendMessage(MESSAGETYPE::ERROR,presentationType,false,_con->value);
                sendMessage(MESSAGETYPE::ERROR,presentationType,false,"| ");
                return ParseResult::FAILURE;
            }
            _con->intValue = atoi(_con->value);
            break;
        }
    }
    return ParseResult::SUCCESS;
}