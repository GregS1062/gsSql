#pragma once
#include <string>
#include "sqlClassLoader.h"

using namespace std;

#define sqlTokenCreate  "CREATE"
#define sqlTokenDelete  "DELETE"
#define sqlTokenFrom    "FROM"
#define sqlTokenSelect  "SELECT"
#define sqlTokenUpdate  "UPDATE"

enum class SQLACTION{
    NOACTION,
    INVALID,
    CREATE,
    SELECT,
    UPDATE,
    DELETE
};

class tokens
{
    public:
    char* token;
    tokens* next = nullptr;
};

class sqlParser
{
    public:
    tokens* columnHead = nullptr;
    tokens* columnTail = nullptr;
    tokens* tableHead = nullptr;
    tokens* tableTail = nullptr;

    ParseResult parse(const char*);
    ParseResult addToken(char*);
    bool        isToken(char*);
    bool        isColumn = false;
    bool        isTable  = false;
    tokens*     getNextColumnToken(tokens*);
    tokens*     getNextTableToken(tokens*);
    ParseResult determineAction(char*);


    SQLACTION sqlAction = SQLACTION::NOACTION;
};
ParseResult sqlParser::determineAction(char* _token)
{

    if(strcmp(_token,sqlTokenSelect) == 0)
    {
        sqlAction = SQLACTION::SELECT;
        return ParseResult::SUCCESS;
    }
    
    if(strcmp(_token,sqlTokenUpdate) == 0)
    {
        sqlAction = SQLACTION::UPDATE;
        return ParseResult::SUCCESS;
    }

    if(strcmp(_token,sqlTokenDelete) == 0)
    {
        sqlAction = SQLACTION::DELETE;
        return ParseResult::SUCCESS;
    }

    if(strcmp(_token,sqlTokenCreate) == 0)
    {
        sqlAction = SQLACTION::CREATE;
        return ParseResult::SUCCESS;
    }

    sqlAction = SQLACTION::INVALID;
    return ParseResult::FAILURE;
}
ParseResult sqlParser::addToken(char* _token)
{
    tokens* tok = new tokens();
    tok->token = (char*)malloc(strlen(_token));
    strcpy(tok->token,_token);

    //This is the first token and must be the sql verb
    if(sqlAction == SQLACTION::NOACTION)
    {
        if(determineAction(_token) != ParseResult::SUCCESS)
                return ParseResult::FAILURE;

        isColumn = true;
        return ParseResult::SUCCESS;
    }

    if(strcmp(_token,sqlTokenFrom) == 0)
    {
        isColumn = false;
        isTable = true;
        return ParseResult::SUCCESS;
    }

    if(isColumn)
    {
        if(columnHead == nullptr)
        {
            columnHead = tok;
            columnTail = tok;
            return ParseResult::SUCCESS;
        }
        else
        {
            tokens* temp = columnTail;
            columnTail = tok;
            temp->next = tok;
            return ParseResult::SUCCESS;
        }
    }

    if(isTable)
    {
        if(tableHead == nullptr)
        {
            tableHead = tok;
            tableTail = tok;
            return ParseResult::SUCCESS;
        }
        else
        {
            tokens* temp = tableTail;
            tableTail = tok;
            temp->next = tok;
            return ParseResult::SUCCESS;
        }
    }


    return ParseResult::FAILURE;
}
tokens* sqlParser::getNextColumnToken(tokens* _token)
{
    try
    {
        if(_token == nullptr)
        {
            return columnHead;
        }
        return _token->next;
    }
    catch(const std::exception& e)
    {
        errText.append( e.what());
        return nullptr;
    } 
}
tokens* sqlParser::getNextTableToken(tokens* _token)
{
    try
    {
        if(_token == nullptr)
        {
            return tableHead;
        }
        return _token->next;
    }
    catch(const std::exception& e)
    {
        errText.append( e.what());
        return nullptr;
    } 
}
bool sqlParser::isToken(char* _token)
{
    tokens* tok = columnHead;
    while(tok != nullptr)
    {
        if(strcmp(tok->token,_token) == 0)
            return true;
        tok = tok->next;
    }
    return false;
}

ParseResult sqlParser::parse(const char* _sql)
{
    bool isToken = false;
    char token[MAXSQLTOKENSIZE];            //token buffer space            
    char c = ' ';                           //character in question
    int  t = 0;                             //token character pointer
    size_t sqlStringLength = strlen(_sql);

    //read string loop
    for (size_t i = 0;i< sqlStringLength;i++)
    {
        c = _sql[i];
        //whitespace
        if( c == SPACE
         || c == COMMA
         || c == NEWLINE
         || c == TAB)
         {
            if(!isToken)
                continue;

            if(isToken)
            {
               isToken = false;
               token[t] = '\0';

               if(addToken(token) != ParseResult::SUCCESS)
                    return ParseResult::FAILURE;

               token[0] = '\0';
               t = 0;
               continue;
            }
        }
        isToken = true;
        token[t] = c;
        t++;
    }
    if(t>0)
    {
        token[t] = '\0';

        if(addToken(token) != ParseResult::SUCCESS)
            return ParseResult::FAILURE;
    }
    return ParseResult::SUCCESS;
}




 
