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
//              A quick demo to show how to use archive writer.
////////////////////////////////////////////////////////////////////////////////////

#include <iostream>
#include <string>
#include <fstream>
#include <string_view>
#include <format>

#include "cpod.hpp"

//////////////////////////////////////////////////////////////////////
///           Below showed how to use custom structure             ///
//////////////////////////////////////////////////////////////////////

struct personal_info {
    std::string           name;
    std::string           gender;
    uint8_t               age;
    std::set<std::string> emails;
};

template <>
struct cpod::serializer<personal_info> {
    // Required static variable.
    static constexpr std::string_view type_name = "personal_info";
    constexpr void operator()(archive& arch, std::string_view name, const personal_info& v, flag_t flag) const {
        auto_structure_description_writer<personal_info> sw(arch, name);
        serializer<std::string>          {}(arch, "name", v.name, flag);     arch << '\n';   
        serializer<std::string>          {}(arch, "gender", v.gender, flag); arch << '\n';
        serializer<std::uint8_t>         {}(arch, "age", v.age, flag);       arch << '\n';
        serializer<std::set<std::string>>{}(arch, "emails", v.emails, flag); arch << '\n';
    }
    constexpr void operator()(std::string::const_iterator& mem_begin, personal_info& v, flag_t flag) {
        // Reader is much shorter and thus faster.
        serializer<std::string>           {}(mem_begin, v.name, flag);
        serializer<std::string>           {}(mem_begin, v.gender, flag);
        serializer<std::uint8_t>          {}(mem_begin, v.age, flag);
        serializer<std::set<std::string>> {}(mem_begin, v.emails, flag);
    }
};


int main(int argc, char** argv) {
    cpod::archive arch;

    personal_info myself = {
        "Henry Du",
        "Male",
        17,
        {"wotsukoroga94@gmail.com", "xidhyu@outlook.com"}
    };

    personal_info myself_cache;
    
    arch << cpod::var("personal_info_0", myself);
    
    std::ofstream out_text("personal_info.cpod.hpp");
    out_text << "#include <string>\n#include <set>\n";
    out_text << arch.content();
    out_text.close();

    arch.compile_content_default();
    arch >> cpod::var("personal_info_0", myself_cache);

    // wotsukoroga94@gmail.com
    std::cout << *myself_cache.emails.begin() << '\n';
}