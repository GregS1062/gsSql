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
    ParseResult     bindColumnValueList();
    ParseResult     bindColumn(char*);
    Column*         resolveColumnName(char*);
    Column*         bindAggregateColumn(char*, signed int);
    ParseResult     bindNonAliasedColumn(char*, char*);
    ParseResult     bindAliasedColumn(char*, TokenPair*, char*);
    ParseResult     bindValueList();
    ParseResult     bindConditions();
    ParseResult     bindOrderBy();
    ParseResult     bindGroupBy();
    ParseResult     populateTable(sTable*,sTable*);
    bool            valueSizeOutofBounds(char*, Column*);
    ParseResult     editColumn(Column*,char*);
    ParseResult     editCondition(Condition*,char*);

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

    if(bindColumnValueList() == ParseResult::FAILURE)
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
    if(bindAliasedColumn(colName, tp, (char*)"") == ParseResult::FAILURE)
            return ParseResult::FAILURE;
    
    return ParseResult::SUCCESS;
}
/******************************************************
 * Bind Column List
 ******************************************************/
ParseResult Binding::bindColumnList()
{
    /*
        case Insert into table values(....)
    */

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
    //TODO
    /*
        update orders set totalAmount = (Select price from items where...) where ordid = ....
    */
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
            delete tp;
            return ParseResult::FAILURE;
        }

        if(tp->two == nullptr)
        {
            if(bindNonAliasedColumn(tp->one,cv->value) == ParseResult::FAILURE)
            {
                delete tp;
                return ParseResult::FAILURE;
            }
        }
        else
        if(bindAliasedColumn(cv->name, tp, cv->value) == ParseResult::FAILURE)
        {
                delete tp;
                return ParseResult::FAILURE;
        }
        delete tp;
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

    Column* col = resolveColumnName(_name);
    if(col == nullptr)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,true,"looking for column name:");
        sendMessage(MESSAGETYPE::ERROR,presentationType,false,_name);
        return ParseResult::FAILURE;
    }
    if(strlen(_value) > 0)
        if(editColumn(col,_value) == ParseResult::FAILURE)
            return ParseResult::FAILURE;
    if(debug)
    {
        fprintf(traceFile,"\nbound column name:%s",col->name);
        fprintf(traceFile,"\nbound column alias:%s",col->alias);
        fprintf(traceFile,"\nvalue:%s",col->value);
    }

    defaultTable->columns.push_back(col);

    return ParseResult::SUCCESS;
}
/******************************************************
 * Resolve Column Alias
 ******************************************************/
Column* Binding::resolveColumnName(char* _str)
{
    /*
        Column name have the following forms

        case 1) aggregates          - COUNT(), SUM(), AVG(), MAX(), MIN()
        case 2) column alias        - surname AS last
        case 3) simple name         - surname
        case 4) table/column        - customers.surname
        case 5) table alias column  - c.surname from customers c
    
    */

   sTable* tbl;

    // case 1) aggregates          - COUNT(), SUM(), AVG(), MAX(), MIN()
    signed int posParen = lookup::findDelimiter(_str,(char*)sqlTokenOpenParen);
    if(posParen > NEGATIVE)
       return bindAggregateColumn(_str, posParen);

    signed int columnAliasPosition = lookup::findDelimiter(_str,(char*)sqlTokenAs);
    if(columnAliasPosition == DELIMITERERR)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Failure to resolve column name");
        sendMessage(MESSAGETYPE::ERROR,presentationType,false,_str);
        return nullptr;
    }

    // case 2) alias               - surname AS last
    char* columnAlias = nullptr;
    char* columnName  = _str;
    if(columnAliasPosition > NEGATIVE)
    {
        columnAlias = dupString(_str+columnAliasPosition+3);
        columnName[columnAliasPosition] = '\0';
    }

    if(debug)
        fprintf(traceFile,"\nCase 2: column name = %s column alias = %s",columnName,columnAlias);

    
    TokenPair* tp = lookup::tokenSplit(columnName,(char*)sqlTokenPeriod);
    if(tp == nullptr)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Failure to resolve column name: ");
        sendMessage(MESSAGETYPE::ERROR,presentationType,false,columnName);
        return nullptr; 
    }

    Column* col{};

    // case 3) simple name         - surname
    if(tp->two == nullptr)
    {
        col = lookup::getColumnByName(defaultTable->columns,tp->one);
        if(col == nullptr)
        {
            col = lookup::getColumnByName(defaultSQLTable->columns,tp->one);
            if(col == nullptr)
            {
                sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Case 3: Column name not found in query table: ");
                sendMessage(MESSAGETYPE::ERROR,presentationType,false,columnName);
                return nullptr; 
            }
        }
        col->alias = columnAlias;
        return col;
    }

    // case 4) table/column        - customers.surname
    tbl = lookup::getTableByName(isqlTables->tables,tp->one);
    if(tbl != nullptr)
    {
        col = lookup::getColumnByName(tbl->columns,tp->two); 
        if(col == nullptr)
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Case 4: Column name not found in query table:");
            sendMessage(MESSAGETYPE::ERROR,presentationType,false,columnName);
            return nullptr; 
        }
        col->alias = columnAlias;
        return col;
    }

    // case 5) table/column        - c.surname
    tbl = lookup::getTableByAlias(lstTables,tp->one);
    if(tbl == nullptr)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Case 5: table alias not foung: ");
        sendMessage(MESSAGETYPE::ERROR,presentationType,false,tp->two);
        return nullptr; 
    }
    tbl = lookup::getTableByName(isqlTables->tables,tbl->name);
    if(tbl == nullptr)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Case 5: table name: ");
        sendMessage(MESSAGETYPE::ERROR,presentationType,false,tp->two);
        return nullptr; 
    }
    col = lookup::getColumnByName(tbl->columns,tp->two); 
    if(col == nullptr)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Case 5: Column name not found in query table: ");
        sendMessage(MESSAGETYPE::ERROR,presentationType,false,tp->two);
        return nullptr; 
    }
    col->alias = columnAlias;
    return col;

};
/******************************************************
 * Bind Non-Aliased Column
 ******************************************************/
