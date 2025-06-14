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

pair<string, float> p = {"Hello world!", 1.234F};

string  myName          = "Henry Du";
int     myAge           = 17;
bool    amIaBoy         = true;
float   myHeight        = 169.9F;
float   myWidth         = 70.456F; // in kg.
string  myEmails[6]     = { 
    "wotsukoroga94@gmail.com", 
    "dududu_721@qq.com", 
    "13552325266@163.com",
    "18516915799@126.com",
    "xidhyu@outlook.com",
    "I do have plenty of emails!"
};

string shaderCode = 
"layout(location = 0) vec3 position;\n"
"void main() {\n"
"    gl_Position = vec4(position, 1.0);\n"
"}";