#pragma once
#include "sqlCommon.h"
#include "queryParser.h"
#include "lookup.h"
class binding
{
    public:
    list<sTable*>   lstTables;
    queryParser*    qp;
    sqlParser*      sp;
    tokenParser*    tok;

    binding(sqlParser*,queryParser*);
    ParseResult validate();
    ParseResult validateTableList();
    ParseResult validateColumnList();
    ParseResult populateTable(sTable*,sTable*);
    bool        valueSizeOutofBounds(char*, column*);
};

binding::binding(sqlParser* _sp,queryParser* _qp) 
{
    qp              = _qp;
    sp              = _sp;
};

ParseResult binding::validate()
{
    if(validateTableList() == ParseResult::FAILURE)
        return ParseResult::FAILURE;
    if(validateColumnList() == ParseResult::FAILURE)
        return ParseResult::FAILURE;
    return ParseResult::SUCCESS;
}
ParseResult binding::validateTableList()
{
    tokenPair tp;
    sTable* temp;
    sTable* table;
    for(char* token : qp->lstTables)
    {
        printf("\n table name %s|| \n",token);
        table = new sTable();
        tp = lookup::tokenSplit(token,(char*)" ");
        temp = lookup::getTableByName(sp->tables,tp.one);
        if(temp != nullptr)
        {
            table->name     = temp->name;
            table->fileName = temp->fileName;
            table->alias    = tp.two;
            lstTables.push_back(table);
            continue;
        }
        //No valid table name at this point
        if(tp.two == nullptr)
        {
            errText.append("<p> invalid table name:");
            errText.append(tp.one);
            return ParseResult::FAILURE;
        }
        //The valid table name must be tp.two and tp.one is the alias
        temp = lookup::getTableByName(sp->tables,tp.two);
        if(temp != nullptr)
        {
            table->name     = temp->name;
            table->fileName = temp->fileName;
            table->alias    = tp.one;
            lstTables.push_back(table);
            continue;
        }  
    }
    if(lstTables.size() == 0)
    {
        errText.append("<p> binding expecting at least one table");
        return ParseResult::FAILURE;
    }
    return ParseResult::SUCCESS;
}
ParseResult binding::validateColumnList()
{
    tokenPair   tp;
    sTable*     lstTbl;
    sTable*     sqlTbl;
    column*     col;

    for(char* token : qp->lstColName)
    {
        tp = lookup::tokenSplit(token,(char*)".");
        if(tp.two == nullptr)
        {
            lstTbl = lstTables.front();
            if(lstTbl == nullptr)
            {
                errText.append("<p>table list is empty");
                return ParseResult::FAILURE;
            }
            sqlTbl = lookup::getTableByName(sp->tables,lstTbl->name);
            if(sqlTbl == nullptr)
            {
                errText.append("<p>looking for table name:");
                errText.append(lstTbl->name);
                return ParseResult::FAILURE;
            }

            if(strcmp(tp.one,(char*)sqlTokenAsterisk) == 0)
            {
                //Since there is no alias, a single table is assumed
                if(populateTable(lstTbl,sqlTbl) == ParseResult::FAILURE)
                    return ParseResult::FAILURE;
                continue;
            }
            col = lookup::getColumnByName(sqlTbl->columns,tp.one);
            if(col == nullptr)
            {
                errText.append("<p>looking for column name:");
                errText.append(tp.one);
                return ParseResult::FAILURE;
            }
            lstTbl->columns.push_back(col);
        }
    }
    return ParseResult::SUCCESS;
}
ParseResult binding::populateTable(sTable* _table,sTable* _sqlTbl)
{

    for (column* col : _sqlTbl->columns) 
    {
        _table->columns.push_back(col);
    }
    return ParseResult::SUCCESS;
}
/******************************************************
 * Value Size Out Of Bounds
 ******************************************************/
