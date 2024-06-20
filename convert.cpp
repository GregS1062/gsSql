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
#include "dataAccess.h"

using namespace std;

char* line;
fstream* fileStream;
int recordLength;
long filePosition = 0;
enum LOADING
{
    CUSTOMERS,
    STORES,
    ORDERS,
    ITEMS,
    PRODUCTS
};

string clean(char* _str)
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
string parseDate(tm _date)
{
    string strOut;
    if(_date.tm_year < 1
    || _date.tm_year > 2024)
    {
        strOut.append("01/01/1900");
        return strOut;
    }
	//Note the tm structure does some weird things to dates.
	// Month is zero based
	// Year is the years after 1900
	char str[11] = "";
    char s[11];
	sfprintf(traceFile,str,"%d", _date.tm_mon + 1);

	strcpy(s, utilities::padLeft(str, 2));

	strcat(s, "/");

	sfprintf(traceFile,str,"%d", _date.tm_mday);

	strcat(s, utilities::padLeft(str, 2));

	strcat(s, "/");

	sfprintf(traceFile,str, "%d", _date.tm_year + 1900);

	strcat(s, str);

    strOut.append(s);
    return strOut;
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
string getItemQueryString()
{
    string queryString;
    Items* c = (Items*)DataAccess::getRecord(filePosition,fileStream,sizeof(Items));
    if (c == nullptr)
        return "";
    queryString.append("insert into items values (0,");
    queryString.append(" \"");
    queryString.append(clean(c->Order_Number));
    queryString.append("\", \"");
    queryString.append(clean(c->Product_Number));
    queryString.append("\", ");
    queryString.append(parseInt(c->Quantity));
    queryString.append(", ");
    queryString.append(parseDouble(c->Price));
    queryString.append(", ");
    queryString.append(parseDouble(c->Discount));
    queryString.append(", ");
    queryString.append(parseDouble(c->Total));
    queryString.append(")");

    return queryString;
}
string getOrderQueryString()
{
    string queryString;
    Orders* c = (Orders*)DataAccess::getRecord(filePosition,fileStream,sizeof(Orders));
    if (c == nullptr)
        return "";
    queryString.append("insert into orders values (0,");
    queryString.append(parseInt(c->Status));
    queryString.append(", \"");
    queryString.append(clean(c->Order_Number));
    queryString.append("\", \"");
    queryString.append(clean(c->Customer_ID));
    queryString.append("\", \"");
    queryString.append(parseDate(c->OrderDate));
    queryString.append("\", \"");
    queryString.append(parseDate(c->DueDate));
    queryString.append("\", \"");
    queryString.append(parseDate(c->ShipDate));
    queryString.append("\", ");
    queryString.append(parseDouble(c->Tax));
    queryString.append(", ");
    queryString.append(parseDouble(c->Freight));
    queryString.append(", ");
    queryString.append(parseDouble(c->TotalDue));
    queryString.append(")");

    return queryString;
}
string getProductQueryString()
{
    string queryString;
    Products* c = (Products*)DataAccess::getRecord(filePosition,fileStream,sizeof(Products));
    if (c == nullptr)
        return "";
    queryString.append("insert into products values (");
    queryString.append("0,\"");
    queryString.append(clean(c->Product_Number));
    queryString.append("\", \"");
    queryString.append(clean(c->Description));
    queryString.append("\", \"");
    queryString.append(clean(c->Color));
    queryString.append("\", \n \"");
    queryString.append(clean(c->Size));
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
    Stores* c = (Stores*)DataAccess::getRecord(filePosition,fileStream,sizeof(Stores));
    if (c == nullptr)
        return "";
    queryString.append("insert into stores values (");
    queryString.append("0,\"");
    queryString.append(clean(c->Customer_ID));
    queryString.append("\", \"");
    queryString.append(clean(c->Name));
    queryString.append("\", \"");
    queryString.append(clean(c->Phone));
    queryString.append("\", \"");
    queryString.append(clean(c->EMAIL));
    queryString.append(" \",\n \"");
    queryString.append(clean(c->Address.Street1));
    queryString.append(" \", \"");
    queryString.append(clean(c->Address.Street2));
    queryString.append(" \", \"");
    queryString.append(clean(c->Address.City));
    queryString.append("\", \"");
    queryString.append(clean(c->Address.State));
    queryString.append("\", \"");
    c->Address.Country[2] = '\0';
    queryString.append(clean(c->Address.Country));
    queryString.append("\", \"");
    queryString.append(clean(c->Address.ZipCode));
    queryString.append("\" )");
    return queryString;
}
string getCustomersQueryString()
{
    string queryString;
    Customers* c = (Customers*)DataAccess::getRecord(filePosition,fileStream,sizeof(Customers));
    if (c == nullptr)
        return "";
    queryString.append("insert into customers values (");
    queryString.append("0,\"");
    queryString.append(clean(c->Customer_ID));
    queryString.append("\", \"");
    queryString.append(clean(c->Person.GivenName));
    queryString.append("\", \"");
    queryString.append(clean(c->Person.MiddleInitial));
    queryString.append("\", \"");
    queryString.append(clean(c->Person.SurName));
    queryString.append("\", \"");
    queryString.append(clean(c->Person.Phone));
    queryString.append("\", \"");
    queryString.append(clean(c->Person.EMAIL));
    queryString.append(" \",\n \"");
    queryString.append(clean(c->Address.Street1));
    queryString.append(" \", \"");
    queryString.append(clean(c->Address.Street2));
    queryString.append(" \", \"");
    queryString.append(clean(c->Address.City));
    queryString.append("\", \"");
    queryString.append(clean(c->Address.State));
    queryString.append("\", \"");
    c->Address.Country[2] = '\0';
    queryString.append(clean(c->Address.Country));
    queryString.append("\", \"");
    queryString.append(clean(c->Address.ZipCode));
    queryString.append("\" )");
    return queryString;
}

int convert(LOADING _loading)
{
    string  fileName;
    filePosition = 0;
    char*   tableName;
    switch(_loading)
    {
        case LOADING::CUSTOMERS:
        {
            fileName        = "/home/greg/projects/bikeData/Customers.dat";
            recordLength    = sizeof(Customers);
            tableName       = (char*)"Customers";
            break;
        }
        case LOADING::STORES:
        {
            fileName        = "/home/greg/projects/bikeData/Stores.dat";
            recordLength    = sizeof(Stores);
            tableName       = (char*)"Stores";
            break;
        }
        case LOADING::ORDERS:
        {
            fileName        = "/home/greg/projects/bikeData/Orders.dat";
            recordLength    = sizeof(Orders);
            tableName       = (char*)"Orders";
            break;
        }
        case LOADING::ITEMS:
        {
            fileName        = "/home/greg/projects/bikeData/Items.dat";
            recordLength    = sizeof(Items);
            tableName       = (char*)"Items";
            break;
        }
        case LOADING::PRODUCTS:
        {
            fileName        = "/home/greg/projects/bikeData/Products.dat";
            recordLength    = sizeof(Products);
            tableName       = (char*)"Products";
            break;
        }
    };
    fileStream = new fstream{};
    fileStream->open(fileName, ios::in | ios::out | ios::binary);
	if (!fileStream->is_open()) 
    {
        fprintf(traceFile,"\n %s open failed",fileName);
        return 0;
    }

    line = (char*)malloc(recordLength);
    
    std::string sqlFileName = "convert.sql";
    std::ifstream ifs(sqlFileName);
    std::string sql ( (std::istreambuf_iterator<char>(ifs) ),
                       (std::istreambuf_iterator<char>()    ) );

    sqlParser* parser = new sqlParser((char*)sql.c_str());

    if(parser->parse() == ParseResult::FAILURE)
    {
        fprintf(traceFile,"\n sql=%s",sql.c_str());
        fprintf(traceFile,"\n %s",errText.c_str());
        return 0;
    }

    sqlEngine* engine = new sqlEngine();
    queryParser* query = new queryParser();

    string queryStr;
    cTable* table = parser->getTableByName(tableName);
    if(table == nullptr)
    {
        fprintf(traceFile,"\n table not found");
        return 0;
    }
    int count = 0;
    while(true)
    {
        count++;
        queryStr.clear();
        switch(_loading)
        {
            case LOADING::CUSTOMERS:
            {
                queryStr = getCustomersQueryString();
                break;
            }
            case LOADING::STORES:
            {
                queryStr = getStoresQueryString();
                break;
            }
            case LOADING::ORDERS:
            {
                queryStr = getOrderQueryString();
                break;
            }
            case LOADING::ITEMS:
            {
                queryStr = getItemQueryString();
                break;
            }
            case LOADING::PRODUCTS:
            {
                queryStr = getProductQueryString();
                break;
            }
        }
        
        if(queryStr.length() == 0)
            break;

        fprintf(traceFile,"\n %s",queryStr.c_str());

        engine = new sqlEngine();
        query = new queryParser();
        if(query->parse((char*)queryStr.c_str(),parser) == ParseResult::FAILURE)
        {
            fprintf(traceFile,"\n query parse failed");
            fprintf(traceFile,"\n error %s",errText.c_str());
            fprintf(traceFile,"\n\n");
            fprintf(traceFile,"%s",queryStr.c_str());
            fprintf(traceFile,"\n\n");
            return 0;
        };
        

        engine->prepare(query,table);
        if(engine->open() == ParseResult::FAILURE)
        {
            fprintf(traceFile,"\n engine open failed");
            fprintf(traceFile,"\n %s",errText.c_str());
            return 0;
        }
        engine->insert();
        engine->close();
        free (engine);
        free(query);
        filePosition = filePosition+recordLength;
    }
    
    fileStream->close();
    fprintf(traceFile,"\n\n %s count=%d\n ",tableName,count);
    return 0;
}

int main()
{
    convert(LOADING::CUSTOMERS);
    convert(LOADING::STORES);
    convert(LOADING::ORDERS);
    convert(LOADING::ITEMS);
    convert(LOADING::PRODUCTS);
}