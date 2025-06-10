#include <cstdint> // For (u)intX_t
#include <string>  // For string.
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
string myEmails[2]     = { "wotsukoroga94@gmail.com", "dududu_721@qq.com" };
float  myHeight        = 169.9F;
float  myWidth         = 70.45F; // in kg.