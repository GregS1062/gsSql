#include <fstream>
#include <string.h>
#include "sqlParser.h"
int main()
{

    std::string scriptFileName = "/home/greg/projects/gsSql/Bike.sql";
    
     std::ifstream ifs(scriptFileName);
    std::string sql ( (std::istreambuf_iterator<char>(ifs) ),
                       (std::istreambuf_iterator<char>()    ) );
    
    sqlParser* parser = new sqlParser((char*)sql.c_str());
    parser->parse();
    return 0;
}