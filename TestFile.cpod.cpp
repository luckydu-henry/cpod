#include <vector>
#include <map>
#include <string>
#include <array>

/* ***********************************************************************
* Test cpod file
* This file shows how you can use cpod to (de)serialize basic types and
* structures.
* Comment handle and string handle are also capable, anyway see for yourself
* ***********************************************************************/
// This is a cpod file.
int IntValue=19;
std::map<std::string,float> Map={{"GoodJob!",5},{"Hello",1}};
std::vector<float>          DynamicVecF={1,2,3,4};
std::array<float,4>         StaticVecF={1,2,3,4};