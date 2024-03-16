#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <iostream>
#include <string.h>
#include <bitset>
#include "conversion.h"
#include "sqlParser.h"
#include "queryParser.h"
#include "sqlEngine.h"
#include "global.h"
#include "utilities.h"

using namespace std;

char* line;
fstream* fileStream;
int recordLength;
long filePosition = 0;
char* getRecord(long _address, fstream* _file, int _size)
{
	try
	{
		if (_size < 1)
		{
			return nullptr;
		}

		char* ptr = (char*)malloc(_size);

		_file->clear();

		if (!_file->seekg(_address))
		{
			free(ptr);
			return nullptr;
		}

		if (!_file->read((char*)ptr, _size))
			ptr = nullptr;
		
		
		_file->flush();

		return ptr;
	}
    catch(const std::exception& e)
    {
        return nullptr;
    } 
}
void dump()
{
    Customers* c = (Customers*)getRecord(filePosition,fileStream,sizeof(Customers));
    printf("\n-------------------------------------------------");
    printf("\n %s",c->Customer_ID);
    printf("\n %s",c->Person.GivenName);
    printf("\n %s",c->Person.MiddleInitial);
    printf("\n %s",c->Person.SurName);
    printf("\n %s",c->Person.Phone);
    printf("\n %s",c->Person.EMAIL);
    printf("\n %s",c->Address.Street1);
    printf("\n %s",c->Address.Street2);
    printf("\n %s",c->Address.City);
    printf("\n %s",c->Address.State);
    printf("\n %s",c->Address.Country);
    printf("\n %s",c->Address.ZipCode);
    return;
}
string notNull(char* _str)
{
    string rString;
    if(_str == nullptr)
        return "NULL";
    char c;
    for(size_t i = 0; i < strlen(_str);i++)
    {
        c = _str[i];
        if(c == 32              // space
        || c == 46              // .
        || c == 64              // @
        || (c > 47 && c < 58)   // number
        || (c > 64 && c < 91)   // upper case alpha
        || (c > 96 && c < 123)  // lower case alpha
        )
        {
            //do nothin
        }
        else
        {
            _str[i] = ' ';
        }
    };
    rString.append(_str);
    if(rString.length() == 0)
        rString.append(" ");
    return rString;
}
string parseDate(tm dt)
{
    string out;
    if(dt.tm_mon < 1)
    {
        out.append("01");
    }
    else{
        out.append(std::to_string(dt.tm_mon));
    }
    out.append("/");
    if(dt.tm_mday < 1)
    {
        out.append("01");
    }
    else{
        out.append(std::to_string(dt.tm_mday));
    }
    out.append("/");
        if(dt.tm_year < 1)
    {
        out.append("1900");
    }
    else{
        out.append(std::to_string(dt.tm_year));
    }
    return out;
}
string parseDouble(double dbl)
{
    std::stringstream ss;
    if(dbl < 0){
    ss << "-" << std::fixed << std::setprecision(2) << -dbl; 
    } else {
    ss << std::fixed << std::setprecision(2) << dbl;
    
    }
    return ss.str();
}
string parseInt(int i)
{
    string out;
    if(i == 0)
    {
        out.append("0");
    }
    else{
        out.append(std::to_string(i));
    }
    return out;
}
string getProductQueryString()
{
    string queryString;
    Products* c = (Products*)getRecord(filePosition,fileStream,sizeof(Products));
    if (c == nullptr)
        return "";
    queryString.append("insert into products values (");
    queryString.append("0,\"");
    queryString.append(notNull(c->Product_Number));
    queryString.append("\", \"");
    queryString.append(notNull(c->Description));
    queryString.append("\", \"");
    queryString.append(notNull(c->Color));
    queryString.append("\", \n \"");
    queryString.append(notNull(c->Size));
    queryString.append(" \", ");
    queryString.append(parseInt(c->SafetyStockLevel));
    queryString.append(", ");
    queryString.append(parseInt(c->ReorderPoint));
    queryString.append(", ");
    queryString.append(parseDouble(c->StandardCost));
    queryString.append(", ");
    queryString.append(parseDouble(c->ListPrice));
    queryString.append(", \"");
    queryString.append(parseDate(c->SellStartDate));
    queryString.append("\", \"");
    queryString.append(parseDate(c->SellEndDate));
    queryString.append("\" )");

        return queryString;
}