ParseResult Binding::bindAliasedColumn(char* colName, TokenPair* tp, char* _value)
{
    sTable* lstTbl;
    sTable* sqlTbl;
    Column* col;
    
    //include all columns of alias table
    //case: s.*
    if(strcmp(tp->two,(char*)sqlTokenAsterisk) == 0)
    {
        lstTbl = lookup::getTableByAlias(isqlTables->tables, tp->one);
        if(lstTbl == nullptr)
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Cannot find table");
            sendMessage(MESSAGETYPE::ERROR,presentationType,false,tp->one);
            return ParseResult::FAILURE;
        }
        sqlTbl = lookup::getTableByName(isqlTables->tables,lstTbl->name);
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


    col = resolveColumnName(colName);
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

    if(editColumn(col,_value) == ParseResult::FAILURE)
        return ParseResult::FAILURE;

    lstTbl->columns.push_back(col);
    return ParseResult::SUCCESS;
}
/******************************************************
 * Bind Aggregate Columns
 ******************************************************/
 Column* Binding::bindAggregateColumn(char* _str, signed int _posParen)
{
    if(debug)
        fprintf(traceFile,"\n------------------bind aggregate------------");

    // Assign aggregate function type
    char* agStr = dupString(_str);
    agStr[_posParen] = '\0';

    t_aggregate ag = t_aggregate::NONE;

    if(lookup::findDelimiter(agStr,(char*)sqlTokenCount) > NEGATIVE)
        ag = t_aggregate::COUNT;
    else
        if(lookup::findDelimiter(agStr,(char*)sqlTokenSum) > NEGATIVE)
            ag = t_aggregate::SUM;
        else
            if(lookup::findDelimiter(agStr,(char*)sqlTokenMax) > NEGATIVE)
                ag = t_aggregate::MAX;
            else
                if(lookup::findDelimiter(agStr,(char*)sqlTokenMin) > NEGATIVE)
                    ag = t_aggregate::MIN;
                else
                    if(lookup::findDelimiter(agStr,(char*)sqlTokenAvg) > NEGATIVE)
                        ag = t_aggregate::AVG;


    if(ag == t_aggregate::NONE)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,true,"unable to identify function: ");
        sendMessage(MESSAGETYPE::ERROR,presentationType,false,agStr);
        return nullptr;
    }

    char* colStr = dupString(_str+_posParen+1); 

    //  Create a new column
    Column* col{};
    if(ag == t_aggregate::COUNT)
    {
       if(lookup::findDelimiter(colStr,(char*)sqlTokenAsterisk) > NEGATIVE)
       {
            col = new Column();
            col->aggregateType = t_aggregate::COUNT;
            col->name = (char*)"COUNT";
            col->edit   = t_edit::t_int;
            return col;
       }
    }


    // all other aggregate function must reference a column
    // Function pattern looks like this: FUNC(column table alias/column name) AS alias

    int ii = 0;
    for(size_t i=0;i<strlen(_str);i++)
    {
        if(colStr[i] != CLOSEPAREN)
        {
            colStr[ii] = colStr[i];
            ii++;
        }
    }
    ii++;
    colStr[ii] = '\0';

    col = resolveColumnName(colStr);
    
    if(col == nullptr)
        return nullptr;
    
    col->aggregateType = ag;

    char* altAlias{};
    switch(ag)
    {
        case t_aggregate::AVG:
            altAlias = (char*)sqlTokenAvg;
            break;
        case t_aggregate::COUNT:
            altAlias = (char*)sqlTokenCount;
            break;
        case t_aggregate::MAX:
            altAlias = (char*)sqlTokenMax;
            break;
        case t_aggregate::MIN:
            altAlias = (char*)sqlTokenMin;
            break;
        case t_aggregate::SUM:
            altAlias = (char*)sqlTokenSum;
            break;
        case t_aggregate::NONE:
            fprintf(traceFile,"\nCounld not find function type");
            return nullptr;
    }
    if(col->alias == nullptr)
        col->alias = altAlias;
    return col;
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
    Column* col;
    for(Condition* con : ielements->lstConditions)
    {
        if(con->name == nullptr)
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Condition column name is null ");
            return ParseResult::FAILURE;
        }
        col = resolveColumnName(con->name);
        if(col == nullptr)
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"looking for column name: ");
            sendMessage(MESSAGETYPE::ERROR,presentationType,false,con->name);
            sendMessage(MESSAGETYPE::ERROR,presentationType,false," ");
            return ParseResult::FAILURE;
        }

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

    Column* col;

    if (ielements->orderBy == nullptr)
        return ParseResult::SUCCESS;

    for(OrderOrGroup order : ielements->orderBy->order)
    {

        col = resolveColumnName(order.name);
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

    Column* col;

    for(OrderOrGroup group : ielements->groupBy->group)
    {
        col = resolveColumnName(group.name);
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
/*     OrderOrGroup groupCount;
    Column* count = new Column();
    groupCount.name = (char*)sqlTokenCount;
    col->name       = (char*)sqlTokenCount;
    groupCount.col  = count;
    groupBy->group.push_back(groupCount); */
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