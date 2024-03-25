#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <string.h>
#include "sqlParser.h"
#include "queryParser.h"
#include "sqlEngine.h"
#include "indexBase.h"
#include "lookup.h"
#include "debug.h"

using namespace std;

void printTable(sTable* tbl)
{
    printf("\n table name %s", tbl->name);
    printf(" alias %s", tbl->alias);
    for(column* col : tbl->columns)
    {
        printf("\n\t\t column name %s", col->name);
        printf(" alias %s", col->alias);
        printf(" value %s", col->value);
        if(col->primary)
            printf(" PRIMARY");
    }
    for(sIndex* idx : tbl->indexes)
    {
        printf("\n\t index name %s", idx->name);
        printf("  file name %s", idx->fileName);
        for(column* col : idx->columns)
        {
            printf("\n\t\t index column name %s", col->name);
        }
    }
}

void printQuery(queryParser* qp)
{
    printf("\n*******************************************");
    printf("\n Query Tables");
    printf("\n*******************************************");
    for(sTable* tbl : qp->tables)
    {
        printTable(tbl);
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
        if(con->table == nullptr)
        {
            printf("\n No condition table ");
            return;
        }
        printTable(con->table);
    }
}

int main()
{

    std::string sqlFileName = "bike.sql";
    std::ifstream ifs(sqlFileName);
    std::string sql ( (std::istreambuf_iterator<char>(ifs) ),
                       (std::istreambuf_iterator<char>()    ) );
    
    std::string scriptFileName = "/home/greg/projects/regTest/scripts/t81-select-store-by-name";
    std::ifstream ifq(scriptFileName);
    std::string queryStr ( (std::istreambuf_iterator<char>(ifq) ),
                       (std::istreambuf_iterator<char>()    ) );
    queryStr.clear();
    
    //queryStr.append("Update customers c set street2 = \"where\" where c.custid = \"0001230001\"");
   //queryStr.append("Select top 5 * from customers  where custid = \"000009196\"");
  // queryStr.append("insert into items i values (0, \"SO43659\", \"BK M82B 42\", 1, 2024.99, 0.00, 2024.99)");
    //queryStr.append("Select top 5 * from stores");
    queryStr.append("select c.custid, s.name, s.email from stores s, customers c");
    printf("\n query=%s \n",queryStr.c_str());

    sqlParser* parser = new sqlParser((char*)sql.c_str());

    if(parser->parse() == ParseResult::FAILURE)
    {
        printf("\n sql=%s",sql.c_str());
        printf("\n %s",errText.c_str());
        return 0;
    }

    printf("\n sql parse success");
    
    queryParser* query = new queryParser();

    if(query->parse((char*)queryStr.c_str(),parser) == ParseResult::FAILURE)
    {
        printf("\n query parse failed");
        printf("\n error %s",errText.c_str());
        return 0;
    };
    
    
    printQuery(query);
    printf("\n\n");
    return 0;
    
    sTable* table = lookup::getTableByName(parser->tables,(char*)query->queryTable->name);
    if(table == nullptr)
    {
        printf("\n table not found");
        return 0;
    }
 
    sqlEngine* engine = new sqlEngine();
    engine->prepare(query,table);

    if(engine->open() == ParseResult::FAILURE)
    {
        printf("\n engine open failed");
        printf("\n error %s",errText.c_str());
        return 0;
    }
        
    printf("\n engine opened");
    if(query->sqlAction == SQLACTION::SELECT)
    {
        string output = engine->select();
        printf("\n select output = %s", output.c_str());
        return 0;
    }
    if(query->sqlAction == SQLACTION::UPDATE)
    {
        engine->update();
        printf("\n %s", errText.c_str());
        printf("\n %s", returnResult.message.c_str());
        return 0;
    }
    if(query->sqlAction == SQLACTION::INSERT)
    {
        engine->insert();
        printf("\n %s", errText.c_str());
        printf("\n %s", returnResult.message.c_str());
    }
    printf("\n\n");
        string indexFileName;
        indexFileName.append("~/projects/test/testIndex/custid.idx");
        fstream* indexStream = new fstream{};
		indexStream->open(indexFileName, ios::in | ios::out | ios::binary);
        Debug::dumpAll(false,indexStream);
        indexStream->close();
    printf("\n\n");
    return 0;
}
