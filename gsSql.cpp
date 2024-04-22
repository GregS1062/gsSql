#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <string.h>
#include "sqlParser.h"
#include "queryParser.h"
#include "indexBase.h"
#include "lookup.h"
#include "binding.h"
#include "sqlEngine.h"


using namespace std;

void printTable(sTable* tbl)
{
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

void printQuery(queryParser* qp,binding* bind)
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
    for(char* c : qp->lstColName)
    {
        fprintf(traceFile,"\n column:%s",c);
    }
    fprintf(traceFile,"\n\n--------------------------------------------");
    fprintf(traceFile,"\n column values");
    fprintf(traceFile,"\n\n--------------------------------------------");
    for(char* c : qp->lstValues)
    {
        fprintf(traceFile,"\n value:%s",c);
    }
    fprintf(traceFile,"\n\n--------------------------------------------");
    fprintf(traceFile,"\n Column Name Values");
    fprintf(traceFile,"\n\n--------------------------------------------");
    for(ColumnNameValue* nv : qp->lstColNameValue)
    {
        fprintf(traceFile,"\n column:%s value:%s",nv->name,nv->value);
    }
    if(qp->conditions.size() == 0)
    {
        fprintf(traceFile,"\n No conditions");
        return;
    }
    fprintf(traceFile,"\n\n--------------------------------------------");
    fprintf(traceFile,"\n Conditions");
    fprintf(traceFile,"\n\n--------------------------------------------");
    for(Condition* con : qp->conditions)
    {
        fprintf(traceFile,"\n\t condition name      %s", con->name);
        fprintf(traceFile,"\n\t condition condition %s", con->condition);
        fprintf(traceFile,"\n\t condition op        %s", con->op);
        fprintf(traceFile,"\n\t condition value     %s", con->value);
    }
}

int main(int argc, char* argv[])
{   
    traceFile = fopen ("trace.txt" , "w");
    std::string scriptFileName;
    std::string script;
    if(argc > 1)
    {
        int runScript = atoi(argv[1]);
        switch(runScript)
        {
            case 1:
                script = "t10-select-top-5";
                break;
          case 2:
                script = "t20-select-asterisk-equal";
                break;
          case 3:   
                script = "t21-select-equal-single-value";
                break;
          case 4:   
                script = "t30-select-like-and-like";
                break;
          case 5:
                script = "t40-select-top-equal-and-equal";
                break;
          case 6:
                script = "t50-insert-into-values";
                break;
          case 7:   
                script = "t55-insert-items-from-values"; 
                break;
          case 8:
                script = "t60-insert-columns-and-values"; 
                break;
          case 9:
                script = "t70-update-customers";
                break;
          case 10:
                script = "t80-insert-store";               
                break;
          case 11:
                script = "t81-select-store-by-name";        //segment fault
                break; 
           case 12:
                script = "t90-select-columns-with-alias";   //conditions failed
                break;
           case 13:
                script = "t100-select-columns-and tables-with-alias"; //To be completed
                break;
            case 14:
                script = "t99-select-orderdate-in-orders";
                break;
             case 15:
                script = "t100-select-multiple-conditions";
                break;        
             case 16:
                script = "t101-select-equal-index";
                break;   
            case 17:
                script = "t102-select-column-greater-column";
                break;   
            case 18:
                script = "t103-delete";
                break; 
            case 19:
                script = "t104-orderby-multiples";
                break;
            case 20:
                script = "t105-select-order-by-orderid";
                break;
          default:
                fprintf(traceFile,"\n No case for this script number\n\n");
                return 0;
        }
    }
    else
    {
        script = "t60-insert-columns-and-values";
    }

    scriptFileName = "/home/greg/projects/regTest/scripts/";
    scriptFileName.append(script);

    std::string sqlFileName = "bike.sql";
    std::ifstream ifs(sqlFileName);
    std::string sql ( (std::istreambuf_iterator<char>(ifs) ),
                       (std::istreambuf_iterator<char>()    ) );
    
    std::ifstream ifq(scriptFileName);
    std::string queryStr ( (std::istreambuf_iterator<char>(ifq) ),
                       (std::istreambuf_iterator<char>()    ) );
    
    fprintf(traceFile,"\n query=%s \n",queryStr.c_str());
    presentationType = PRESENTATION::TEXT;
    sqlParser* parser = new sqlParser((char*)sql.c_str());

    if(parser->parse() == ParseResult::FAILURE)
    {
        fprintf(traceFile,"\n sql=%s",sql.c_str());
        fprintf(traceFile,"\n %s",errText.c_str());
        return 0;
    }
    debug = true;
    queryParser* query = new queryParser();

    if(query->parse((char*)queryStr.c_str(),parser) == ParseResult::FAILURE)
    {
        fprintf(traceFile,"\n query parse failed");
        fprintf(traceFile,"\n error %s",errText.c_str());
        return 0;
    };

    binding* bind = new binding(parser,query);
    if(bind->bind() == ParseResult::FAILURE)
    {
        fprintf(traceFile,"\n bind validate failed");
        fprintf(traceFile,"\n error %s",errText.c_str());
        return 0;
    };

    fclose(traceFile);
    return 0; 

    sqlEngine* engine = new sqlEngine(parser);
    if(engine->execute(bind->lstStatements.front()) == ParseResult::FAILURE)
    {
        fprintf(traceFile,"\n error %s",errText.c_str());
        return 0;
    }

    fprintf(traceFile,"\n %s",returnResult.resultTable.c_str());
    fprintf(traceFile,"\n %s",msgText.c_str());
    fprintf(traceFile,"\n %s",errText.c_str());
        
    fprintf(traceFile,"\n\n");
    fclose(traceFile);
    return 0; 
}
