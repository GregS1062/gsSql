#pragma once
#include "sqlCommon.h"
#include "interfaces.h"
#include "prepareQuery.cpp"

/******************************************************
 * Binding
 ******************************************************/
class Binding
{
    private:

    shared_ptr<sTable>              defaultTable;    
    shared_ptr<sTable>              defaultSQLTable;
    shared_ptr<iSQLTables>          isqlTables;
    
    ParseResult                     bindColumnList();
    ParseResult                     bindFunctionColumn(std::shared_ptr<columnParts>);
    shared_ptr<sTable>              assignTable(std::shared_ptr<columnParts>);
    shared_ptr<Column>              getColumnDetail(std::shared_ptr<columnParts>,string);
    ParseResult                     bindValueList();
    ParseResult                     bindConditions();
    ParseResult                     bindCondition(shared_ptr<Condition>,bool);
    ParseResult                     bindHaving();
    ParseResult                     bindOrderBy();
    ParseResult                     bindGroupBy();
    ParseResult                     populateTable(shared_ptr<sTable>,shared_ptr<sTable>,SQLACTION);
    bool                            valueSizeOutofBounds(string, shared_ptr<Column>);
    ParseResult                     editColumn(shared_ptr<Column>,string);
    ParseResult                     editCondition(shared_ptr<Condition>);

    public:

    list<shared_ptr<columnParts>>   reportColumns;
    shared_ptr<OrderBy>             orderBy = make_shared<OrderBy>(); 
    shared_ptr<GroupBy>             groupBy = make_shared<GroupBy>(); 
    list<shared_ptr<sTable>>        lstTables;  //public for diagnostic purposes
    shared_ptr<iElements>           ielements = make_shared<iElements>();
    shared_ptr<Statement>           statement = make_shared<Statement>();     

    Binding(shared_ptr<iSQLTables>);
    shared_ptr<Statement>        bind(shared_ptr<iElements>);
};
/******************************************************
 * Constructor
 ******************************************************/
Binding::Binding(shared_ptr<iSQLTables> _isqlTables) 
{
    isqlTables = _isqlTables;
};
/******************************************************
 * Bind - Binds an array of elements into a statement
 ******************************************************/
shared_ptr<Statement> Binding::bind(shared_ptr<iElements> _ielements)
{
    try
    {

        if(debug)
            fprintf(traceFile,"\n\n-------------------------BEGIN BIND-------------------------------------------");
        
        ielements = _ielements;

        if(lstTables.size() == 0)
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"bind empty table list ");
            return nullptr;
        }

        //Table name may be formated with an alias - trim this off.
        ielements->tableName = trim(ielements->tableName);
        size_t posSpace = ielements->tableName.find(SPACE);
        if(posSpace != std::string::npos)
            ielements->tableName = clipString(ielements->tableName,posSpace);

        statement->table = getTableByName(lstTables,ielements->tableName);
        if(statement->table == nullptr)
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"statement table name is null");
            return nullptr;
        }

        statement->action = ielements->sqlAction;
        statement->rowsToReturn = ielements->rowsToReturn;

        //Will contain ONLY columns called in this query
        defaultTable = statement->table;
        if(defaultTable == nullptr)
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,false," Binding: unable to load default table");
            return nullptr;
        }

        //Contains ALL columns for table
        defaultSQLTable = getTableByName(isqlTables->tables,defaultTable->name);
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
    catch_and_trace
    return nullptr;
}
/******************************************************
 * Bind Column List - binds column name parts into table columns
 ******************************************************/
