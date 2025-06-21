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

// This is a cpod file! 
pair<int, tuple<float,float>> AMap[2] = {
    {1, {1,2} },
    {3, {1,2} }
};