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
#include "sql.h"

using namespace std;

int main()
{

    sqlParser* parser = new sqlParser();
    sqlClassLoader* loader = new sqlClassLoader();

    parser->parse("SELECT top 5 * from customer where surname like ""sch""");
    
    loader->loadSqlClasses("dbDef.json","bike");
    
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