ParseResult Binding::bindColumnList()
{
    try
    {

        if(debug)
            fprintf(traceFile,"\n-------------------- bind column list ----------------");
        
        // case Insert into table values(....)
        if(ielements->lstColumns.size() == 0
        && ielements->sqlAction == SQLACTION::INSERT)
        {
            if(populateTable(defaultTable,defaultSQLTable,ielements->sqlAction) == ParseResult::FAILURE)
                return ParseResult::FAILURE;
            return ParseResult::SUCCESS;
        }

        shared_ptr<sTable> tbl;
        shared_ptr<Column> col;
        for(std::shared_ptr<columnParts> parts : ielements->lstColumns)
        {
            
            //Case 1: function
            if(!parts->function.empty())
            {
                if(bindFunctionColumn(parts) == ParseResult::FAILURE)
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

            //Case 2: Name = *
            if(parts->columnName.compare(sqlTokenAsterisk) == 0)
            {
                shared_ptr<sTable> sqlTable = getTableByName(isqlTables->tables,tbl->name );
                if(populateTable(tbl,sqlTable,ielements->sqlAction) == ParseResult::FAILURE)
                    return ParseResult::FAILURE;
                continue;
            }
            /*
                At this point, we have the column name and have identified the column - but we
                know nothing else about it.  Now we get the edit, length, record position etc.
            */
            col = getColumnDetail(parts,tbl->name);
            if(col == nullptr)
            {
                sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Cannot assign column to table: ");
                sendMessage(MESSAGETYPE::ERROR,presentationType,false,parts->fullName);
                return ParseResult::FAILURE;
            }
            
            if(!parts->columnAlias.empty())
                col->alias = parts->columnAlias;

            if(!parts->value.empty())
            {
                if(editColumn(col,parts->value) == ParseResult::FAILURE)
                    return ParseResult::FAILURE;
                col->value = parts->value;
            }
            parts->tableName = tbl->name;
            reportColumns.push_back(parts);
            tbl->columns.push_back(col);  
            fprintf(traceFile,"\n col %s added to table %s",col->name.c_str(), tbl->name.c_str());
        }
        return ParseResult::SUCCESS;
    }
    catch_and_trace
    return ParseResult::FAILURE;
}

/******************************************************
 * Assign Table - Assigns a table to column name parts
 *      - example: s.name becomes column name - table store
 ******************************************************/
shared_ptr<sTable> Binding::assignTable(std::shared_ptr<columnParts> _parts)
{
    try
    {
        if(_parts == nullptr)
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Column name parts are null ");
            return nullptr;
        }

        shared_ptr<sTable> tbl = make_shared<sTable>();
        if(!_parts->tableAlias.empty())
        {
            // case 1:  theTable.theColumn  prefix is table name
            tbl = getTableByName(lstTables,_parts->tableAlias);
            if(tbl != nullptr)
                return tbl;

            // case 2: t.theColumn  prefix is an alias
            tbl = getTableByAlias(lstTables,_parts->tableAlias);
            if(tbl != nullptr)
                return tbl;
        }

        // case 3: order by and group by can reference a column alias   tax AS ripoff   order by ripoff
        shared_ptr<Column> col = getColumnByAlias(defaultTable->columns,_parts->columnName);
        if(col != nullptr)
            return defaultTable;

        // case 4: order by and group by can reference a function   count(*)   order by count
        tbl = getTableByName(isqlTables->tables,sqlTokenFumctionTable);
        
        if(tbl == nullptr)
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Cannot retrieve function table: ");
            return nullptr;
        }

        col = getColumnByName(tbl->columns,_parts->columnName);
        if(col != nullptr)
            return tbl;

        // case 5: Most common form of all  theColumn No table reference/no table alias
        return defaultTable;
    }
    catch_and_trace
    return nullptr;
}
/******************************************************
 * Get Column Detail - Populates column detail from the SQL 
 *  definition using the table/column name pair as the key
 ******************************************************/
