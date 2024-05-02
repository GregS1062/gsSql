#pragma once
#include "sqlCommon.h"
#include "interfaces.h"
#include "lookup.cpp"
class Binding
{
    private:

    sTable*          defaultTable;    
    sTable*          defaultSQLTable;   
    tokenParser*     tok;
    
    ParseResult     bindTableList();
    ParseResult     bindColumnList();
    ParseResult     bindColumnValueList();
    ParseResult     bindColumn(char*);
    Column*         resolveColumn(TokenPair*);
    ParseResult     bindNonAliasedColumn(char*, char*);
    ParseResult     bindAliasedColumn(TokenPair*, char*);
    ParseResult     bindValueList();
    ParseResult     bindConditions();
    ParseResult     bindOrderBy();
    ParseResult     bindGroupBy();
    ParseResult     populateTable(sTable*,sTable*);
    bool            valueSizeOutofBounds(char*, Column*);
    ParseResult     editColumn(Column*,char*);
    ParseResult     editCondition(Condition*,char*);

    public:

    iSQLTables*      isqlTables;
    iElements*       ielements;
    list<Statement*> lstStatements;
    OrderBy*         orderBy    = new OrderBy();
    GroupBy*         groupBy    = new GroupBy();  

    list<sTable*>    lstTables;  //public for diagnostic purposes

    Binding();
    ParseResult     bind();
};
/******************************************************
 * Constructor
 ******************************************************/
Binding::Binding() 
{

};
/******************************************************
 * Bind
 ******************************************************/
ParseResult Binding::bind()
{

    if(debug)
        fprintf(traceFile,"\n\n-------------------------BEGIN BIND-------------------------------------------");
    
    Statement* statement;

    if(bindTableList() == ParseResult::FAILURE)
        return ParseResult::FAILURE;

    if(lstTables.size() == 0)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,true,"bind empty table list ");
        return ParseResult::FAILURE;
    }

    for(sTable* tbl : lstTables)
    {
         statement = new Statement();
         statement->table = tbl;
         statement->action = ielements->sqlAction;
         statement->rowsToReturn = ielements->rowsToReturn;
         lstStatements.push_back(statement);
    }

    if(lstTables.size() == 1)
    {
        defaultTable = lstTables.front();
        if(defaultTable == nullptr)
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,false," Binding: unable to load default table");
            return ParseResult::FAILURE;
        }
        defaultSQLTable = lookup::getTableByName(isqlTables->tables,defaultTable->name);
        if(defaultSQLTable == nullptr)
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,false," Binding: unable to load default SQL table");
            return ParseResult::FAILURE;
        }
    }


    if(bindColumnList() == ParseResult::FAILURE)
    {
        if(debug)
            fprintf(traceFile,"\n column Binding failure");
        return ParseResult::FAILURE;
    }

    if(bindValueList() == ParseResult::FAILURE)
        return ParseResult::FAILURE;

    if(bindColumnValueList() == ParseResult::FAILURE)
        return ParseResult::FAILURE;

    if(bindConditions() == ParseResult::FAILURE)
       return ParseResult::FAILURE;

    if(bindOrderBy() == ParseResult::FAILURE)
       return ParseResult::FAILURE;

    if(bindGroupBy() == ParseResult::FAILURE)
       return ParseResult::FAILURE;

    return ParseResult::SUCCESS;
}
/******************************************************
 * Bind Table List
 ******************************************************/
ParseResult Binding::bindTableList()
{
    TokenPair* tp;
    sTable* temp;
    sTable* table;
    for(char* token : ielements->lstTables)
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
            table->alias        = tp->two;;
            table->recordLength = temp->recordLength;
            for(sIndex* idx : temp->indexes)
            {
                table->indexes.push_back(idx);
            }
            lstTables.push_back(table);
            continue;
        }

        //No valid table name at this point
        if(tp->two == nullptr)
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true," invalid table name:");
            sendMessage(MESSAGETYPE::ERROR,presentationType,false,tp->one);
            return ParseResult::FAILURE;
        }

        //The valid table name must be tp->two and tp->one is the alias
        temp = lookup::getTableByName(isqlTables->tables,tp->two);
        if(temp != nullptr)
        {
            table->name         = temp->name;
            table->fileName     = temp->fileName;
            table->alias        = tp->one;
            table->recordLength = temp->recordLength;
            for(sIndex* idx : temp->indexes)
            {
                table->indexes.push_back(idx);
            }
            lstTables.push_back(table);
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
 * Bind Column
 ******************************************************/
ParseResult Binding::bindColumn(char* colName)
{
    TokenPair*  tp;
    tp = lookup::tokenSplit(colName,(char*)".");
    if(tp == nullptr)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Column name is null");
        return ParseResult::FAILURE;
    }

    if(tp->two == nullptr)
    {
        if(bindNonAliasedColumn(tp->one,(char*)"") == ParseResult::FAILURE)
            return ParseResult::FAILURE;
    }
    else
    if(bindAliasedColumn(tp, (char*)"") == ParseResult::FAILURE)
            return ParseResult::FAILURE;
    
    return ParseResult::SUCCESS;
}
/******************************************************
 * Bind Column List
 ******************************************************/
