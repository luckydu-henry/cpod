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
// //              A quick demo to show how to use inline solution
// ////////////////////////////////////////////////////////////////////////////////////
//
// #include "test_v1.cpod.hpp" // Include this straightforward is the best performance usage
// #include <iostream>
// #include <format>
//
// int main(int argc, char ** argv) {
//     
//     std::cout << "Read mesh : " << mesh_name << '\n';
//     std::cout << "---------------------------------------------" << std::endl;
//
//     std::cout << "Enum number : " << enum_number << '\n';
//     std::cout << "---------------------------------------------\n";
//     for (std::size_t i = 0; i != position_color_uv_vertices.size(); ++i) {
//
//         auto& pos = std::get<0>(position_color_uv_vertices[i]);
//         auto& col = std::get<1>(position_color_uv_vertices[i]);
//         auto& uv  = std::get<2>(position_color_uv_vertices[i]);
//
//         std::cout << std::format("Vertex: {}\n", i);
//         std::cout << std::format("    vec3 position({}, {}, {})\n", pos[0], pos[1], pos[2]);
//         std::cout << std::format("    vec3 color({}, {}, {})\n", col[0], col[1], col[2]);
//         std::cout << std::format("    vec2 uv({}, {})\n", uv[0], uv[1]);
//     }
// }