bool binding::valueSizeOutofBounds(char* value, column* col)
{
    if(value == nullptr)
    {
        errText.append(col->name);
        errText.append(" : value is null");

        return true;
    }

    if(col == nullptr)
    {
        errText.append(" cannot find column for ");
        errText.append(value);
        return true;
    }

    if(strlen(value) > (size_t)col->length
    && col->edit == t_edit::t_char)
    {
        errText.append(col->name);
        errText.append(" value length ");
        errText.append(std::to_string(strlen(value)));
        errText.append(" > edit length ");
        errText.append(std::to_string(col->length));
        return true;
    }

    //value is okay
    return false;
}



 /* //Update will only have one query table, though conditions may have more
      queryTable = tables.front(); 
    if(queryTable == nullptr)
    {
        errText.append(" cannot find selected table ");
        return ParseResult::FAILURE;
    }
    
    // The template table defined by the CREATE syntax)
    dbTable = lookup::getTableByName(sqlDB->tables,(char*)queryTable->name);


    else{
        dbTable = lookup::getTableByName(sqlDB->tables,(char*)queryTable->name);
        if(dbTable == nullptr)
        {
            errText.append(" Check your table name. Cannot find ");
            errText.append(queryTable->name);
            errText.append(" in SQL def. ");
            return ParseResult::FAILURE;
        }
        populateQueryTable(dbTable);
    } 
    
       col = lookup::scrollColumnList(queryTable->columns,count);
        
        if(valueSizeOutofBounds(token,col))
            return ParseResult::FAILURE;
    

        if(strcasecmp(token,sqlTokenCloseParen) == 0)
        {    
            if(count == 0)
            {
                errText.append("<p> expecting at least one value");
                return ParseResult::FAILURE;
            }

            if(queryTable->columns.size() != (long unsigned int)count)
            {
                errText.append(" value count does not match number of columns in table");
                return ParseResult::FAILURE;
            }
            return ParseResult::SUCCESS;
        }

        if(conditions.size() == 0)
    {
        errText.append(" expecting at least one condition column");
        return ParseResult::FAILURE;
    }
    for(Condition* con : conditions)
    {
        col = dbTable->getColumn(con->name);
        if(col == nullptr)
        {

            errText.append(" condition column |");
            errText.append(con->name);
            errText.append("| not found Value=");
            errText.append(con->value);
            return ParseResult::FAILURE;
        }

        if(valueSizeOutofBounds(con->value,col))
            return ParseResult::FAILURE;

        switch(col->edit)
        {
            case t_edit::t_bool:    //do nothing
            {
                break; 
            }  
            case t_edit::t_char:    //do nothing
            {
                break; 
            }  
            case t_edit::t_date:    //do nothing
            {
                if(!utilities::isDateValid(con->value))
                    return ParseResult::FAILURE;
                con->dateValue = utilities::parseDate(con->value);
                break; 
            } 
            case t_edit::t_double:
            {
                if(!utilities::isNumeric(con->value))
                {
                    errText.append(" condition column ");
                    errText.append(con->name); 
                    errText.append("  value not numeric |");
                    errText.append(con->value);
                    errText.append("| ");
                    return ParseResult::FAILURE;
                }
                con->doubleValue = atof(con->value);
                break;
            }
            case t_edit::t_int:
            {
                if(!utilities::isNumeric(con->value))
                {
                    errText.append(" condition column ");
                    errText.append(con->name); 
                    errText.append("  value not numeric |");
                    errText.append(con->value);
                    errText.append("| ");
                    return ParseResult::FAILURE;
                }
                con->intValue = atoi(con->value);
                break;
            }
        }

           column* col;
    for(ColumnNameValue* colVal : lstColNameValue)
    {
        col = dbTable->getColumn(colVal->name);
        if(col->primary)
        {
            errText.append(" ");
            errText.append(colVal->name);
            errText.append(" is a primary key, cannot update ");
            return ParseResult::FAILURE;
        }
        if(col == nullptr)
        {
            errText.append(" column not found ");
            errText.append(colVal->name);
            return ParseResult::FAILURE;
        }
        if(valueSizeOutofBounds(colVal->value,col))
            return ParseResult::FAILURE;

        col->value = colVal->value;
        queryTable->columns.push_back(col);


    //Update will only have one primary table
    queryTable = tables.front();
    if(queryTable == nullptr)
    {
        errText.append(" cannot identify query table ");
        return ParseResult::FAILURE;
    }

        // Populate the query table with the columns that 
       // will be used by the engine
    

    queryTable = tables.front();
    if(queryTable == nullptr)
    {
        errText.append(" cannot find selected table ");
        return ParseResult::FAILURE;
    }



 */