ParseResult Binding::bindColumnList()
{
    //case Insert into table values(....)
    if(ielements->lstColName.size() == 0
    && ielements->lstColNameValue.size() == 0)
    {
        if(populateTable(defaultTable,defaultSQLTable) == ParseResult::FAILURE)
            return ParseResult::FAILURE;
        return ParseResult::SUCCESS;
    }

    for(char* token : ielements->lstColName)
    {
        if(debug)
            fprintf(traceFile,"\n Binding column %s",token);

        if(bindColumn(token) == ParseResult::FAILURE)
            return ParseResult::FAILURE;
    }
    return ParseResult::SUCCESS;
}
/******************************************************
 * Bind Column Value List
 ******************************************************/
ParseResult Binding::bindColumnValueList()
{
    TokenPair*  tp;
    if(debug)
    {
        fprintf(traceFile,"\n\n-----------Bind Column Value List----------------");
    }
    for(ColumnNameValue* cv : ielements->lstColNameValue)
    {
        if(debug)
        {
          fprintf(traceFile,"\ncolumn name:%s",cv->name);  
          fprintf(traceFile," value:%s",cv->value);
        }
        tp = lookup::tokenSplit(cv->name,(char*)".");
        if(tp == nullptr)
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Column name/value is null");
            return ParseResult::FAILURE;
        }
        if(tp->two == nullptr)
        {
            if(bindNonAliasedColumn(tp->one,cv->value) == ParseResult::FAILURE)
                return ParseResult::FAILURE;
        }
        else
        if(bindAliasedColumn(tp, cv->value) == ParseResult::FAILURE)
                return ParseResult::FAILURE;
    }
    return ParseResult::SUCCESS;
}
/******************************************************
 * Bind Non-Aliased Column
 ******************************************************/
ParseResult Binding::bindNonAliasedColumn(char* _name, char* _value)
{
    //Since there is no alias, the default table is assumed

    if(strcmp(_name,(char*)sqlTokenAsterisk) == 0)
    {
        if(populateTable(defaultTable,defaultSQLTable) == ParseResult::FAILURE)
            return ParseResult::FAILURE;
        return ParseResult::SUCCESS;
    }

    Column* col = lookup::getColumnByName(defaultSQLTable->columns,_name);
    if(col == nullptr)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,true,"looking for column name:");
        sendMessage(MESSAGETYPE::ERROR,presentationType,false,_name);
        return ParseResult::FAILURE;
    }
    if(strlen(_value) > 0)
        if(editColumn(col,_value) == ParseResult::FAILURE)
            return ParseResult::FAILURE;
    fprintf(traceFile,"\nbound column name:%s",col->name);
    fprintf(traceFile," value:%s",col->value);

    defaultTable->columns.push_back(col);

    return ParseResult::SUCCESS;
}
/******************************************************
 * Resolve Column Alias
 ******************************************************/
