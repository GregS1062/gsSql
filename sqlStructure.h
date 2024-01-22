#include <iostream>
#include <map>
#include <string>
using namespace std;
enum t_edit{
    t_char,
    t_int,
    t_date
};
class databases
{
    public:
    string name;
    map<string,void*>tables;
};
class tables
{
    public:
    string name;
    map<string,void*>columns;
};
class column
{
    public:
    string name;
    t_edit edit;
    int16_t length;
};