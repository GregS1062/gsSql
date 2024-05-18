#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <string.h>
#include "parseSQL.cpp"
#include "lookup.cpp"
#include "plan.cpp"


using namespace std;


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
            case 21:
                script = "t200-Join-Books-on-Author";
                break;
            case 22:
                script = "t201-Join-Books-on-Translator";
                break;
            case 23:
                script = "t202-Group-By";
                break;
            case 24:
                script = "t203-count-asterisk";
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
    std::string sqlFile ( (std::istreambuf_iterator<char>(ifs) ),
                       (std::istreambuf_iterator<char>()    ) );
    
    std::ifstream ifq(scriptFileName);
    std::string queryStr ( (std::istreambuf_iterator<char>(ifq) ),
                       (std::istreambuf_iterator<char>()    ) );

    
    fprintf(traceFile,"\n query=%s \n",queryStr.c_str());

    presentationType = PRESENTATION::TEXT;
    
    Plan* plan = new Plan();
    sqlParser* sql = new sqlParser((char*)sqlFile.c_str(),plan->isqlTables);

    if(sql->parse() == ParseResult::FAILURE)
    {
        fprintf(traceFile,"\n sql=%s",sqlFile.c_str());
        fprintf(traceFile,"\n %s",errText.c_str());
        delete sql;
        delete plan;
        return 0;
    }

    debug = true;

    plan->prepare((char*)queryStr.c_str());
    plan->execute();
    delete sql;
    delete plan;

    fprintf(traceFile,"\n %s",returnResult.resultTable.c_str());
    fprintf(traceFile,"\n %s",msgText.c_str());
    fprintf(traceFile,"\n %s",errText.c_str());
        
    fprintf(traceFile,"\n\n");
    fclose(traceFile);
    return 0; 
}
