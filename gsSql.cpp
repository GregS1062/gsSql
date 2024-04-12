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
    printf("\n table name %s", tbl->name);
    printf(" alias %s", tbl->alias);
    for(Column* col : tbl->columns)
    {
        printf("\n\t column name %s", col->name);
        printf(" alias %s", col->tableName);
        printf(" value %s", col->value);
        if(col->primary)
            printf(" PRIMARY");
    }
    printf("\n\n Conditions");
    for(Condition* con : tbl->conditions)
    {
        printf("\n\n\t condition name      %s", con->name);
        printf("\n\t condition condition %s", con->condition);
        printf("\n\t condition op        %s", con->op);
        printf("\n\t condition value     %s", con->value);
    }
    for(sIndex* idx : tbl->indexes)
    {
        printf("\n\t index name %s", idx->name);
        printf("  file name %s", idx->fileName);
        for(Column* col : idx->columns)
        {
            printf("\n\t\t index column name %s", col->name);
        }
    }
}

void printQuery(queryParser* qp,binding* bind)
{
    printf("\n*******************************************");
    printf("\n Query Tables");
    printf("\n*******************************************");
    for(sTable* tbl : bind->lstTables)
    {
        printTable(tbl);
    }
    return;
    printf("\n--------------------------------------------");
    printf("\n column names");
    printf("\n--------------------------------------------");
    for(char* c : qp->lstColName)
    {
        printf("\n column:%s",c);
    }
    printf("\n--------------------------------------------");
    printf("\n column values");
    printf("\n--------------------------------------------");
    for(char* c : qp->lstValues)
    {
        printf("\n value:%s",c);
    }
    printf("\n--------------------------------------------");
    printf("\n Column Name Values");
    printf("\n--------------------------------------------");
    for(ColumnNameValue* nv : qp->lstColNameValue)
    {
        printf("\n column:%s value:%s",nv->name,nv->value);
    }
    if(qp->conditions.size() == 0)
    {
        printf("\n No conditions");
        return;
    }
    printf("\n--------------------------------------------");
    printf("\n Conditions");
    printf("\n--------------------------------------------");
    for(Condition* con : qp->conditions)
    {
        printf("\n\t condition name      %s", con->name);
        printf("\n\t condition condition %s", con->condition);
        printf("\n\t condition op        %s", con->op);
        printf("\n\t condition value     %s", con->value);
    }
}

int main(int argc, char* argv[])
{   
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
          default:
                printf("\n No case for this script number\n\n");
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
    
    printf("\n query=%s \n",queryStr.c_str());
    presentationType = PRESENTATION::TEXT;
    sqlParser* parser = new sqlParser((char*)sql.c_str());

    if(parser->parse() == ParseResult::FAILURE)
    {
        printf("\n sql=%s",sql.c_str());
        printf("\n %s",errText.c_str());
        return 0;
    }
    
    queryParser* query = new queryParser();

    if(query->parse((char*)queryStr.c_str(),parser) == ParseResult::FAILURE)
    {
        printf("\n query parse failed");
        printf("\n error %s",errText.c_str());
    };

    binding* bind = new binding(parser,query);
    if(bind->bind() == ParseResult::FAILURE)
    {
        printf("\n bind validate failed");
        printf("\n error %s",errText.c_str());
    };

    sqlEngine* engine = new sqlEngine();
    if(engine->execute(bind->statements.front()) == ParseResult::FAILURE)
    {
        printf("\n error %s",errText.c_str());
        return 0;
    }

    printf("\n %s",returnResult.resultTable.c_str());
    printf("\n %s",msgText.c_str());
    printf("\n %s",errText.c_str());
        
    printf("\n\n");
    return 0; 
}
