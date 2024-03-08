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
    std::string sql = "SELECT top 10 * from customer";
    
    //std::string sql = "SELECT top 10 * from customer where surname = ""schiller""";
   
   // const char * sql = "INSERT INTO customer (deleted, custid, givenname, middleinitial, surname, phone, email, street1, street2, city, state, country, zipcode) VALUES ()"; 

   /* std::string scriptFileName = "/home/greg/projects/regTest/scripts/t70-Update";
    
     std::ifstream ifs(scriptFileName);
    std::string sql ( (std::istreambuf_iterator<char>(ifs) ),
                       (std::istreambuf_iterator<char>()    ) );
    */
    printf("\n sql=%s \n",sql.c_str());

    if(parser->parse(sql.c_str(),loader) == ParseResult::FAILURE)
    {
        printf("\n sql=%s",sql.c_str());
        printf("\n %s",errText.c_str());
        return 0;
    }

    printf("\n parse success");
    
    cTable* table = loader->getTableByName((char*)"customer");
    if(table == nullptr)
    {
        printf("\n table not found");
        return 0;
    }
    
    sqlEngine* engine = new sqlEngine(parser,table);
    if(engine->open() == ParseResult::SUCCESS)
    {
        printf("\n engine opened");
        if(parser->sqlAction == SQLACTION::SELECT)
        {
            string output = engine->select();
            printf("\n %s", output.c_str());
            return 0;
        }
        if(parser->sqlAction == SQLACTION::UPDATE)
        {
            engine->update();
            printf("\n %s", errText.c_str());
            printf("\n %s", returnResult.message.c_str());
            return 0;
        }

    }
    
    printf("\n\n");
 
}