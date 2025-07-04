//
// MIT License
//
// Copyright (c) 2025 Henry Du
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
////////////////////////////////////////////////////////////////////////////////////
//                 A quick demo to show how to use dynamic IO
////////////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <string>
#include <fstream>
#include <string_view>
#include <format>
#include "cpod.hpp"

int main() {
    using namespace std::string_view_literals;
    using namespace std::string_literals;

    std::ifstream source_file("test_v1.cpod.hpp");
    // Ignore macros and includes inside source file.
    std::string   cleaner_source;
    std::string   source_line;
    while (std::getline(source_file, source_line)) {
        if (!(source_line[0] == '#' && source_line[1] != 'd')) {
            source_line.push_back('\n');
            cleaner_source += source_line;
        }
    }

    cpod::archive arch(cleaner_source);
    
    // Compiler will turn text to binary so that we can read data much faster.
    // This step is required because reader can not read text data.
    auto msg = arch.compile_content_default();

    // Default binary archive has no any metadata (header/author...)
    // Add metadata as your will.
    // There will have many zeros inside binary archive, it's also good to combine this with 'zip' or something
    // if you need smaller file or faster data streaming.
    std::ofstream out_binary("binary_vertices.cpod.bin", std::ios::binary);
    out_binary.write(arch.content().data(), arch.content().size());

    std::vector<
    std::tuple<
        std::array<float, 3>, 
        std::array<float, 3>,
        std::array<float, 2>
        >
    > position_color_uv_vertices;

    // You can use custom string allocator.
    // Also, available to other container types.
    std::pmr::string mesh_name;

    int enum_number = 0;
    
    // Empty means no error message.
    if (msg.empty()) {
        try {
            arch
            >> cpod::var("position_color_uv_vertices", position_color_uv_vertices)
            >> cpod::var("mesh_name", mesh_name)
            >> cpod::var("enum_number",  enum_number);
            
            std::cout << "Read mesh : " << mesh_name << '\n';
            std::cout << "---------------------------------------------\n";

            std::cout << "Enum number : " << enum_number << '\n';
            std::cout << "---------------------------------------------\n";
            // Same to inline version but read dynamically.
            for (std::size_t i = 0; i != position_color_uv_vertices.size(); ++i) {
            
                auto& pos = std::get<0>(position_color_uv_vertices[i]);
                auto& col = std::get<1>(position_color_uv_vertices[i]);
                auto& uv  = std::get<2>(position_color_uv_vertices[i]);
            
                std::cout << std::format("Vertex: {}\n", i);
                std::cout << std::format("    vec3 position({}, {}, {})\n", pos[0], pos[1], pos[2]);
                std::cout << std::format("    vec3 color({}, {}, {})\n", col[0], col[1], col[2]);
                std::cout << std::format("    vec2 uv({}, {})\n", uv[0], uv[1]);
            }
            
        } catch (std::exception& e) {
            std::cout << e.what() << std::endl;
        }
    } else {
        std::cout << msg << '\n';
    }
    
}