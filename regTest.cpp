#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <string.h>
#include <iostream>
#include <filesystem>
#include <list>
#include "keyValue.h"
#include "print.h"
#include "parseJason.h"
#include "parseSql.h"
#include "sqlClassLoader.h"
#include "sqlEngine.h"
#include "global.h"

namespace fs = std::filesystem;

    std::string scriptPath = "/home/greg/projects/regTest/scripts";
    std::string resultPath = "/home/greg/projects/regTest/results";
    std::string proofPath = "/home/greg/projects/regTest/proofs";

        /*const int result = remove( "no-file" );
    if( result == 0 ){
        printf( "success\n" );  
    } else {
        printf( "%s\n", strerror( errno ) ); // No such file or directory
    }*/

bool runScript(string script)
{
    //Reads jason database description and load sql classes for the sqlEngine
    sqlClassLoader* loader = new sqlClassLoader();
    loader->loadSqlClasses("dbDef.json","bike");

    sqlParser* parser = new sqlParser();

    string scriptFileName = scriptPath + "/" + script;

    std::ifstream ifs(scriptFileName);
    std::string sql ( (std::istreambuf_iterator<char>(ifs) ),
                       (std::istreambuf_iterator<char>()    ) );

    if(parser->parse(sql.c_str()) == ParseResult::FAILURE)
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
        if(engine->ValidateQueryColumns() == ParseResult::SUCCESS)
        {
            string resultFileName = resultPath + "/" + script;
            ofstream resultFile;
            resultFile.open (resultFileName);
            resultFile << engine->fetchData();
            resultFile.close();
            return true;
        }
    }
    printf("\n %s", errText.c_str());
    return false;
}

int main()
{
    string fileName;
    string result;
    string proof;

    std::vector<string> scripts;
 
    for (const auto & entry : fs::directory_iterator(scriptPath))
    {
        fileName = entry.path().filename();
        runScript(fileName);
    }

    for (const auto & entry : fs::directory_iterator(scriptPath))
    {
        fileName = entry.path().filename();

        ifstream inStream;
        inStream.open (resultPath + "/" + fileName);
        inStream >> result;
        inStream.close();

        inStream.open (proofPath + "/" + fileName);
        inStream >> proof;
        inStream.close();

        if(result.compare(proof) != 0)
            printf("\n %s failed test", fileName.c_str());
    }



}