shared_ptr<Column>  Binding::getColumnDetail(std::shared_ptr<columnParts> _parts,string _tableName)
{
    try
    {

        shared_ptr<sTable> tbl = getTableByName(isqlTables->tables, _tableName);
        if(tbl == nullptr)
            return nullptr;

        shared_ptr<Column> col;
        shared_ptr<Column> newCol = make_shared<Column>();
        col = getColumnByName(tbl->columns,_parts->columnName);

        if(col == nullptr)
            col = getColumnByAlias(tbl->columns,_parts->columnName);

        if(col == nullptr)
            return nullptr;

 
        newCol->edit        = col->edit;
        newCol->length      = col->length;
        newCol->position    = col->position;
        newCol->name        = col->name;
        newCol->primary     = col->primary;
        newCol->tableName   = _tableName;
        return newCol;
    }
    catch_and_trace
    return nullptr;
};
/******************************************************
 * Bind Function Columns - Fills in the column detail for
 *  function columns
 ******************************************************/
 ParseResult Binding::bindFunctionColumn(std::shared_ptr<columnParts> _parts)
{
    try
    {

        if(debug)
            fprintf(traceFile,"\n------------------bind function------------");

        t_function ag = getFunctionType(_parts->function);

        if(ag == t_function::NONE)
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"unable to identify function: ");
            sendMessage(MESSAGETYPE::ERROR,presentationType,false,_parts->function);
            return ParseResult::FAILURE;
        }

        //  Create a new column
        shared_ptr<Column> col = make_shared<Column>();
        if(ag == t_function::COUNT)
        {
            if(findKeyword(_parts->columnName,(char*)sqlTokenAsterisk) != std::string::npos)
            {
                    col->functionType = t_function::COUNT;
                    col->name           = sqlTokenAsterisk;
                    _parts->columnName  = sqlTokenAsterisk;
                    col->tableName      = defaultTable->name;
                    _parts->tableName   = defaultTable->name;
                    col->edit           = t_edit::t_int;
                    if(!_parts->columnAlias.empty())
                        col->alias  = _parts->columnAlias;
                    defaultTable->columns.push_back(col);
                    reportColumns.push_back(_parts);
                    return ParseResult::SUCCESS;
            }
        }

        // all other function function must reference a column
        // Function pattern looks like this: FUNC(column table alias/column name) AS alias
        
        //Bind column to table
        shared_ptr<sTable> tbl = assignTable(_parts);
        if(tbl == nullptr)
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Cannot assign column table: ");
            sendMessage(MESSAGETYPE::ERROR,presentationType,false,_parts->fullName);
            return ParseResult::FAILURE;
        }

        col = getColumnDetail(_parts,tbl->name);
        if(col == nullptr)
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Cannot find template column: ");
            sendMessage(MESSAGETYPE::ERROR,presentationType,false,_parts->fullName);
            return ParseResult::FAILURE;
        }
        
        if( (ag == t_function::SUM
          || ag == t_function::AVG)
        &&
        (   col->edit == t_edit::t_char
         || col->edit == t_edit::t_bool
         || col->edit == t_edit::t_date))
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Cannot do mathematical functions on this type: ");
            sendMessage(MESSAGETYPE::ERROR,presentationType,false,_parts->function);
            return ParseResult::FAILURE;
        }
        
        col->functionType = ag;
        if(ag == t_function::COUNT)
            col->edit    = t_edit::t_int;

        string altAlias{};
        switch(ag)
        {
            case t_function::AVG:
                altAlias = sqlTokenAvg;
                break;
            case t_function::COUNT:
                altAlias = sqlTokenCount;
                break;
            case t_function::MAX:
                altAlias = sqlTokenMax;
                break;
            case t_function::MIN:
                altAlias = sqlTokenMin;
                break;
            case t_function::SUM:
                altAlias = sqlTokenSum;
                break;
            case t_function::NONE:
                fprintf(traceFile,"\nCounld not find function type");
                return ParseResult::FAILURE;
            case t_function::EMPTY:
                return ParseResult::FAILURE;
        }
        if(!_parts->columnAlias.empty())
            col->alias = _parts->columnAlias;

        if(col->alias.empty())
            col->alias = altAlias;

        
        tbl = getTableByName(lstTables,col->tableName);
        if(tbl == nullptr)
        {
            //is this table in the default table
            if(defaultSQLTable->isColumn(col->name))
            {
                _parts->tableName = defaultTable->name;
                defaultTable->columns.push_back(col);
                return ParseResult::SUCCESS;
            }
            else
            {
                sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Cannot find column: ");
                sendMessage(MESSAGETYPE::ERROR,presentationType,false,_parts->fullName);
                return ParseResult::FAILURE;
            }
        }
        _parts->tableName = col->tableName;
        reportColumns.push_back(_parts);
        tbl->columns.push_back(col);

        return ParseResult::SUCCESS;
    }
    catch_and_trace
    return ParseResult::FAILURE;
}; 

/******************************************************
 * Bind Value List - Binds a value list to colums
 ******************************************************/
ParseResult Binding::bindValueList()
{
    try
    {

        if(ielements->lstValues.size() == 0)
            return ParseResult::SUCCESS;

        //TODO account for case: update customer c OR insert into customer c ....select..
        shared_ptr<sTable> table = lstTables.front();
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

        for(shared_ptr<Column> col : table->columns)
        {
            if(editColumn(col,ielements->lstValues.front()) == ParseResult::FAILURE)
                return ParseResult::FAILURE;
            ielements->lstValues.pop_front();
        }
        return ParseResult::SUCCESS;
    }
    catch_and_trace
    return ParseResult::FAILURE;
}
/******************************************************
 * Populate table - Used when a query invokes an asterisk
 ******************************************************/
