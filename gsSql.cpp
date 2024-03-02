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

    sqlParser* parser = new sqlParser();
   // const char * sql = "SELECT top 5 * from customer where surname like ""sch""";
   // const char * sql = "INSERT INTO customer (deleted, custid, givenname, middleinitial, surname, phone, email, street1, street2, city, state, country, zipcode) VALUES ()"; 

    std::string scriptFileName = "/home/greg/projects/regTest/scripts/t60-insert-columns-and-values";
    
     std::ifstream ifs(scriptFileName);
    std::string sql ( (std::istreambuf_iterator<char>(ifs) ),
                       (std::istreambuf_iterator<char>()    ) );

    if(parser->parse(sql.c_str()) == ParseResult::FAILURE)
    {
        printf("\n sql=%s",sql.c_str());
        printf("\n %s",errText.c_str());
        return 0;
    }
    
    cTable* table = loader->getTableByName((char*)"customer");
    if(table == nullptr)
    {
        printf("\n table not found");
        return 0;
    }
    
    sqlEngine* engine = new sqlEngine(parser,table);
    if(engine->open() == ParseResult::SUCCESS)
    {
        engine->ValidateQueryColumns();
        string output = engine->fetchData();
        printf("\n %s", output.c_str());
    }
    
    printf("\n\n");
 
}