Column* Binding::resolveColumn(TokenPair* tp)
{
    ColumnAlias* ca = new ColumnAlias();
    sTable* temp1;
    sTable* temp2;
    Column* col;
    if(tp == nullptr)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,true,"resolveColumn tokenPair is null ");
        return nullptr;
    }

    if(tp->two == nullptr)
    {
        return lookup::getColumnByName(defaultSQLTable->columns,tp->one);
    }

    temp1 = lookup::getTableByAlias(lstTables,tp->one);
    temp2 = lookup::getTableByAlias(lstTables,tp->two);
    if(temp1 == nullptr
    && temp2 == nullptr)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,true,"looking for table by alias:");
        sendMessage(MESSAGETYPE::ERROR,presentationType,false,tp->one);
        sendMessage(MESSAGETYPE::ERROR,presentationType,false," or ");
        sendMessage(MESSAGETYPE::ERROR,presentationType,false,tp->two);
        sendMessage(MESSAGETYPE::ERROR,presentationType,false," ");
        return nullptr;
    }
    if(temp1 != nullptr)
    {
        ca->tableName = temp1->name;
        ca->columnName = tp->two;
    }
    else
    {
        ca->tableName = temp2->name;
        ca->columnName = tp->one;
    }
    sTable* lstTbl;
    sTable* sqlTbl;

    lstTbl = lookup::getTableByName(lstTables,ca->tableName);
    if(lstTbl == nullptr)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,true,"looking for table name: ");
        sendMessage(MESSAGETYPE::ERROR,presentationType,false,ca->tableName);
        sendMessage(MESSAGETYPE::ERROR,presentationType,false," ");
        return nullptr;
    }
    sqlTbl = lookup::getTableByName(isqlTables->tables,ca->tableName);
    if(sqlTbl == nullptr)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,true,"looking for table name: ");
        sendMessage(MESSAGETYPE::ERROR,presentationType,false,ca->tableName);
        sendMessage(MESSAGETYPE::ERROR,presentationType,false," ");
        return nullptr;
    }

    if(strcasecmp(ca->columnName,(char*)sqlTokenAsterisk) == 0 )
    {
        col             = new Column();
        col->name       = ca->columnName;
        col->tableName  = ca->tableName;
        return col;
    }

    col = lookup::getColumnByName(sqlTbl->columns,ca->columnName);
    if(col == nullptr)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,true,ca->columnName);
        sendMessage(MESSAGETYPE::ERROR,presentationType,false," not found ");
        return nullptr;  
    }
    col->tableName  = ca->tableName;
    return col;
};
/******************************************************
 * Bind Non-Aliased Column
 ******************************************************/
ParseResult Binding::bindAliasedColumn(TokenPair* tp, char* _value)
{
    sTable* lstTbl;
    sTable* sqlTbl;
    Column* col;
    
    col = resolveColumn(tp);
    if(col == nullptr)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,true," cannot resolve alias table name ");
        return ParseResult::FAILURE;
    }

    lstTbl = lookup::getTableByName(lstTables,col->tableName);
    if(lstTbl == nullptr)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,true,"bind alias column table list null ");
        return ParseResult::FAILURE;
    }

    if(strcmp(col->name,(char*)sqlTokenAsterisk) == 0)
    {
        //Since there is no alias, a single table is assumed
        sqlTbl = lookup::getTableByName(isqlTables->tables,col->tableName);
        if(sqlTbl == nullptr)
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"looking for table name:");
            sendMessage(MESSAGETYPE::ERROR,presentationType,false,lstTbl->name);
            sendMessage(MESSAGETYPE::ERROR,presentationType,false," ");
            return ParseResult::FAILURE;
        }
        if(populateTable(lstTbl,sqlTbl) == ParseResult::FAILURE)
            return ParseResult::FAILURE;
        return ParseResult::SUCCESS;
    }

    if(editColumn(col,_value) == ParseResult::FAILURE)
        return ParseResult::FAILURE;

    lstTbl->columns.push_back(col);
    return ParseResult::SUCCESS;
}

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
    TokenPair* tp;
    Column* col;
    for(Condition* con : ielements->lstConditions)
    {
        tp = lookup::tokenSplit(con->name,(char*)".");
        if(tp == nullptr)
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Condition column name is null ");
            return ParseResult::FAILURE;
        }
        col = resolveColumn(tp);
        if(col == nullptr)
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"looking for column name: ");
            sendMessage(MESSAGETYPE::ERROR,presentationType,false,con->name);
            sendMessage(MESSAGETYPE::ERROR,presentationType,false," ");
            return ParseResult::FAILURE;
        }

        if(tp->two == nullptr)
        {
            con->col = col;
            if(editCondition(con,con->value) == ParseResult::FAILURE)
                return ParseResult::FAILURE;

            defaultTable->conditions.push_back(con);
            continue;
        }
        //--------------------------------------
        // case alias.col
        //--------------------------------------

        con->col  = col;
        sTable* lstTbl = lookup::getTableByName(lstTables,col->tableName);
        if(lstTbl == nullptr)
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"bind alias table name not found: ");
             sendMessage(MESSAGETYPE::ERROR,presentationType,false,col->tableName);
             sendMessage(MESSAGETYPE::ERROR,presentationType,false," ");
        }
        con->col->tableName = lstTbl->name;
        if(editCondition(con,con->value) == ParseResult::FAILURE)
            return ParseResult::FAILURE;

        lstTbl->conditions.push_back(con);
    }
     
    return ParseResult::SUCCESS;
}
/******************************************************
 * Bind Order By
 ******************************************************/
