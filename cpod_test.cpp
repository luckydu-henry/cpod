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
    std::cout << ti.content() << std::endl;

    std::array<short, 65536> buf;
    std::array<bool, 10> bbuf;
    char myage = 0;
    bool AmIaBoy = false;
    auto rng = cpod::make_span(buf);
    auto brg = cpod::make_span(bbuf);
    
    ti
    .get("TArray", rng)
    .get("MyAge", myage)
    .get("AmIaBoy", AmIaBoy)
    .get("LightStates", brg)
    ;
    
    std::ranges::copy(rng, std::ostream_iterator<short>(std::cout, ", "));
    std::cout << std::endl;
    std::cout << std::format("MyAge is {:d}", myage) << std::endl;
    std::cout << std::format("Am I a boy ? : {}", AmIaBoy) << std::endl << std::boolalpha;
    std::ranges::copy(brg, std::ostream_iterator<bool>(std::cout, ", "));
}
