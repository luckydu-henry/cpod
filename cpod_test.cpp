#include "cpod.hpp"
#include <iostream>
#include <array>
#include <fstream>
#include <sstream>
#include <format>
#include <memory_resource>

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

    std::cout << ti.content() << std::endl;
    
    // Next we will load data from archive to these variables.
    // Variable types must be same
    std::string                  myName;          
    int                          myAge;           
    bool                         amIaBoy;         
    float                        myHeight;        
    float                        myWidth;
    std::set<std::string>        myEmails;
    std::string                  shaderCode;
    
    ti
    .get("myName", myName)
    .get("myAge", myAge)
    .get("amIaBoy", amIaBoy)
    .get("myHeight", myHeight)
    .get("myWidth", myWidth)
    .get("myEmails", myEmails)
    .get("shaderCode", shaderCode);
    
    std::cout << "Personal info: \n" << std::boolalpha
              << "Name:        " << myName   << '\n'
              << "Age:         " << myAge    << '\n'
              << "Height:      " << myHeight << '\n'
              << "Width:       " << myWidth  << '\n'
              << "Am I a boy:  " << amIaBoy  << '\n'
              << "Emails: " << '\n';
    for (auto& i : myEmails) { std::cout << '\t' << i << '\n'; }

    std::cout << "----------This is a vertex shader-------------\n";
    std::cout << shaderCode << std::endl;

    // cpod::text_archive archive;
    // archive.put("HiThere", std::unordered_set<float>{1.15F, 2.26F});
    // std::cout << archive.content() << std::endl;
}
