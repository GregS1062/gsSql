#pragma once
#include "sqlCommon.h"
#include "queryParser.h"
#include "lookup.h"
class binding
{
    public:
    list<sTable*>    lstTables;
    list<Condition*> lstConditions;
    list<Statement*> statements;
    sTable*          defaultTable;    
    sTable*          defaultSQLTable;   
    queryParser*     qp;
    sqlParser*       sp;
    tokenParser*     tok;

    binding(sqlParser*,queryParser*);
    ParseResult     bind();
    ParseResult     bindTableList();
    ParseResult     bindColumnList();
    ParseResult     bindColumnValueList();
    ParseResult     bindColumn(char*);
    ColumnAlias*    resolveColumnAlias(TokenPair*);
    ParseResult     editColumn(Column*,char*);
    ParseResult     editCondition(Condition*,char*);
    ParseResult     bindNonAliasedColumn(char*, char*);
    ParseResult     bindAliasedColumn(TokenPair*, char*);
    ParseResult     bindValueList();
    ParseResult     bindConditions();
    ParseResult     populateTable(sTable*,sTable*);
    bool            valueSizeOutofBounds(char*, Column*);
};
/******************************************************
 * Constructor
 ******************************************************/
binding::binding(sqlParser* _sp,queryParser* _qp) 
{
    qp              = _qp;
    sp              = _sp;
};
/******************************************************
 * Bind
 ******************************************************/
ParseResult binding::bind()
{


    Statement* statement;

    if(bindTableList() == ParseResult::FAILURE)
        return ParseResult::FAILURE;

    if(lstTables.size() == 0)
    {
        utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,true,"bind empty table list ");
        return ParseResult::FAILURE;
    }

    for(sTable* tbl : lstTables)
    {
         statement = new Statement();
         statement->table = tbl;
         statement->plan->action = qp->sqlAction;
         statement->plan->rowsToReturn = qp->rowsToReturn;
         statements.push_back(statement);
    }

    if(lstTables.size() == 1)
    {
        defaultTable = lstTables.front();
        if(defaultTable == nullptr)
        {
            utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false," binding: unable to load default table");
            return ParseResult::FAILURE;
        }
        defaultSQLTable = lookup::getTableByName(sp->tables,defaultTable->name);
        if(defaultSQLTable == nullptr)
        {
            utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false," binding: unable to load default SQL table");
            return ParseResult::FAILURE;
        }
    }


    if(bindColumnList() == ParseResult::FAILURE)
    {
        if(debug)
            printf("\n column binding failure");
        return ParseResult::FAILURE;
    }

    if(bindValueList() == ParseResult::FAILURE)
        return ParseResult::FAILURE;

    if(bindColumnValueList() == ParseResult::FAILURE)
        return ParseResult::FAILURE;

    if(bindConditions() == ParseResult::FAILURE)
       return ParseResult::FAILURE;

    return ParseResult::SUCCESS;
}
/******************************************************
 * Bind Table List
 ******************************************************/
ParseResult binding::bindTableList()
{
    TokenPair* tp;
    sTable* temp;
    sTable* table;
    for(char* token : qp->lstTables)
    {
        if(debug)
            printf("\n table name %s|| \n",token);

        table = new sTable();

        tp = lookup::tokenSplit(token,(char*)" ");

        if(tp == nullptr)
        {
            utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Table name is null");
            return ParseResult::FAILURE;
        }

        if(debug)
            printf("\n tp1:%s| tp2:%s|",tp->one, tp->two);

        temp = lookup::getTableByName(sp->tables,tp->one);
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
            utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,true," invalid table name:");
            utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false,tp->one);
            return ParseResult::FAILURE;
        }

        //The valid table name must be tp->two and tp->one is the alias
        temp = lookup::getTableByName(sp->tables,tp->two);
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
        utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,true," binding expecting at least one table");
        return ParseResult::FAILURE;
    }

    return ParseResult::SUCCESS;
}
/******************************************************
 * Bind Column
 ******************************************************/
