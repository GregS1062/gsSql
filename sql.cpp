#include <fstream>
#include <string.h>
#include "conversion.h"
#include "parseSQL.cpp"
int main()
{
    fprintf(traceFile,"\n customers %d",sizeof(Customers));
    fprintf(traceFile,"\n Stores    %d",sizeof(Stores));
    fprintf(traceFile,"\n Products  %d",sizeof(Products));
    fprintf(traceFile,"\n Orders    %d",sizeof(Orders));
    fprintf(traceFile,"\n Items     %d",sizeof(Items));
    

    std::string scriptFileName = "/home/greg/projects/gsSql/bike.sql";
    
     std::ifstream ifs(scriptFileName);
    std::string sql ( (std::istreambuf_iterator<char>(ifs) ),
                       (std::istreambuf_iterator<char>()    ) );
    
    sqlParser* parser = new sqlParser((char*)sql.c_str());
    if(parser->parse() == ParseResult::FAILURE)
    {
        fprintf(traceFile,"\n error %s",errText.c_str());
        return 0;
    };
    fprintf(traceFile,"\n\n");
    map<char*,column*>::iterator itr;
    column* col;
    for(sTable* table : parser->tables)
    {
        fprintf(traceFile,"\n\n----------------------------------------");
        fprintf(traceFile,"\n table name %s",table->name);
        fprintf(traceFile," at ");
        fprintf(traceFile,table->fileName.c_str());
        fprintf(traceFile,"  %d",table->recordLength);
        map<char*,column*>columns = table->columns;
        for (itr = columns.begin(); itr != columns.end(); ++itr) 
        {
            col = (column*)itr->second;
            fprintf(traceFile,"\n\t\t\tname %s",col->name);
            fprintf(traceFile," length %d",col->length);
            if(col->primary)
                fprintf(traceFile,"\t PRIMARY");
        }
        for(sIndex* index : table->indexes)
        {
            fprintf(traceFile,"\n\t index name %s",index->name);
            fprintf(traceFile," at ");
            fprintf(traceFile,index->fileName.c_str());
            map<char*,column*>columns = index->columns;
            for (itr = columns.begin(); itr != columns.end(); ++itr) 
            {
                col = (column*)itr->second;
                fprintf(traceFile,"\n\t\t\tname %s",col->name);
                fprintf(traceFile," length %d",col->length);
            }
        }

        
    }
    fprintf(traceFile,"\n\n");
    return 0;
}