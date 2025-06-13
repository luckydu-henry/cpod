#include <cstdint> // For (u)intX_t
#include <string>  // For string.
#include <utility>
using namespace std;
/* ***********************************************************************
* Test cpod file
* This file shows how you can use cpod to (de)serialize basic types and
* structures.
* Comment handle and string handle are also capable, anyway see for yourself
* ***********************************************************************/

string myName          = "Henry Du";
int    myAge           = 17;
bool   amIaBoy         = true;
float  myHeight        = 169.9F;
float  myWidth         = 70.456F; // in kg.
string myEmails[4]     = { 
    "wotsukoroga94@gmail.com", 
    "dududu_721@qq.com", 
    "13552325266@163.com",
    "18516915799@126.com"
};