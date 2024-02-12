#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <string.h>
#include "keyValue.h"
#include "print.h"
#include "parseJason.h"
#include "parseSql.h"
#include "sqlClassLoader.h"
#include "global.h"
#include "sql.h"

using namespace std;

int main()
{

   /* sqlParser parser;
    parser.parse("select cust, givenname, surname, middlename, address from customer, store");
    printf("\n Columns \n");
    tokens* nextTok = parser.getNextColumnToken(nullptr);  //get first token (head)
    while( nextTok != nullptr)
    {
        printf("\n %s", nextTok->token);
        nextTok = parser.getNextColumnToken(nextTok);
    }
    printf("\n\n Tables \n");
    
    nextTok = parser.getNextTableToken(nullptr);  //get first token (head)
    while( nextTok != nullptr)
    {
        printf("\n %s", nextTok->token);
        nextTok = parser.getNextTableToken(nextTok);
    }
    printf("\n\n");
    */
    loadSqlClasses("dbDef.json","bike");

     table* t = tableHead;
  /* while(t != nullptr)
    {
        printf("\n table %s",t->name.c_str());
        printf("\n table location = %s", t->fileName.c_str());
        printf("\n recordlength %d",t->recordLength);
        column* c = t->columnHead;
        while(c != nullptr)
        {
            printf("\n\t %s",c->name.c_str());
            printf(" length %d",c->length);
            printf(" position %d",c->position);
            c = c->next;
        }
        t = t->next;
    }
    */ 
    
    

  // keyValue* jasonDef = parseJasonDatabaseDefinition("dbDef.json");
  //  printObject(jasonDef,0);
    printf("\n\n");
 
}