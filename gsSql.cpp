#include <stdio.h>
#include <stdlib.h>
#include <fstream>
#include <string.h>
#include "keyValue.h"
#include "print.h"
#include "tokenizer.h"
#include "parseJason.h"
#include "global.h"
#include "sql.h"

using namespace std;

int main()
{
    /* valueList* v;
    keyValue* result;
    keyValue* kv = parseDatabaseDefinition();
    result = getDatabaseEntity(kv, lit_database, "bike");
    result = getDatabaseEntity(result, lit_table, "customer");
    result = getDatabaseEntity(result, lit_column, "givenname");
    v = (valueList*)result->value;
    if(v->t_type == t_Object)
    {
        keyValue* nkv = (keyValue*)v->value;
        printf("\ntype:%s", nkv->key);
        v = (valueList*)nkv->value;
        if(v->t_type == t_string)
            printf("\nlength:%s", (char*)v->value);
    }

    printf("\n\n"); */
    string htmlRequest = "select";
    string htmlResponse;
    htmlResponse = parseSqlStatement(htmlRequest);
    printf("\n\n Executed = %s", htmlResponse.c_str());
}