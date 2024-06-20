#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <string.h>
#include "parseSQL.cpp"
#include "parseQuery.cpp"
#include "binding.cpp"
#include "plan.cpp"
#include "printDiagnostics.cpp"

using namespace std;


int main(int argc, char* argv[])
{ 
    try
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
                    script = "t55-insert-items-into-values"; 
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
                    script = "t100-functions-table-column-alias";
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
                    script = "t200-Join-Orders-On-Customers";
                    break;
                case 22:
                    script = "t201-Complex-Join";
                    break;
                case 23:
                    script = "t202-Group-By";
                    break;
                case 24:
                    script = "t203-count-asterisk";
                    break;
                case 25:
                    script = "test";
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
        
        auto sql = make_unique<parseSql>(sqlFile);

        if(sql->parse() == ParseResult::FAILURE)
        {
            fprintf(traceFile,"\n******************SQL Parser FAILED*****************");
            fprintf(traceFile,"\n %s",errText.c_str());
            return 0;
        }

        debug = false;

        Plan plan(sql->isqlTables);
        
        plan.prepare(queryStr);
       
        debug = true;

        for(shared_ptr<Statement> statement : plan.lstStatements)
            printStatement(statement);

        fprintf(traceFile,"\n\n");
        fclose(traceFile);

        if(!errText.empty())
            fprintf(traceFile,"\nERROR %s",errText.c_str());
        
        return 0; 
        }
    catch(const std::exception& e)
    {
        fprintf(traceFile,"\n\nEXCEPTION %s",e.what());

    }
}