ParseResult Binding::populateTable(shared_ptr<sTable> _table,shared_ptr<sTable> _sqlTbl,SQLACTION sqlAction)
{
   try
   {
        if(debug)
            fprintf(traceFile,"\n-------------------- Populate table  ----------------");
        for (shared_ptr<Column> col : _sqlTbl->columns) 
        {
            if(sqlAction != SQLACTION::INSERT
            && sqlAction != SQLACTION::UPDATE)
            {
                shared_ptr<columnParts> parts = make_shared<columnParts>();
                parts->columnName = col->name;
                parts->tableName  = _table->name;
                reportColumns.push_back(parts);
            }
            _table->columns.push_back(col);
        }
        return ParseResult::SUCCESS;
    }
    catch_and_trace
    return ParseResult::FAILURE;
}
/******************************************************
 * Value Size Out Of Bounds - edit check for out of bounds
 *  condition
 ******************************************************/
bool Binding::valueSizeOutofBounds(string value,shared_ptr<Column> col)
{
    try
    {
    
        if(value.empty())
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

        if(value.length() > (size_t)col->length
        && col->edit == t_edit::t_char)
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,false,col->name);
            sendMessage(MESSAGETYPE::ERROR,presentationType,false," value length ");
            sendMessage(MESSAGETYPE::ERROR,presentationType,false,(char*)std::to_string(value.length()).c_str());
            sendMessage(MESSAGETYPE::ERROR,presentationType,false," > edit length ");
            sendMessage(MESSAGETYPE::ERROR,presentationType,false,(char*)std::to_string(col->length).c_str());
            sendMessage(MESSAGETYPE::ERROR,presentationType,false," ");
            return true;
        }

        //value is okay
        return false;
    }
    catch_and_trace
    return true;
}
/******************************************************
 * Bind Conditions - Binds query conditions to the proper
 *  table
 ******************************************************/
ParseResult Binding::bindConditions()
{
    try
    {

        shared_ptr<sTable> tbl;
        for(shared_ptr<Condition> con : ielements->lstConditions)
        {
            //Normal binding
            if(bindCondition(con,false) == ParseResult::FAILURE)
                return ParseResult::FAILURE;
            //Bind to compareToColumn
            if(bindCondition(con,true) == ParseResult::FAILURE)
                return ParseResult::FAILURE;
            tbl = getTableByName(lstTables,con->col->tableName);
            tbl->conditions.push_back(con);
        }

        for(shared_ptr<Condition> con : ielements->lstJoinConditions)
        {
            //Normal binding
            if(bindCondition(con,false) == ParseResult::FAILURE)
                return ParseResult::FAILURE;
            //Bind to compareToColumn
            if(bindCondition(con,true) == ParseResult::FAILURE)
                return ParseResult::FAILURE;

            defaultTable->conditions.push_back(con);
        }
        
        return ParseResult::SUCCESS;
    }
    catch_and_trace
    return ParseResult::FAILURE;
}
/******************************************************
 * Bind Condition - Binds a condition to proper table
 ******************************************************/
