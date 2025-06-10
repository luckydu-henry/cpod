#include "cpod.hpp"
#include <iostream>
#include <array>
#include <fstream>
#include <sstream>
#include <format>

int main(int argc, char* argv[]) {

    std::ifstream ifs("TestFile.cpod.cpp");
    std::stringstream iss;

    // Remove macro defines.
    for (std::string line; std::getline(ifs, line);) {
        if (line[0] != '#' && line != "using namespace std;") {
            iss  << line << "\n";
        } 
    }

    cpod::text_archive ti(iss.str());
    ti.normalize();

    // Next we will load data from archive to these variables.
    // Variable types must be same
    std::string                  myName;          
    int                          myAge;           
    bool                         amIaBoy;         
    std::array<std::string, 2>   myEmails;     
    float                        myHeight;        
    float                        myWidth;

    auto emailSpan = cpod::make_span(myEmails);
    
    ti
    .get("myName", myName)
    .get("myAge", myAge)
    .get("amIaBoy", amIaBoy)
    .get("myHeight", myHeight)
    .get("myWidth", myWidth)
    .get("myEmails", emailSpan);


    std::cout << "Personal info: \n" << std::boolalpha
              << "Name:        " << myName   << '\n'
              << "Age:         " << myAge    << '\n'
              << "Height:      " << myHeight << '\n'
              << "Width:       " << myWidth  << '\n'
              << "Am I a boy:  " << amIaBoy  << '\n'
              << "Emails: \n";
    for (auto& i : myEmails) { std::cout << '\t' << i << std::endl; }
    
}
