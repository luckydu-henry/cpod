#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include "cpod.hpp"




int main() {
    using namespace std::string_view_literals;
    using namespace std::string_literals;

    int         A = 0;
    float       B = 0.F;
    std::string C;
    
    cpod::archive arch;
    
    arch << R"(
std::string C = "Hello World, I'm \"Henry Du!\" ";
float B = 20.35F;
int A = 100;

)";
    auto msg = arch.compile_content_default();

    // Empty means no error message.
    if (msg.empty()) {

        arch >> cpod::var("A", A)
             >> cpod::var("B", B)
             >> cpod::var("C", C);
        
        std::cout << A << '\n';
        std::cout << B << '\n';
        std::cout << C << '\n';
        
    } else {
        std::cout << msg << '\n';
    }
}