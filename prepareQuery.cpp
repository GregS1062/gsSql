#pragma once
#include <list>
#include "sqlCommon.h"
/*
    functions to simplify string handling and reduce clutter in code
*/
/******************************************************
 * Trim (spaces)
 ******************************************************/
string trim(string _token)
{
    _token.erase(0, _token.find_first_not_of(SPACE));
    _token.erase(_token.find_last_not_of(SPACE)+1);
    return _token;
}

/******************************************************
 * Find Delimiter char*
 ******************************************************/
size_t findKeyword(string _str, string _delimiter)
{
    try
    {
 
        if(_str.empty())
            return std::string::npos;

        char buff[MAXSQLSTRINGSIZE];
        bool betweenQuotes = false;

        //if the delimeter is enclosed in quotes, ignore it.
        // To do so, blank out everything between quotes in the search buffer
        for(size_t i = 0;i<_str.length();i++)
        {
            if(_str[i] == QUOTE)
            {
                if(betweenQuotes)
                    betweenQuotes = false; 
                else
                    betweenQuotes = true; 
            }
            
            if(betweenQuotes)
                buff[i] = ' ';
            else
                if((int)_str[i] > 96
                && (int)_str[i] < 123)
                {
                    buff[i] = (char)toupper(_str[i]);
                }
                else{
                    buff[i] = _str[i];
                }
                
        }
        buff[_str.length()] = '\0';
    // if(debug)
    //    fprintf(traceFile,"\nFind delimiter buffer:%s delimiter=%s",buff,_delimiter);

        char *s;
        s = strstr(buff, _delimiter.c_str());      // search for string "hassasin" in buff

        if (s != NULL)                     // if successful then s now points at "hassasin"
        {
            size_t result = s - buff;

            //Need to make sure this is an actual delimiter rather than a keyword embedded in another word.  example "DELETE" found in DELETED
            // in short, a space must follow a keyword

            size_t expectingSpace = _delimiter.length() + result;
            if(expectingSpace < _str.length())
            {
                if(_str[expectingSpace] != SPACE)
                    return NEGATIVE;
            }

            return result;
        }
        return std::string::npos;
    }                                  
    catch_and_throw_to_caller
    return std::string::npos;
}
/******************************************************
 * Find keyword from list
 ******************************************************/
//Returns the first delimiter to appear in the string
size_t findKeywordFromList(string _string, list<string> _list)
{
    try
    {  
        size_t len = _string.length();
        size_t found = len;
        size_t result;
        for(string delimiter : _list)
        {
            result = findKeyword(_string,delimiter);

            if(result != std::string::npos
            && result < found)
                found = result;
        }
        if(found < len)
            return found;
        return std::string::npos;
    }
    catch_and_throw_to_caller
}
/******************************************************
 * Get Function Type (convert string value in t_function value)
 ******************************************************/
t_function getFunctionType(string _type)
{
    try
    {
        if(findKeyword(_type,(char*)sqlTokenCount) != std::string::npos)
            return t_function::COUNT;
        else
            if(findKeyword(_type,(char*)sqlTokenSum) != std::string::npos)
                return t_function::SUM;
            else
                if(findKeyword(_type,(char*)sqlTokenMax) != std::string::npos)
                    return t_function::MAX;
                else
                    if(findKeyword(_type,(char*)sqlTokenMin) != std::string::npos)
                        return t_function::MIN;
                    else
                        if(findKeyword(_type,(char*)sqlTokenAvg) != std::string::npos)
                            return t_function::AVG;
        return t_function::NONE;
    }
    catch_and_throw_to_caller
}
/******************************************************
 * Snip String (string.substr wrapper)
 ******************************************************/
string snipString(string _string, size_t _position)
{
    //returns the end of a string
    return _string.substr(_position,_string.length() - _position);
}
/******************************************************
 * Snip String (string.substr wrapper, with removal of token)
 ******************************************************/
string snipString(string _string,string _token,size_t _position)
{
    //returns the end of a string after a token (pluse one space)
    _position = _position + _token.length() + 1;
    return _string.substr(_position,_string.length() - _position);
}
/******************************************************
 * Clip String (Erases N characters from end)
 ******************************************************/
string clipString(string _string,size_t _position)
{
    return _string.erase(_position, _string.length() - _position);
}
/******************************************************
 * Is Query Well Formed
 ******************************************************/