ParseResult binding::bindColumn(char* colName)
{
    TokenPair*  tp;
    tp = lookup::tokenSplit(colName,(char*)".");
    if(tp == nullptr)
    {
        utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Column name is null");
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
ParseResult binding::bindColumnList()
{
    //case Insert into table values(....)
    if(qp->lstColName.size() == 0
    && qp->lstColNameValue.size() == 0)
    {
        if(populateTable(defaultTable,defaultSQLTable) == ParseResult::FAILURE)
            return ParseResult::FAILURE;
        return ParseResult::SUCCESS;
    }

    for(char* token : qp->lstColName)
    {
        if(debug)
            printf("\n binding column %s",token);

        if(bindColumn(token) == ParseResult::FAILURE)
            return ParseResult::FAILURE;
    }
    return ParseResult::SUCCESS;
}
/******************************************************
 * Bind Column Value List
 ******************************************************/
ParseResult binding::bindColumnValueList()
{
    TokenPair*  tp;

    for(ColumnNameValue* cv : qp->lstColNameValue)
    {
        tp = lookup::tokenSplit(cv->name,(char*)".");
        if(tp == nullptr)
        {
            utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Column name/value is null");
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
ParseResult binding::bindNonAliasedColumn(char* _name, char* _value)
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
        utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,true,"looking for column name:");
        utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false,_name);
        return ParseResult::FAILURE;
    }

    if(editColumn(col,_value) == ParseResult::FAILURE)
        return ParseResult::FAILURE;

    defaultTable->columns.push_back(col);

    return ParseResult::SUCCESS;
}
/******************************************************
 * Resolve Column Alias
 ******************************************************/
ColumnAlias* binding::resolveColumnAlias(TokenPair* tp)
{
    ColumnAlias* ca = new ColumnAlias();
    sTable* temp1;
    sTable* temp2;

    if(tp == nullptr)
    {
        utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,true,"ResolveColumnAlias tokenPair is null");
        return nullptr;
    }
    
    temp1 = lookup::getTableByAlias(lstTables,tp->one);
    temp2 = lookup::getTableByAlias(lstTables,tp->two);
    if(temp1 == nullptr
    && temp2 == nullptr)
    {
        utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,true,"looking for table by alias:");
        utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false,tp->one);
        utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false," or ");
        utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false,tp->two);
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
    return ca;
};
/******************************************************
 * Bind Non-Aliased Column
 ******************************************************/
ParseResult binding::bindAliasedColumn(TokenPair* tp, char* _value)
{
    sTable* lstTbl;
    sTable* sqlTbl;
    Column* col;
    
    ColumnAlias* ca = resolveColumnAlias(tp);
    if(ca == nullptr)
    {
        utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,true,"cannot resolve alias table name");
        return ParseResult::FAILURE;
    }

    lstTbl = lookup::getTableByName(lstTables,ca->tableName);
    if(lstTbl == nullptr)
    {
        utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,true,"bind alias column table list null ");
        return ParseResult::FAILURE;
    }
    sqlTbl = lookup::getTableByName(sp->tables,ca->tableName);
    if(sqlTbl == nullptr)
    {
        utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,true,"looking for table name:");
        utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false,lstTbl->name);
        return ParseResult::FAILURE;
    }

    if(strcmp(ca->columnName,(char*)sqlTokenAsterisk) == 0)
    {
        //Since there is no alias, a single table is assumed
        if(populateTable(lstTbl,sqlTbl) == ParseResult::FAILURE)
            return ParseResult::FAILURE;
        return ParseResult::SUCCESS;
    }

    col = lookup::getColumnByName(sqlTbl->columns,ca->columnName);
    if(col == nullptr)
    {
        utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,true,"looking for column name:");
        utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false,tp->two);
        return ParseResult::FAILURE;
    }

    col->tableName = lstTbl->name;
    if(editColumn(col,_value) == ParseResult::FAILURE)
        return ParseResult::FAILURE;

    lstTbl->columns.push_back(col);
    return ParseResult::SUCCESS;
}

/******************************************************
 * Bind Value List
 ******************************************************/
ParseResult binding::bindValueList()
{
    if(qp->lstValues.size() == 0)
        return ParseResult::SUCCESS;

    //TODO account for case: update customer c OR insert into customer c ....select..
    sTable* table = lstTables.front();
    if(table == nullptr)
    {
        utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,true,"bind value list table null ");
        return ParseResult::FAILURE;
    }
    if(table->columns.size() != qp->lstValues.size())
    {
        utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,true,"The count of columns ");
        utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false,(char*)std::to_string(table->columns.size()).c_str());
        utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false," and values ");
        utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false,(char*)std::to_string(qp->lstValues.size()).c_str());
        utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false," do not match");
        return ParseResult::FAILURE;
    }

    for(Column* col : table->columns)
    {
        if(editColumn(col,qp->lstValues.front()) == ParseResult::FAILURE)
            return ParseResult::FAILURE;
        qp->lstValues.pop_front();
    }
    return ParseResult::SUCCESS;
}
/******************************************************
 * Populate table
 ******************************************************/
ParseResult binding::populateTable(sTable* _table,sTable* _sqlTbl)
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
bool binding::valueSizeOutofBounds(char* value, Column* col)
{
    if(value == nullptr)
    {
        utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false,col->name);
        utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false," : value is null");
        return true;
    }

    if(col == nullptr)
    {
        utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false," cannot find column for ");
        utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false,value);
        return true;
    }

    if(strlen(value) > (size_t)col->length
    && col->edit == t_edit::t_char)
    {
        utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false,col->name);
        utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false," value length ");
        utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false,(char*)std::to_string(strlen(value)).c_str());
        utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false," > edit length ");
        utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false,(char*)std::to_string(col->length).c_str());
        return true;
    }

    //value is okay
    return false;
}
/******************************************************
 * Bind Conditions
 ******************************************************/
