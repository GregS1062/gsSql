#include "prepareQuery.cpp"
#include <iostream>
using namespace std;
int main(int argc, char* argv[])
{ 
    if(argc < 2)
    {
        cout << "empty" << std::endl;
        return 0;
    }
    string _str = argv[1];
    if(_str.empty())
    {
        cout << "empty" << std::endl;
        return 0;
    }
    /*
        Column name have the following forms

        case 1) column alias        - surname AS last
        case 2) functions           - COUNT(), SUM(), AVG(), MAX(), MIN()
        case 3) simple name         - surname
        case 4) table/column        - customers.surname
        case 5) table alias column  - c.surname from customers c
    */

        _str = trim(_str);
        shared_ptr<columnParts> parts = make_unique<columnParts>(); 
        parts->fullName =_str;

        size_t columnAliasPosition = findKeyword(_str,sqlTokenAs);

        //case 1) column alias        - surname AS last
        string columnName  = _str;
        if(columnAliasPosition != std::string::npos)
        {
            parts->columnAlias = snipString(_str,columnAliasPosition+3);
            _str = _str.substr(0,columnAliasPosition);
        }

        // case 2) functions          - COUNT(), SUM(), AVG(), MAX(), MIN()
        size_t posParen = _str.find(sqlTokenOpenParen);
        if(posParen != std::string::npos)
        {
            parts->function = clipString(_str,posParen);
            columnName = snipString(_str,posParen+1); 
            columnName.erase(columnName.find_last_of(CLOSEPAREN));
        }
        else
            columnName = _str;

        if(columnName.empty())
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Column name is empty");
            return nullptr;
        }
        size_t posPeriod = columnName.find(sqlTokenPeriod);
        if(posPeriod == std::string::npos)
        {
             // case 3) simple name         - surname
            parts->columnName = columnName;
        }
        else
        {
            // case 4/5)
            parts->tableAlias = columnName.substr(0,posPeriod);
            parts->columnName = snipString(columnName,posPeriod+1);
        }



            fprintf(traceFile,"\nColumn parts: \nfunction:%s \nname:%s \nalias:%s \ntable Alias %s\n",parts->function.c_str(), parts->columnName.c_str(), parts->columnAlias.c_str(), parts->tableAlias.c_str());


}