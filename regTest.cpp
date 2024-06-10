#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <string.h>
#include <iostream>
#include <filesystem>
#include <list>
#include "parseSQL.cpp"
#include "parseQuery.cpp"
#include "sqlEngine.cpp"
#include "plan.cpp"

namespace fs = std::filesystem;

    std::string scriptPath = "/home/greg/projects/regTest/scripts";
    std::string resultPath = "/home/greg/projects/regTest/results";
    std::string proofPath = "/home/greg/projects/regTest/proofs";

string runScript(shared_ptr<sqlParser> _sql, string script)
{
    //Reads jason database description and load sql classes for the sqlEngine

    string scriptFileName = scriptPath + "/" + script;

    std::ifstream ifq(scriptFileName);              
    std::string queryStr ( (std::istreambuf_iterator<char>(ifq) ),
                       (std::istreambuf_iterator<char>()    ) );

    presentationType = PRESENTATION::TEXT;
    
    unique_ptr<Plan> plan ( new Plan(_sql->isqlTables));
    plan->prepare((char*)queryStr.c_str());
    plan->execute();

    string returnString;
    returnString.append(returnResult.resultTable);
    returnString.append(msgText);
    returnString.append(errText);
    return returnString;
}

int main()
{
    std::string sqlFileName = "bike.sql";
    std::ifstream ifs(sqlFileName);
    std::string sqlFile ( (std::istreambuf_iterator<char>(ifs) ),
                       (std::istreambuf_iterator<char>()    ) );

    shared_ptr<sqlParser> sql (new sqlParser((char*)sqlFile.c_str()));

    if(sql->parse() == ParseResult::FAILURE)
    {
        fprintf(traceFile,"\n sql=%s",sqlFile.c_str());
        fprintf(traceFile,"\n %s",errText.c_str());
        return 0;
    }

    string fileName;
    string result;
    string proof;

    std::list<string> scripts;

    for (const auto & entry : fs::directory_iterator(scriptPath))
    {
        fileName = entry.path().filename();
        scripts.push_back(fileName);
    }
    scripts.sort();
 
    for (string name : scripts)
    {
        runScript(sql,name);
    }

    for (string name : scripts)
    {
        ifstream inStream;
        inStream.open (resultPath + "/" + name);
        inStream >> result;
        inStream.close();

        inStream.open (proofPath + "/" + name);
        inStream >> proof;
        inStream.close();

        if(result.compare(proof) != 0)
            fprintf(traceFile,"\n %s failed test", name.c_str());
    }

}