string getStoresQueryString()
{
    string queryString;
    Stores* c = (Stores*)getRecord(filePosition,fileStream,sizeof(Stores));
    if (c == nullptr)
        return "";
    queryString.append("insert into stores values (");
    queryString.append("0,\"");
    queryString.append(notNull(c->Customer_ID));
    queryString.append("\", \"");
    queryString.append(notNull(c->Name));
    queryString.append("\", \"");
    queryString.append(notNull(c->Phone));
    queryString.append("\", \"");
    queryString.append(notNull(c->EMAIL));
    queryString.append(" \",\n \"");
    queryString.append(notNull(c->Address.Street1));
    queryString.append(" \", \"");
    queryString.append(notNull(c->Address.Street2));
    queryString.append(" \", \"");
    queryString.append(notNull(c->Address.City));
    queryString.append("\", \"");
    queryString.append(notNull(c->Address.State));
    queryString.append("\", \"");
    c->Address.Country[2] = '\0';
    queryString.append(notNull(c->Address.Country));
    queryString.append("\", \"");
    queryString.append(notNull(c->Address.ZipCode));
    queryString.append("\" )");
    return queryString;
}
string getCustomersQueryString()
{
    string queryString;
    Customers* c = (Customers*)getRecord(filePosition,fileStream,sizeof(Customers));
    if (c == nullptr)
        return "";
    queryString.append("insert into customers values (");
    queryString.append("0,\"");
    queryString.append(notNull(c->Customer_ID));
    queryString.append("\", \"");
    queryString.append(notNull(c->Person.GivenName));
    queryString.append("\", \"");
    queryString.append(notNull(c->Person.MiddleInitial));
    queryString.append("\", \"");
    queryString.append(notNull(c->Person.SurName));
    queryString.append("\", \"");
    queryString.append(notNull(c->Person.Phone));
    queryString.append("\", \"");
    queryString.append(notNull(c->Person.EMAIL));
    queryString.append(" \",\n \"");
    queryString.append(notNull(c->Address.Street1));
    queryString.append(" \", \"");
    queryString.append(notNull(c->Address.Street2));
    queryString.append(" \", \"");
    queryString.append(notNull(c->Address.City));
    queryString.append("\", \"");
    queryString.append(notNull(c->Address.State));
    queryString.append("\", \"");
    c->Address.Country[2] = '\0';
    queryString.append(notNull(c->Address.Country));
    queryString.append("\", \"");
    queryString.append(notNull(c->Address.ZipCode));
    queryString.append("\" )");
    return queryString;
}

int main()
{
    filePosition = 0;
    string   fileName = "/home/greg/projects/bikeData/Products.dat";
    fileStream = new fstream{};
    fileStream->open(fileName, ios::in | ios::out | ios::binary);
	if (!fileStream->is_open()) 
    {
        printf("\n open failed");
        return 0;
    }

    recordLength = sizeof(Products);
    line = (char*)malloc(recordLength);

    std::string sqlFileName = "bike.sql";
    std::ifstream ifs(sqlFileName);
    std::string sql ( (std::istreambuf_iterator<char>(ifs) ),
                       (std::istreambuf_iterator<char>()    ) );

    sqlParser* parser = new sqlParser((char*)sql.c_str());

    if(parser->parse() == ParseResult::FAILURE)
    {
        printf("\n sql=%s",sql.c_str());
        printf("\n %s",errText.c_str());
        return 0;
    }

    sqlEngine* engine = new sqlEngine();
    queryParser* query = new queryParser();

    string queryStr;
    cTable* table = parser->getTableByName((char*)"Products");
    if(table == nullptr)
    {
        printf("\n table not found");
        return 0;
    }
    int count = 0;
    while(true)
    {
        count++;
        printf("\n count %d",count);
        //if(count > 50)
         //   break;
        queryStr.clear();
        queryStr = getProductQueryString();
        if(queryStr.length() == 0)
            break;
        //printf("\n\n %s",queryStr.c_str());

        engine = new sqlEngine();
        query = new queryParser();
        if(query->parse((char*)queryStr.c_str(),parser) == ParseResult::FAILURE)
        {
            printf("\n query parse failed");
            printf("\n error %s",errText.c_str());
            printf("\n\n");
            printf("%s",queryStr.c_str());
            printf("\n\n");
            return 0;
        };
        

        engine->prepare(query,table);
        if(engine->open() == ParseResult::FAILURE)
        {
            printf("\n engine open failed");
            return 0;
        }
        engine->insert();
        engine->close();
        free (engine);
        free(query);
        filePosition = filePosition+recordLength;
    }
    
    fileStream->close();
    printf("\n\n count %d\n ",count);

}