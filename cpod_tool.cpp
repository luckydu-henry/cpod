#include "cpod.hpp"
#include <iostream>
#include <array>
#include <fstream>
#include <sstream>

int main(int argc, char* argv[]) {

    std::ifstream ifs("TestFile.cpod.cpp");
    std::stringstream iss;
    iss << ifs.rdbuf();

    cpod::text_archive ti(iss.str());
    // Remove comments, useless spaces and other unrequired characters.
    // This step is necessary if you want to read data correctly.
    ti.normalize_content();

    char g = 0;
    ti.get("MyAge", g);
    std::cout << "g = " << static_cast<int>(g) << std::endl;
}
