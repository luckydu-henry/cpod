#include "cpod.hpp"
#include <iostream>
#include <array>
#include <fstream>
#include <sstream>
#include <format>

int main(int argc, char* argv[]) {

    std::ifstream ifs("TestFile.cpod.cpp");
    std::stringstream iss;
    iss << ifs.rdbuf();

    cpod::text_archive ti(iss.str());
    // Remove comments, useless spaces and other unrequired characters.
    // This step is necessary if you want to read data correctly.
    ti.normalize_content();

    std::array<short, 65536> buf;
    char myage = 0;
    auto rng = cpod::make_span(buf);
    
    ti
    .get("TArray", rng)
    .get("MyAge", myage)
    ;
    
    std::ranges::copy(rng, std::ostream_iterator<short>(std::cout, ", "));
    std::cout << std::endl;
    std::cout << std::format("MyAge is {:d}", myage);
}