ParseResult isQueryWellFormed(char* _queryString)
{
    /*
      Is queryString well formed?

      ensures that parthesis and quotes are balanced
    */
   try
   {
   
        string sql;
        sql.append(_queryString);

        // Do open and close parenthesis match?
        if(std::count(sql.begin(), sql.end(), '(')
        != std::count(sql.begin(), sql.end(), ')'))
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Syntax error: mismatch of parenthesis");
            return ParseResult::FAILURE;
        }

        //Do quotes match?
        bool even = std::count(sql.begin(), sql.end(), '"') % 2 == 0;
        if(!even)
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Syntax error: too many or missing quotes.");
            return ParseResult::FAILURE;
        }

        return ParseResult::SUCCESS;
   }
   catch_and_throw_to_caller
}
/*******************************************************
   Normalize Query
*******************************************************/
string normalizeQuery(string _target, size_t _maxSize)
{
    /*
        1) Remove white noise: tabs, newline, carriage returns and formfeed
        2) Reduce multiple spaces to one
        3) Ensure there is a space before and after parenthesis
        4) Control spaces around and near equal sign.  somedata=somedata becomes somedata = somedata, same with somedata =< somedata
        5) Do not normalize text between quotes
    */
   try
   {

        char c      = SPACE;
        char next   = SPACE;
        char prior  = SPACE;
        int quotes      = 0;
        int openParen   = 0;
        int closeParen  = 0;
        bool betweenQuotes = false;
        size_t len = _target.length();
        size_t eos = len -1;                    //hard end of string
        if(len > _maxSize)
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Query exceeds max query size of ",to_string(_maxSize));
            return nullptr;
        }

        char str[_maxSize];
        size_t itr = 0;
        for(size_t i = 0;i<len;i++)
        {		
            if(itr > _maxSize)
            {
                sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Well formed query size exceeds max query size of",to_string(_maxSize));
                return nullptr;
            }

            prior = c;
            c = _target[i];

            // 5)
            if(c == QUOTE)
            {
                quotes++;
                if(betweenQuotes)
                    betweenQuotes = false;
                else
                    betweenQuotes = true;
            }

            //keep white space between quotes
            if(betweenQuotes)
            {
                str[itr] = c;
                itr++;
                continue;
            }

            // 1) eliminate white noise
            //      by converting to spaces
            //      multiple spaces will be dealt with downstream
            if((int)c == 0	//null
            || c == TAB
            || c == NEWLINE
            || c == RETURN
            || c == FORMFEED
            || c == VTAB)
            {
                c = SPACE;
            }  

            //prepare inquiry into normalization 
            if( itr<len
            ||  i < eos)        
                next = _target[i+1];
            
            if(c == OPENPAREN)
                openParen++;

            if(c == CLOSEPAREN)
                closeParen++;

            // 2) eliminate multiple spaces
            if(c == SPACE
            && prior == SPACE)
                continue;

            if(c == OPENPAREN
            || c == CLOSEPAREN)
            {
                if(prior != SPACE)
                {
                    str[itr] = SPACE;
                    itr++;
                }
                str[itr] = c;
                itr++;
                if(next != SPACE)
                {
                    str[itr] = SPACE;
                    itr++;
                }
                continue;
            }

            // 3)
            if(c == GTR
            || c == LST)
            {
                if(prior != SPACE
                && prior != LST)
                {
                    str[itr] = SPACE;
                    itr++;
                }
                str[itr] = c;
                itr++;
                if(next != SPACE
                && next != EQUAL
                && next != GTR)
                {
                    str[itr] = SPACE;
                    itr++;
                }
                continue;
            }

            //x=x
            if(c == EQUAL)
            {
                if(prior != SPACE
                && prior != GTR
                && prior != LST)
                {
                    str[itr] = SPACE;
                    itr++;
                }
                str[itr] = c;
                itr++;
                if(next != SPACE)
                {          
                    str[itr] = SPACE;
                    itr++;
                }
                continue;
            }

            str[itr] = c;
            itr++;
        }

        //Terminat string
        str[itr] = '\0';

        if( quotes > 0)
        {
        if(!(quotes   % 2 == 0))
            {
                sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Syntax error: too many or missing quotes.");
                return nullptr;
            } 
        }

        if(openParen == 0
        && closeParen == 0)
            return string(str);


        if(openParen != closeParen)
        {
            sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Syntax error: mismatch between ( and ).");
            return string();
        }
    
        return string(str);
    }
    catch_and_throw_to_caller
}
/*******************************************************
   Prepare String Template
*******************************************************/
string prepareStringTemplate(string _string)
{
    /*
        Prepare template string for searching.
            The template is used to normalize searching for locations of strings without having to
                alter the actual string itself.
        
        1) Transform to uppercase to eliminate case conflicts
        2) Avoid confusing key strings with text inbtween quotes by reducing quoted text to spaces in the template 
    */

   // 1)
   try
   {
    transform(_string.begin(), _string.end(), _string.begin(), ::toupper);

        bool betweenQuotes = false;
    // 2)
    for(size_t i = 0;i<_string.length(); i++)
    {
            if(_string[i] == QUOTE)
            {
                if(betweenQuotes)
                    betweenQuotes = false;
                else
                    betweenQuotes = true;
                
                continue;
            }

            if(betweenQuotes)
                _string[i] = SPACE;
    }
    return _string;
   }
   catch_and_throw_to_caller
}

