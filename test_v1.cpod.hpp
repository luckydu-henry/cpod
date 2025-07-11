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
///         namespace syntax                           (Definitely will have in the future).
///
/// Macro now only supports ifdef endif and ifndef endif, elifdef/elifndef/else are not supported yet.
/// Don't have macro names inside a string e.g. if you have a macro 'A' then you can no longer have A inside a string but words
/// with A like "Apple" or "Actually" are still available you just can't have 'A' itself.
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

// This is the recommended usage of macro.
// Do not support macro functions since cpod itself doesn't have expression support either so it's unnecessary to have macro functions.
#define ENUM_NONE   0
#define ENUM_FIRST  1
#define ENUM_SECOND 2
#define ENUM_THIRD  3
#define ENUM_FOURTH 4

#define VECTOR3_TYPE std::array<float, 3>
#define VECTOR2_TYPE std::array<float, 2>

// Since there are no expressions inside cpod, using typedef and macro have no much difference, macros can even be a lot more flexible, so use this to do complex type aliasing.
#define position_color_uv_vertex_type std::tuple<\
    VECTOR3_TYPE, VECTOR3_TYPE, VECTOR2_TYPE>

#ifndef MESH_VERSION_TAG
static inline const
std::string mesh_name = "Mesh_Square";
#endif

#ifdef MESH_VERSION_TAG
static inline const
std::string mesh_name = "Mesh_Square_" MESH_VERSION_TAG ;
#endif

static inline const
int  enum_number = ENUM_FOURTH;

static inline const 
std::vector<position_color_uv_vertex_type>
position_color_uv_vertices = {
    {{ 1.F, 1.F, 0.F}, {1.F,0.F,0.F}, {1.F, 0.F}},
    {{ 1.F,-1.F, 0.F}, {0.F,1.F,0.F}, {0.F, 1.F}},
    {{-1.F, 1.F, 0.F}, {0.F,0.F,1.F}, {1.F, 1.F}},
    {{-1.F,-1.F, 0.F}, {0.F,0.F,0.F}, {0.F, 0.F}},
    
    {{ 1.F, 1.F, 0.F}, {1.F,0.F,0.F}, {1.F, 0.F}},
    {{ 1.F,-1.F, 0.F}, {0.F,1.F,0.F}, {0.F, 1.F}},
    {{-1.F, 1.F, 0.F}, {0.F,0.F,1.F}, {1.F, 1.F}},
    {{-1.F,-1.F, 0.F}, {0.F,0.F,0.F}, {0.F, 0.F}},

    {{ 1.F, 1.F, 0.F}, {1.F,0.F,0.F}, {1.F, 0.F}},
    {{ 1.F,-1.F, 0.F}, {0.F,1.F,0.F}, {0.F, 1.F}},
    {{-1.F, 1.F, 0.F}, {0.F,0.F,1.F}, {1.F, 1.F}},
    {{-1.F,-1.F, 0.F}, {0.F,0.F,0.F}, {0.F, 0.F}},
    
    {{ 1.F, 1.F, 0.F}, {1.F,0.F,0.F}, {1.F, 0.F}},
    {{ 1.F,-1.F, 0.F}, {0.F,1.F,0.F}, {0.F, 1.F}},
    {{-1.F, 1.F, 0.F}, {0.F,0.F,1.F}, {1.F, 1.F}},
    {{-1.F,-1.F, 0.F}, {0.F,0.F,0.F}, {0.F, 0.F}}
};

#undef ENUM_FIRST
#undef ENUM_SECOND
#undef ENUM_THIRD
#undef ENUM_FOURTH
#undef ENUM_NONE