ParseResult Binding::bindCondition(shared_ptr<Condition> _con, bool _compareToColumn)
{
    try
    {
        shared_ptr<sTable> tbl;
        shared_ptr<Column> col;
        std::shared_ptr<columnParts> columnName;

        if(_con == nullptr)
            return ParseResult::SUCCESS;

        //What is being bound, a normal column or a compareToColumn?
        if(_compareToColumn)
        {
            //It is okay for compareToColumn to be null
            if(_con->compareToName == nullptr)
                return ParseResult::SUCCESS;

            if(debug)
                fprintf(traceFile,"\nbinding condition compareToName %s alias:%s table:%s",_con->compareToName->columnName.c_str(), _con->compareToName->columnAlias.c_str(), _con->compareToName->tableName.c_str());
            columnName = _con->compareToName;
        }
        else
        {
            if(debug)
                fprintf(traceFile,"\nbinding condition name %s alias:%s table:%s",_con->name->columnName.c_str(), _con->name->columnAlias.c_str(),_con->name->tableName.c_str());
            columnName = _con->name;
        }
            
        //It is not okay for normal condition column to be null
        if(columnName == nullptr)
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Condition column name is null ");
            return ParseResult::FAILURE;
        }

        //Bind column to table
        tbl = assignTable(columnName);
        if(tbl == nullptr)
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Cannot find table: ");
            sendMessage(MESSAGETYPE::ERROR,presentationType,false,_con->name->fullName);
            return ParseResult::FAILURE;
        }

        col = getColumnDetail(columnName,tbl->name);
        if(col == nullptr)
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Cannot find column name: ");
            sendMessage(MESSAGETYPE::ERROR,presentationType,false,_con->name->fullName);
            return ParseResult::FAILURE;
        }

        //Test if condition column is in the table list - if not, put it there as a non-display column
        //  This will provide data for join compares in the joinEngine
        shared_ptr<Column> testColumn = make_shared<Column>();
        if(_compareToColumn)
        {
            _con->compareToColumn = col;
            _con->compareToColumn->tableName = tbl->name;
            testColumn = tbl->getColumn(_con->compareToColumn->name);
            if(testColumn == nullptr)
            {
                _con->compareToColumn->display = false;
                tbl->columns.push_back(_con->compareToColumn);
            }
        }
        else
        {
            _con->col = col;
            _con->col->tableName = tbl->name;
            testColumn = tbl->getColumn(_con->col->name);
            if(testColumn == nullptr)
            {
                _con->col->display = false;
                tbl->columns.push_back(_con->col);
            }
        }
        if(_con->col != nullptr
        && _con->compareToColumn != nullptr)
        {
            if(_con->col->edit != _con->compareToColumn->edit)
            {
                sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Column ",_con->col->name.c_str());
                sendMessage(MESSAGETYPE::ERROR,presentationType,false," and ",_con->compareToColumn->name.c_str());
                sendMessage(MESSAGETYPE::ERROR,presentationType,false," not the same type. Cannot compare ");
                return ParseResult::FAILURE; 
            }
        }
        if(editCondition(_con) == ParseResult::FAILURE)
            return ParseResult::FAILURE;

        return ParseResult::SUCCESS;
    }
    catch_and_trace
    return ParseResult::FAILURE;
}
/******************************************************
 * Bind Order By - Binds order by columns to proper table
 ******************************************************/
ParseResult Binding::bindOrderBy()
{
    try
    {

        if (ielements->orderBy == nullptr)
            return ParseResult::SUCCESS;
       
        if(debug)
            fprintf(traceFile,"\n----------------------- bind order by -----------------------------------");
        
        shared_ptr<sTable> tbl;
        shared_ptr<Column> col;

        orderBy->asc = ielements->orderBy->asc;
        
        for(OrderOrGroup order : ielements->orderBy->order)
        {
            //case: order by or group by column is referenced by alias - check reported columns for column definition
            if(order.name->tableName.empty()
            && order.name->tableAlias.empty())
            {
                for(shared_ptr<columnParts> parts : reportColumns)
                {
                    if(strcasecmp(parts->columnAlias.c_str(),order.name->columnName.c_str()) == 0)
                    {
                        order.name = parts;
                        break;
                    }
                }
            }

            //Bind column to table
            tbl = assignTable(order.name);
            if(tbl == nullptr)
            {
                sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Cannot bind Order By column to table: ");
                if(order.name != nullptr)
                    sendMessage(MESSAGETYPE::ERROR,presentationType,false,order.name->fullName);
                return ParseResult::FAILURE;
            }

            col = getColumnDetail(order.name,tbl->name);
            if(col == nullptr)
            {
                sendMessage(MESSAGETYPE::ERROR,presentationType,true,": ");
                sendMessage(MESSAGETYPE::ERROR,presentationType,false,order.name->fullName);
                return ParseResult::FAILURE;
            }
            order.name->tableName = tbl->name;
            order.col = col;
            orderBy->order.push_back(order);
            
            if(debug)
                fprintf(traceFile,"\n bind order Column name:%s table: %s  sort# %d",col->name.c_str(), col->tableName.c_str(), order.columnNbr);
            
        }
        return ParseResult::SUCCESS;
    }
    catch_and_trace
    return ParseResult::FAILURE;
}
/******************************************************
 * Bind Group By - Binds group by columns to proper table
 ******************************************************/