ParseResult binding::bindConditions()
{
    TokenPair* tp;
    for(Condition* con : qp->conditions)
    {
        tp = lookup::tokenSplit(con->name,(char*)".");
        if(tp == nullptr)
        {
            utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Condition column name is null");
            return ParseResult::FAILURE;
        }
        //------------------------------------------------
        // case: no alias
        //------------------------------------------------
        if(tp->two == nullptr)
        {
            Column* col = lookup::getColumnByName(defaultSQLTable->columns,tp->one);
            if(col == nullptr)
            {
                utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,true,"looking for column name:");
                utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false,tp->one);
                return ParseResult::FAILURE;
            }
            con->col = col;
            if(editCondition(con,con->value) == ParseResult::FAILURE)
                return ParseResult::FAILURE;

            defaultTable->conditions.push_back(con);
            continue;
        }
        //--------------------------------------
        // case alias.col
        //--------------------------------------
        sTable* lstTbl;
        sTable* sqlTbl;
        ColumnAlias* ca = resolveColumnAlias(tp);
        if(ca == nullptr)
        {
            utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,true,"could not resolve alias for column:");
            utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false,con->name);
            utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,true," table name:");
            utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false,defaultTable->name);
            utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false," alias:");
            utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false,defaultTable->alias);
            return ParseResult::FAILURE; 
        }

        lstTbl = lookup::getTableByName(lstTables,ca->tableName);
        if(lstTbl == nullptr)
        {
            utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,true,"looking for table name:");
            utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false,ca->tableName);
            return ParseResult::FAILURE;
        }
        sqlTbl = lookup::getTableByName(sp->tables,ca->tableName);
        if(sqlTbl == nullptr)
        {
            utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,true,"looking for table name:");
            utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false,ca->tableName);
            return ParseResult::FAILURE;
        }

        Column* col = lookup::getColumnByName(sqlTbl->columns,ca->columnName);
        if(col == nullptr)
        {
            utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,true,ca->columnName);
            utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false," not found");
            return ParseResult::FAILURE;  
        }
        con->col  = col;
        con->col->tableName = lstTbl->name;
        if(editCondition(con,con->value) == ParseResult::FAILURE)
            return ParseResult::FAILURE;

        lstTbl->conditions.push_back(con);
    }
     
    return ParseResult::SUCCESS;
}
/******************************************************
 * edit column
 ******************************************************/
ParseResult binding::editColumn(Column* _col,char* _value)
{
    if(_col == nullptr)
    {
        utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,true,"edit column null ");
        return ParseResult::FAILURE;
    }

    if(_value == nullptr)
        return ParseResult::SUCCESS;

    switch(_col->edit)
    {
        case t_edit::t_bool:    //do nothing
        {
            _col->value = _value;
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
            if(!utilities::isDateValid(_col->value))
                return ParseResult::FAILURE;
            _col->value = _value;
            break; 
        } 
        case t_edit::t_double:
        {
            if(!utilities::isNumeric(_value))
            {
                utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false," column ");
                utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false,_col->name); 
                utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false,"  value not numeric |");
                utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false,_value);
                utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false,"| ");
                return ParseResult::FAILURE;
            }
            _col->value = _value;
            break;
        }
        case t_edit::t_int:
        {
            if(!utilities::isNumeric(_value))
            {
                utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false," column ");
                utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false,_col->name); 
                utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false,"  value not numeric |");
                utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false,_value);
                utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false,"| ");
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
ParseResult binding::editCondition(Condition* _con ,char* _value)
{
    if(_con == nullptr)
    {
        utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,true,"edit condition null ");
        return ParseResult::FAILURE;
    }

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
            if(!utilities::isDateValid(_con->value))
                return ParseResult::FAILURE;

            _con->dateValue = utilities::parseDate(_con->value);
            break; 
        } 
        case t_edit::t_double:
        {
            if(!utilities::isNumeric(_value))
            {
                utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false," condition column ");
                utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false,_con->name); 
                utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false,"  value not numeric |");
                utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false,_value);
                utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false,"| ");
                return ParseResult::FAILURE;
            }
            _con->doubleValue = atof(_value);
            break;
        }
        case t_edit::t_int:
        {
            if(!utilities::isNumeric(_value))
            {
                utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false," condition column ");
                utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false,_con->name); 
                utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false,"  value not numeric |");
                utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false,_value);
                utilities::sendMessage(MESSAGETYPE::ERROR,presentationType,false,"| ");
                return ParseResult::FAILURE;
            }
            _con->intValue = atoi(_con->value);
            break;
        }
    }
    return ParseResult::SUCCESS;
}