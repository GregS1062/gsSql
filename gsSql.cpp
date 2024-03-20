#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <string.h>
#include "sqlParser.h"
#include "queryParser.h"
#include "sqlEngine.h"
#include "global.h"

using namespace std;

int main()
{

    std::string sqlFileName = "bike.sql";
    std::ifstream ifs(sqlFileName);
    std::string sql ( (std::istreambuf_iterator<char>(ifs) ),
                       (std::istreambuf_iterator<char>()    ) );
    
    std::string scriptFileName = "/home/greg/projects/regTest/scripts/t22-greater-than";
    std::ifstream ifq(scriptFileName);
    std::string queryStr ( (std::istreambuf_iterator<char>(ifq) ),
                       (std::istreambuf_iterator<char>()    ) );
    queryStr.clear();
    
    queryStr.append("Update customers set street2 = \"\" where street2 = \"NULL\"");
    
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
    printf("\n query parse success");
    
    cTable* table = parser->getTableByName((char*)query->queryTable->name.c_str());
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
        return 0;
    }

    
    printf("\n\n");
 
}