/*******************************************************
   Get Table By Name
*******************************************************/
shared_ptr<sTable> getTableByName(list<shared_ptr<sTable>> tables,string _name)
{
    try
    {
        for(shared_ptr<sTable> tbl : tables)
        {
            if(strcasecmp(tbl->name.c_str(),_name.c_str()) == 0)
                return tbl;
        }
        return nullptr;
    }
    catch_and_throw_to_caller
}

/******************************************************
 * Get Table By Alias (Name)
 ******************************************************/
shared_ptr<sTable> getTableByAlias(list<shared_ptr<sTable>> tables,string _alias)
{
    try
    {
        for(shared_ptr<sTable> tbl : tables)
        {
            if(strcasecmp(tbl->alias.c_str(),_alias.c_str()) == 0)
                return tbl;
        }
        return nullptr;
    }
    catch_and_throw_to_caller
}

/******************************************************
 * Get Column By Name
 ******************************************************/
shared_ptr<Column> getColumnByName(list<shared_ptr<Column>> _columns, string _name)
{
    try
    {
        if(_name.empty())
            return nullptr;

        for(shared_ptr<Column> col : _columns)
        {
            if(strcasecmp(col->name.c_str(),_name.c_str()) == 0)
                return col;
        }
        
        return nullptr;
    }
    catch_and_throw_to_caller
}
/******************************************************
 * Get Column By Name
 ******************************************************/
shared_ptr<Column> getColumnByAlias(list<shared_ptr<Column>> _columns, string _alias)
{
    try
    {
        if(_alias.empty())
            return nullptr;

        for(shared_ptr<Column> col : _columns)
        {
            if(strcasecmp(col->alias.c_str(),_alias.c_str()) == 0)
                return col;
        }
        
        return nullptr;
    }
    catch_and_throw_to_caller
}
/******************************************************
 * Is Join
 ******************************************************/
size_t isJoin(string _string)
{
    try
    {
        list<string> lstJoin;
        lstJoin.push_back(sqlTokenJoin);
        lstJoin.push_back(sqlTokenInnerJoin);
        lstJoin.push_back(sqlTokenOuterJoin);
        lstJoin.push_back(sqlTokenLeftJoin);
        lstJoin.push_back(sqlTokenRightJoin);
        return findKeywordFromList(_string,lstJoin);
    }
   catch_and_throw_to_caller
    return std::string::npos;
}
/******************************************************
 * Process Split
 ******************************************************/
list<string> split(string _queryString)
{
    list<string> queries;
    try
    {
        if(debug)
            fprintf(traceFile,"\n------------- split ----------------");
        
        string queryString = _queryString;

        size_t joinFound = isJoin(queryString);

        if(joinFound == std::string::npos)
        {
            queries.push_back(queryString);
            return queries;
        }
        else
        {
            queries.push_back(queryString.substr(0,joinFound));
            //return portion of string AFTER join
            queryString = snipString(queryString,joinFound);
        }
        while(true)
        {
            /*
                The pupose of this routine is to walk backward through the string, clipping off each join
            */

            //A join statement will the first keyword in the string - offset that by 10 (well clear of a join keyword) and test
            string testStr = queryString.substr(10,queryString.length() -1);

            joinFound = isJoin(testStr);
            //if no join found at this point then this is the last segment
            if(joinFound == std::string::npos)
            {
                queries.push_back(queryString);
                break;
            }
                

            size_t posJoin = findKeyword(testStr,sqlTokenJoin);
            size_t posInnerJoin = findKeyword(testStr,sqlTokenInnerJoin);
            size_t posOuterJoin = findKeyword(testStr,sqlTokenOuterJoin);
            size_t posLeftJoin = findKeyword(testStr,sqlTokenLeftJoin);
            size_t posRightJoin = findKeyword(testStr,sqlTokenRightJoin);

            list<size_t> stack;
            if(posJoin != std::string::npos)
                stack.push_back(posJoin+10);
            if(posInnerJoin != std::string::npos)
                stack.push_back(posInnerJoin+10);
            if(posOuterJoin != std::string::npos)
                stack.push_back(posOuterJoin+10);
            if(posLeftJoin != std::string::npos)
                stack.push_back(posLeftJoin+10);
            if(posRightJoin != std::string::npos)
                stack.push_back(posRightJoin+10);

            stack.sort();
            stack.reverse();

            for(size_t join : stack)
            {
                //return portion of string AFTER join
                string query = snipString(queryString,join);
                queries.push_back(query);
                //erase portion of string AFTER join
                queryString = trim(clipString(queryString,join));
            }
        }

        return queries;
    }
    catch_and_throw_to_caller
    return queries;
}