ParseResult Binding::bindGroupBy()
{
    try
    {
        if(ielements->groupBy == nullptr)
        {
            return ParseResult::SUCCESS;
        }
        if(debug)
            fprintf(traceFile,"\n----------------------- bind group by -----------------------------------");

        shared_ptr<sTable> tbl;
        shared_ptr<Column> col;
        for(OrderOrGroup group : ielements->groupBy->group)
        {

            //case: order by or group by column is referenced by alias - check reported columns for column definition
            if(group.name->tableName.empty()
            && group.name->tableAlias.empty())
            {
                for(shared_ptr<columnParts> parts : reportColumns)
                {
                    if(strcasecmp(parts->columnAlias.c_str(),group.name->columnName.c_str()) == 0)
                    {
                        group.name = parts;
                        break;
                    }
                }
            }
            //Bind column to table
            tbl = assignTable(group.name);
            if(tbl == nullptr)
            {
                sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Cannot bind Group By column to table: ");
                sendMessage(MESSAGETYPE::ERROR,presentationType,false,group.name->fullName);
                return ParseResult::FAILURE;
            }

            col = getColumnDetail(group.name,tbl->name);
            if(col == nullptr)
            {
                sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Cannot bind Group By template column to table: ");
                sendMessage(MESSAGETYPE::ERROR,presentationType,false,group.name->fullName);
                return ParseResult::FAILURE;
            }
            group.name->tableName = tbl->name;
            group.col = col;
            groupBy->group.push_back(group);
            

            if(debug)
                fprintf(traceFile,"\n bind group Column name:%s table: %s  sort# %d",col->name.c_str(), col->tableName.c_str(), group.columnNbr);
            
            bindHaving();
        }

        return ParseResult::SUCCESS;
    }
    catch_and_trace
    return ParseResult::FAILURE;
}

/******************************************************
 * bind Having - binds having condition to proper table
 ******************************************************/
ParseResult Binding::bindHaving()
{

    try
    {

        //  Condition must apply to an item in the report line

        shared_ptr<Column> col;
        
        // case 1: group by zipcode Having Count(*) > 10                    Count   = function
        // case 2: group by order Having Max(tax) < 100                     tax     = column name
        // case 3: Max(tax) as maxtax group by order Having maxtax > 50     maxtax  = column alias
        
        for(shared_ptr<Condition> con : ielements->lstHavingConditions)
        {
            col = defaultTable->getColumn(con->name->columnName);
            
            // Notice what is going on here. Since a HAVING statement does not have an alias, we test the HAVING column name against the default column alias.
            if(col == nullptr)
                col = getColumnByAlias(defaultTable->columns,con->name->columnName);
            
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
    catch_and_trace
    return ParseResult::FAILURE;
}

/******************************************************
 * edit column - edits column values
 ******************************************************/
ParseResult Binding::editColumn(shared_ptr<Column> _col,string _value)
{
    try
    {
 
        if(_col == nullptr)
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"edit column null ");
            return ParseResult::FAILURE;
        }

        if(_value.empty())
            return ParseResult::SUCCESS;

        switch(_col->edit)
        {
            case t_edit::t_bool: 
            {
                if(strcasecmp(_value.c_str(),"T") == 0
                || strcasecmp(_value.c_str(),"F") == 0)
                    _col->value = _value;
                else
                {
                    fprintf(traceFile,"\n bool value %s",_value.c_str());
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
            default:
                break;
        }
        return ParseResult::SUCCESS;
    }
    catch_and_trace
    return ParseResult::FAILURE;
}
/******************************************************
 * Edit Condition Column - edits condition columns
 ******************************************************/
ParseResult Binding::editCondition(shared_ptr<Condition> _con)
{
    try
    {
    
        if(_con == nullptr)
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"condition edit is null ");
            return ParseResult::FAILURE;
        }

        if(_con->compareToName != nullptr
        && _con->compareToColumn != nullptr)
        {
            if (_con->col->edit != _con->compareToColumn->edit)
            {
                sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Comparing columns of different edits ");
                return ParseResult::FAILURE;
            }
            else
            {
                //Without an explicit value, no need for further tests
                return ParseResult::SUCCESS;
            }
        }

        if(_con->compareToName != nullptr)
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

                _con->dateValue = parseDate((char*)_con->value.c_str());
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
                _con->doubleValue = atof(_con->value.c_str());
                break;
            }
            case t_edit::t_int:
            {
                if(!isNumeric(_con->value))
                {
                    sendMessage(MESSAGETYPE::ERROR,presentationType,true," condition column ");
                    sendMessage(MESSAGETYPE::ERROR,presentationType,false,_con->name->fullName); 
                    sendMessage(MESSAGETYPE::ERROR,presentationType,false,"  value not numeric |");
                    sendMessage(MESSAGETYPE::ERROR,presentationType,false,_con->value);
                    sendMessage(MESSAGETYPE::ERROR,presentationType,false,"| ");
                    return ParseResult::FAILURE;
                }
                _con->intValue = atoi(_con->value.c_str());
                break;
            }
            default:
                break;
        }
        return ParseResult::SUCCESS;
    }
    catch_and_trace
    return ParseResult::FAILURE;
}