// //
// // MIT License
// //
// // Copyright (c) 2025 Henry Du
// //
// // Permission is hereby granted, free of charge, to any person obtaining a copy
// // of this software and associated documentation files (the "Software"), to deal
// // in the Software without restriction, including without limitation the rights
// // to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// // copies of the Software, and to permit persons to whom the Software is
// // furnished to do so, subject to the following conditions:
// //
// // The above copyright notice and this permission notice shall be included in all
// // copies or substantial portions of the Software.
// //
// // THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// // IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// // FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// // AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// // LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// // OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// // SOFTWARE.
// //
// ////////////////////////////////////////////////////////////////////////////////////
// //              A quick demo to show how to use archive writer.
// ////////////////////////////////////////////////////////////////////////////////////
//
// #include <iostream>
// #include <string>
// #include <fstream>
// #include <string_view>
// #include <format>
// #include "cpod.hpp"
//
// int main(int argc, char** argv) {
//     cpod::archive arch;
//     // std::string           myName        = "Henry Du";
//     // std::string           myGender      = "Male";
//     // uint8_t               myAge         = 17;
//     // std::set<std::string> myEmails = {
//     //     "wotsukoroga94@gmail.com",
//     //     "xidhyu@outlook.com"
//     // };
//     //
//     // arch << cpod::var("name", myName) << '\n'
//     //      << cpod::var("gender", myGender) << '\n'
//     //      << cpod::var("age", myAge) << '\n'
//     //      << cpod::var("emails", myEmails) << '\n';
//     //
//     // std::ofstream out_text("personal_info.cpod.hpp");
//     // out_text << "#include <string>\n#include <set>\n";
//     // out_text << arch.content();
//     // out_text.close();
// }