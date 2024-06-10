#include "sqlCommon.h"
/******************************************************
 * Is Query Well Formed
 ******************************************************/
ParseResult isQueryWellFormed(char* _queryString)
{
    //---------------------------------
    //  Is queryString well formed?
    //---------------------------------

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
/*******************************************************
   Ensure well formed query
*******************************************************/
shared_ptr<char[]> sanitizeQuery(char _target[], size_t _maxSize)
{
	char c      = SPACE;
    char next   = SPACE;
    char prior  = SPACE;
    int quotes      = 0;
    int openParen   = 0;
    int closeParen  = 0;
    size_t eol         = _maxSize-1;  //hard end of line
	size_t len = strlen(_target);
    size_t eos = len -1;                    //hard end of string
    if(len > _maxSize)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Query exceeds max query size of ",to_string(_maxSize));
        return nullptr;
    }

    std::shared_ptr<char[]> str(new char[_maxSize]);
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

        if( itr<eol
        ||  i < eos)        
            next = _target[i+1];

        if(c == QUOTE)
            quotes++;
        
        if(c == OPENPAREN)
            openParen++;

        if(c == CLOSEPAREN)
            closeParen++;

		if((int)c == 0	//null
		|| c == NEWLINE
        || c == RETURN
        || c == FORMFEED
        || c == TAB
        || c == VTAB)
        {
            continue;
        }

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
        return str;

    if(openParen != closeParen)
    {
        sendMessage(MESSAGETYPE::ERROR,presentationType,true,"Syntax error: mismatch between ( and ).");
        return nullptr;
    }
 
    return str;
}