ParseResult Binding::bindOrderBy()
{
    if(debug)
        fprintf(traceFile,"\n----------------------- bind order by -----------------------------------");

    TokenPair* tp;
    Column* col;

    if (ielements->orderBy == nullptr)
        return ParseResult::SUCCESS;

    for(OrderOrGroup order : ielements->orderBy->order)
    {

        tp = lookup::tokenSplit(order.name,(char*)".");
        col = resolveColumn(tp);
        if(col == nullptr)
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Order by: Cannot find valid table name to sort on ");
            return ParseResult::FAILURE;
        }
        sTable* lstTable = lookup::getTableByName(lstTables,col->tableName);
        if(lstTable == nullptr)
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Order by: Cannot find table to sort on ");
            return ParseResult::FAILURE;
        }
        int columnNbr   = NEGATIVE;
        int count       = 0;
        for(Column* column : lstTable->columns)
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
        return ParseResult::SUCCESS;

    TokenPair* tp;
    Column* col;

    for(OrderOrGroup group : ielements->groupBy->group)
    {
        tp = lookup::tokenSplit(group.name,(char*)".");
        col = resolveColumn(tp);
        if(col == nullptr)
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Group by: Cannot resolve column name. See:");
            sendMessage(MESSAGETYPE::ERROR,presentationType,false,group.name);
            return ParseResult::FAILURE;
        }
        sTable* lstTable = lookup::getTableByName(lstTables,col->tableName);
        if(lstTable == nullptr)
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Group by: Cannot find table to group on. See:");
            sendMessage(MESSAGETYPE::ERROR,presentationType,false,col->tableName);
            return ParseResult::FAILURE;
        }
        int columnNbr   = NEGATIVE;
        int count       = 0;
        for(Column* column : lstTable->columns)
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
        group.name = col->name;
        groupBy->group.push_back(group);
        sendMessage(MESSAGETYPE::INFORMATION,presentationType,true," group by column: ");
        sendMessage(MESSAGETYPE::INFORMATION,presentationType,false,col->name);
        sendMessage(MESSAGETYPE::INFORMATION,presentationType,true," ");

        if(debug)
            fprintf(traceFile,"\n bind group Column name:%s table: %s  sort# %d",col->name, col->tableName, group.columnNbr);
        
    }
    //TODO Do count properly
    OrderOrGroup groupCount;
    Column* count = new Column();
    groupCount.name = (char*)sqlTokenCount;
    col->name       = (char*)sqlTokenCount;
    groupCount.col  = count;
    groupBy->group.push_back(groupCount);
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
ParseResult Binding::editCondition(Condition* _con ,char* _value)
{
    if(_con == nullptr)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,true,"edit condition null ");
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

            _con->col->value     = _con->value;
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
            if(!isNumeric(_value))
            {
                sendMessage(MESSAGETYPE::ERROR,presentationType,false," condition column ");
                sendMessage(MESSAGETYPE::ERROR,presentationType,false,_con->name); 
                sendMessage(MESSAGETYPE::ERROR,presentationType,false,"  value not numeric ");
                sendMessage(MESSAGETYPE::ERROR,presentationType,false,_value);
                sendMessage(MESSAGETYPE::ERROR,presentationType,false," ");
                return ParseResult::FAILURE;
            }
            _con->doubleValue = atof(_value);
            break;
        }
        case t_edit::t_int:
        {
            if(!isNumeric(_value))
            {
                sendMessage(MESSAGETYPE::ERROR,presentationType,false," condition column ");
                sendMessage(MESSAGETYPE::ERROR,presentationType,false,_con->name); 
                sendMessage(MESSAGETYPE::ERROR,presentationType,false,"  value not numeric |");
                sendMessage(MESSAGETYPE::ERROR,presentationType,false,_value);
                sendMessage(MESSAGETYPE::ERROR,presentationType,false,"| ");
                return ParseResult::FAILURE;
            }
            _con->intValue = atoi(_con->value);
            break;
        }
    }
    return ParseResult::SUCCESS;
}