#include <iostream>
#include "cpod.hpp"

int main() {
    using namespace std::string_view_literals;
    using namespace std::string_literals;

    std::unordered_map<int, std::tuple<float, float>> map = { {1, std::make_tuple(1.F, 2.F)}, {3, std::make_tuple(1.F, 2.F)} };
    
    cpod::archive arch;
    arch << cpod::var("AMap", map);

    std::cout << arch.content() << std::endl;
}