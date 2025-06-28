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
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///
/// This hpp file shows how to use cpod to (de)serialize basic types.
/// NOTICE: below keywords are not supported by cpod yet.
///         char                           (Use int8_t instead).
///         unsigned char                  (Use uint8_t instead).
///         short                          (Use int16_t instead).
///         unsigned short                 (Use uint16_t instead).
///         int32_t                        (Use int instead).
///         unsigned int                   (Use uint32_t instead).
///         long long                      (Use int64_t instead).
///         unsigned long long             (Use uint64_t instead).
///         const char*                    (Use std::string instead).
/// And some C++ syntax are also not supported until now.
///         void* or any type*                         (Pointer syntax).
///         type name[len] array syntax                (Use std::vector for dynamic array and std::array for fixed size array).
///         struct/class                               (Will have future support).
///         macro e.g. # operator                      (Maybe will support this in the future).
///         namespace syntax                           (Definitely will have in the future).
///         separate string e.g. "A""B" == "AB"        (Not a very important feature, maybe will have but use R"()" string for now).
///         
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
///

#pragma once
#include <vector>
#include <map>
#include <string>
#include <array>
#include <unordered_map>
#include <unordered_set>
#include <tuple>
#include <utility>

static inline const
std::string mesh_name = "Mesh_Square";

static inline const 
std::vector<
    std::tuple<
        std::array<float, 3>,
        std::array<float, 3>, 
        std::array<float, 2>
    >
>
position_color_uv_vertices = {
    {{ 1.F, 1.F, 0.F}, {1.F,0.F,0.F}, {1.F, 0.F}},
    {{ 1.F,-1.F, 0.F}, {0.F,1.F,0.F}, {0.F, 1.F}},
    {{-1.F, 1.F, 0.F}, {0.F,0.F,1.F}, {1.F, 1.F}},
    {{-1.F,-1.F, 0.F}, {0.F,0.F,0.F}, {0.F, 0.F}}
    // More vertices as our will.
};