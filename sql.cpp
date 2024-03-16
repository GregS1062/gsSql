#include <fstream>
#include <string.h>
#include "conversion.h"
#include "sqlParser.h"
int main()
{
    printf("\n customers %d",sizeof(Customers));
    printf("\n Stores    %d",sizeof(Stores));
    printf("\n Products  %d",sizeof(Products));
    printf("\n Orders    %d",sizeof(Orders));
    printf("\n Items     %d",sizeof(Items));
    

    std::string scriptFileName = "/home/greg/projects/gsSql/bike.sql";
    
     std::ifstream ifs(scriptFileName);
    std::string sql ( (std::istreambuf_iterator<char>(ifs) ),
                       (std::istreambuf_iterator<char>()    ) );
    
    sqlParser* parser = new sqlParser((char*)sql.c_str());
    if(parser->parse() == ParseResult::FAILURE)
    {
        printf("\n error %s",errText.c_str());
        return 0;
    };
    printf("\n\n");
    map<char*,column*>::iterator itr;
    column* col;
    for(cTable* table : parser->tables)
    {
        printf("\n table name %s",table->name.c_str());
       // printf(" at ");
       // printf(table->fileName.c_str());
        printf("  %d",table->recordLength);
        map<char*,column*>columns = table->columns;
        for (itr = columns.begin(); itr != columns.end(); ++itr) 
        {
            col = (column*)itr->second;
            printf("\n\t\tname %s",col->name.c_str());
            printf(" length %d",col->length);
        }
        
    }
    printf("\n\n");
    return 0;
}