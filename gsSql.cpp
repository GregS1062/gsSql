#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <string.h>
#include "keyValue.h"
#include "print.h"
#include "parseJason.h"
#include "parseSql.h"
#include "sqlClassLoader.h"
#include "sqlEngine.h"
#include "global.h"

using namespace std;

int main()
{
    //Reads jason database description and load sql classes for the sqlEngine
    sqlClassLoader* loader = new sqlClassLoader();
    loader->loadSqlClasses("dbDef.json","bike");

    return 0;

    sqlParser* parser = new sqlParser();
    const char * sql = "SELECT top 5 * from customer where surname like ""sch""";
    if(parser->parse(sql) == ParseResult::FAILURE)
    {
        printf("%s",errText.c_str());
        return 0;
    }
    
    ctable* table = loader->getTableByName((char*)"customer");
    if(table == nullptr)
    {
        printf("\n table not found");
        return 0;
    }
    
    sqlEngine* engine = new sqlEngine(parser,table);
    if(engine->open() == ParseResult::SUCCESS)
    {
        engine->selectQueryColumns();
        string output = engine->fetchData();
        printf("\n %s", output.c_str());
    }
    
    printf("\n\n");
 
}