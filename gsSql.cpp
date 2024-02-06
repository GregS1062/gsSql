#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <string.h>
#include "keyValue.h"
#include "print.h"
#include "tokenizer.h"
#include "parseJason.h"
#include "sqlClassLoader.h"
#include "global.h"
#include "sql.h"

using namespace std;

int main()
{

    loadSqlClasses("dbDef.json","bike");

    table* t = tableHead;
    while(t != nullptr)
    {
        printf("\n table %s",t->name.c_str());
        printf(" recordlength %d",t->recordLength);
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

  
    printf("\n");
    return 0;

    printf("\n\n"); 
    string htmlRequest = "select";
    string htmlResponse;
    printf("\n\n Executed = %s", htmlResponse.c_str());
}