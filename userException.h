#pragma once
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include "message.h"

class user_exception : public std::runtime_error {
    std::string msg;
public:
    user_exception(const std::string &arg, const char *file, const char *function, int line) :
    std::runtime_error(arg) {

        string errorText = "\n\tFatal error at File: ";
	    errorText.append(file);
        errorText.append("\n\tFunction: ");
        errorText.append(function);
        errorText.append("\n\tLine: ");
        errorText.append(std::to_string(line)); 
        errorText.append("\n\tError: ");
        errorText.append(arg.c_str());
        Message::message(DISPLAY::ERROR,errorText.c_str(),"");
    }
};

class exception_trace
{
    public:
    exception_trace(const std::string &arg, const char *file, const char *function, int line)
     {
        string errorText = "\n\tFatal error at File";
        errorText.append(file);
        errorText.append("\n\tFunction:");
        errorText.append(function);
        errorText.append("\n\tLine:");
        errorText.append(std::to_string(line));
        errorText.append("\n\tError:");
        errorText.append(arg.c_str());
        Message::message(DISPLAY::ERROR,errorText.c_str(),"");
    }
};

//Throws user_execption - logging file, function, line and arguement
#define throw_user_exception(arg)   throw user_exception(arg, __FILE__, __FUNCTION__, __LINE__);

//Same as above with a more descriptive title
#define throw_back_to_caller(arg)   throw user_exception(arg, __FILE__, __FUNCTION__, __LINE__);

//Logs caught exception
#define trace_exception(arg)        exception_trace(arg, __FILE__, __FUNCTION__, __LINE__);

//Catches std::exception and everything else (...), logs the information above and rethrows the exception 
#define catch_and_throw_to_caller   catch(const std::exception& e) { throw_back_to_caller(e.what()); } catch(...) { throw_back_to_caller("Undefined exception"); }

//Catches std::exception and everything else (...) and logs the information above, no rethrowing
#define catch_and_trace 	        catch(const std::exception& e) { trace_exception(e.what()); } catch(...)	{ trace_exception("Undefined exception"); }

//Display exception error
#define display_exception           render->message(DISPLAY::ERROR,"Exception at ",__FUNCTION__);

//Catch exeption, trace error, display message

#define catch_trace_display	        catch(const std::exception& e) { trace_exception(e.what()); display_exception } catch(...)	{ trace_exception("Undefined exception"); display_exception }
// test string to raise errors
//throw_user_exception("Test");  //DEBUG

// testing statement
// throw exception(); //DEBUG