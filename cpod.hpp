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
#pragma once

// Core headers
#include <string>
#include <ranges>
#include <utility>
#include <algorithm>
#include <stdexcept>
#include <utility>
#include <tuple>
#include <type_traits>
#include <charconv>  // from_chars and to_chars

// Container support headers.
#include <array>
#include <vector>
#include <span>
#include <deque>
#include <list>
#include <forward_list>
#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>

namespace cpod {

    using  flag_t = std::uint32_t;
    class  archive;

    template <class Ty>
    struct variable_view {
        std::string_view             name;
        Ty*                          value;
        flag_t                       flag; // For get this is useless.

        variable_view(std::string_view n, const Ty& v, flag_t f = {}) :
        name(n), value(const_cast<Ty*>(&v)), flag(f) {}

        variable_view(std::string_view n, Ty& v, flag_t f = {}) :
        name(n), value(&v), flag(f) {}
    };

    // For convenient construction.
    template <class Ty>
    using var = variable_view<Ty>;

    // This demonstrates what a basic serializer should contain.
    template <class Ty>
    struct serializer {};
    
    typedef struct {
        std::string            code;
        std::string            errmsg;
    } compile_result;

    

    // Compiler declaration namespace.
    namespace detail {

        // Remove comments and useless spaces and
        std::string    remove_useless_data(std::string_view src);
        // Replace '\n' '\t' to there actual memory form and convert all string to 'R' string.
        std::string    normalize_string(std::string_view src);
        // Lexer that split tokens apart.
        std::string    tokenizer_split_source(std::string_view src);
        // Generate final result.
        std::string    generate_byte_stream  (std::string_view src);

        // Generate binary code that can be loaded by the program.
        compile_result generate_binary_code(std::string_view src);
    }

    class archive {
        std::string content_;
        std::string::const_iterator find_variable_begin(std::string_view var_name);
    public:
        archive() = default;
        archive(std::string_view c) : content_(c) {}
        
        std::string&        content()       { return content_; }
        std::string_view    content() const { return content_; }

        // Compile writes compiled code stream to content_.
        compile_result      compile();

        template <class Ty>
        constexpr archive& operator<<(const variable_view<Ty> v) {
            serializer<Ty>{}(*this, v.name, *v.value, v.flag);
            return *this;
        }

        constexpr archive& operator<<(std::string_view str) {
            content_.append(str);
            return *this;
        }

        constexpr archive& operator<<(char c) {
            content_.push_back(c);
            return *this;
        }

        template <class Ty>
        constexpr archive& operator>>(variable_view<Ty> v) {
            auto it = find_variable_begin(v.name);
            serializer<Ty>{}(*this, it, *v.value);
            return *this;
        }
    };

    namespace detail {

        template <class Ty>
        struct std_basic_type_traits : std::false_type {};

#define DEFINE_STD_BASIC_TYPE_STRING(type, id)                                 \
template <>                                                                    \
struct std_basic_type_traits<std::remove_cvref_t<##type##>> : std::true_type { \
    static constexpr std::string_view name = #type ;                           \
    static constexpr std::uint8_t     identifier = id;                         \
}
    
        DEFINE_STD_BASIC_TYPE_STRING(int8_t,        1);
        DEFINE_STD_BASIC_TYPE_STRING(uint8_t,       2);
        DEFINE_STD_BASIC_TYPE_STRING(int16_t,       3);
        DEFINE_STD_BASIC_TYPE_STRING(uint16_t,      4);
        DEFINE_STD_BASIC_TYPE_STRING(int,           5);
        DEFINE_STD_BASIC_TYPE_STRING(uint32_t,      6);
        DEFINE_STD_BASIC_TYPE_STRING(int64_t,       7);
        DEFINE_STD_BASIC_TYPE_STRING(uint64_t,      8);
        DEFINE_STD_BASIC_TYPE_STRING(float,         9);
        DEFINE_STD_BASIC_TYPE_STRING(double,        10);
        DEFINE_STD_BASIC_TYPE_STRING(bool,          11);
        DEFINE_STD_BASIC_TYPE_STRING(std::string,   12);

        // String and string_view uses same type name.
        template <>
        struct std_basic_type_traits<std::basic_string_view<char>> : std::true_type {
            static constexpr std::string_view name        = "std::string";
            static constexpr std::uint8_t     identifier  = 12;
        };
#undef  DEFINE_STD_BASIC_TYPE_STRING

        // Concepts and constraints.
        template <class Ty>
        struct std_string_type_traits : std::false_type {};

        template <typename CharT, template <class> class Allocator>
        struct std_string_type_traits<std::basic_string<CharT, std::char_traits<CharT>, Allocator<CharT>>> : std::true_type {
            static constexpr bool is_view = false;
        };

        template <typename CharT>
        struct std_string_type_traits<std::basic_string_view<CharT, std::char_traits<CharT>>> : std::true_type {
            static constexpr bool is_view = true;
        };

        // Container type traits.
        template <class Ty>
        struct std_template_library_type_traits : std::false_type {};
        
#define DEFINE_MONO_STL_TRAITS(type, id, resizable) \
    template <typename Ty, typename ... OtherStuff> \
    struct std_template_library_type_traits<type##<Ty, OtherStuff...>> : std::true_type { \
        static constexpr bool             is_resizeable      = resizable;\
        static constexpr bool             is_mono            = true;     \
        static constexpr bool             is_double          = false;    \
        static constexpr std::string_view name               = #type;    \
        static constexpr std::uint8_t     identifier         = id;       \
    }
#define DEFINE_DOUBLE_STL_TRAITS(type, id) \
template <typename K, typename V, typename ... OtherStuff> \
    struct std_template_library_type_traits<type##<K, V, OtherStuff...>> : std::true_type { \
        static constexpr bool             is_resizeable      = false;    \
        static constexpr bool             is_mono            = false;    \
        static constexpr bool             is_double          = true;     \
        static constexpr std::string_view name               = #type;    \
        static constexpr std::uint8_t     identifier         = id;       \
    }
        
        DEFINE_MONO_STL_TRAITS(std::vector,               13, true);
        DEFINE_MONO_STL_TRAITS(std::deque,                14, true);
        DEFINE_MONO_STL_TRAITS(std::list,                 15, true);
        DEFINE_MONO_STL_TRAITS(std::forward_list,         16, true);
        DEFINE_MONO_STL_TRAITS(std::set,                  17, false);
        DEFINE_MONO_STL_TRAITS(std::multiset,             18, false);
        DEFINE_MONO_STL_TRAITS(std::unordered_set,        19, false);
        DEFINE_MONO_STL_TRAITS(std::unordered_multiset,   20, false);

        DEFINE_DOUBLE_STL_TRAITS(std::map,                21);
        DEFINE_DOUBLE_STL_TRAITS(std::multimap,           22);
        DEFINE_DOUBLE_STL_TRAITS(std::unordered_map,      23);
        DEFINE_DOUBLE_STL_TRAITS(std::unordered_multimap, 24);

        template <typename K, typename V>
        struct std_template_library_type_traits<std::pair<K, V>> : std::true_type {
            static constexpr bool             is_resizeable      = false;
            static constexpr bool             is_mono            = false;
            static constexpr bool             is_double          = false; 
            static constexpr std::string_view name               = "std::pair";
            static constexpr std::uint8_t     identifier         = 25;
        };

        template <typename Ty, std::size_t N>
        struct std_template_library_type_traits<std::array<Ty, N>> : std::true_type {
            static constexpr bool             is_resizeable      = false;
            static constexpr bool             is_mono            = false;
            static constexpr bool             is_double          = false; 
            static constexpr std::string_view name               = "std::array";
            static constexpr std::uint8_t     identifier         = 26;
            
        };

        template <typename ... Args>
        struct std_template_library_type_traits<std::tuple<Args...>> : std::true_type {
            static constexpr bool             is_resizeable      = false;
            static constexpr bool             is_mono            = false;
            static constexpr bool             is_double          = false; 
            static constexpr std::string_view name               = "std::tuple";
            static constexpr std::uint8_t     identifier         = 27;
        };
#undef DEFINE_DOUBLE_STL_TRAITS
#undef DEFINE_MONO_STL_TRAITS

        template <class Ty>
        concept std_basic_type             = std_basic_type_traits<Ty>::value;

        template <class Ty>
        concept std_template_library_range = std_template_library_type_traits<Ty>::is_mono || std_template_library_type_traits<Ty>::is_double;

        template <class Ty>
        struct iterate_std_template_stuff_impl {};

        template <class ... Args>
        struct iterate_std_template_recursive_helper {};

        template <class Last>
        struct iterate_std_template_recursive_helper<Last> {
            constexpr auto operator()(std::string& buf, bool bin) const {
                iterate_std_template_stuff_impl<Last>{}(buf, bin);
            }
        };
        
        template <class First, typename ... Rest>
        struct iterate_std_template_recursive_helper<First, Rest...> {
            constexpr auto operator()(std::string& buf, bool bin) const {
                iterate_std_template_stuff_impl<First>{}(buf, bin);
                iterate_std_template_recursive_helper<Rest...>{}(buf, bin);
            }
        };
        
        template <std_basic_type Value>
        struct iterate_std_template_stuff_impl<Value> {
            constexpr auto operator()(std::string& buf, bool bin) const {
                if (!bin) {
                    buf.append(std_basic_type_traits<std::remove_const_t<Value>>::name);
                } else {
                    buf.push_back(std_basic_type_traits<std::remove_const_t<Value>>::identifier);
                }
                buf.push_back(',');
            }
            template <class Formatter>
            constexpr auto operator()(std::string& buf, Formatter formatter, const Value& value) {
                std::invoke(formatter, buf, value);
                buf.push_back(',');
            }
        };

        template <std_template_library_range STL>
        struct iterate_std_template_stuff_impl<STL> {
            constexpr auto operator()(std::string& buf, bool bin) const {
                if (!bin) {
                    buf.append(std_template_library_type_traits<STL>::name).push_back('<');   
                } else {
                    buf.push_back(std_template_library_type_traits<STL>::identifier);
                    buf.push_back('<');
                }
                if constexpr (std_template_library_type_traits<STL>::is_mono) {
                    iterate_std_template_recursive_helper<typename STL::value_type>{}(buf, bin);
                }
                else if constexpr (std_template_library_type_traits<STL>::is_double) {
                    iterate_std_template_recursive_helper<typename STL::key_type, typename STL::value_type::second_type>{}(buf, bin);
                }
                buf.back() = '>';
                buf.push_back(',');
            }
            template <class Formatter>
            constexpr auto operator()(std::string& buf, Formatter formatter, const STL& value) {
                buf.push_back('{');
                for (auto i = value.cbegin(); i != value.cend(); ++i) {
                    iterate_std_template_stuff_impl<typename STL::value_type>{}(buf, formatter, *i);
                }
                buf.back() = '}';
                buf.push_back(',');
            }
        };

        template <typename F, typename S>
        struct iterate_std_template_stuff_impl<std::pair<F, S>> {
            constexpr auto operator()(std::string& buf, bool bin) const {
                if (!bin) {
                    buf.append("std::pair<");
                } else {
                    buf.push_back('\x19');
                    buf.push_back('<');
                }
                iterate_std_template_recursive_helper<F, S>{}(buf, bin);
                buf.back() = '>';
                buf.push_back(',');
            }
            template <class Formatter>
            constexpr auto operator()(std::string& buf, Formatter formatter, const std::pair<F, S>& value) {
                buf.push_back('{');
                iterate_std_template_stuff_impl<std::remove_cvref_t<F>>{}(buf, formatter, value.first);
                iterate_std_template_stuff_impl<std::remove_cvref_t<S>>{}(buf, formatter, value.second);
                buf.back() = '}';
                buf.push_back(',');
            }
        };
        
        template <typename Ty, std::size_t N>
        struct iterate_std_template_stuff_impl<std::array<Ty, N>> {
            template <std::size_t Index = 0, class Formatter>
            constexpr void write_array(std::string& buf, Formatter formatter, const std::array<Ty, N>& value) {
                if constexpr (Index < N) {
                    iterate_std_template_stuff_impl<Ty>{}(buf, formatter, std::get<Index>(value));
                    write_array<Index + 1, Formatter>(buf, formatter, value);
                }
            } 
            constexpr auto operator()(std::string& buf, bool bin) const {
                if (!bin) {
                    buf.append("std::array<");
                } else {
                    buf.push_back('\x1a');
                    buf.push_back('<');
                }
                iterate_std_template_stuff_impl<Ty>{}(buf, bin);
                buf.append(std::to_string(N)).push_back('>');
                buf.push_back(',');
            }
            template <class Formatter>
            constexpr auto operator()(std::string& buf, Formatter formatter, const std::array<Ty, N>& value) {
                buf.push_back('{');
                write_array(buf, formatter, value);
                buf.back() = '}';
                buf.push_back(',');
            }
        };

        template <class ... Args>
        struct iterate_std_template_stuff_impl<std::tuple<Args...>> {
            template <std::size_t Index = 0, class Formatter>
            constexpr void write_tuple(std::string& buf, Formatter formatter, const std::tuple<Args...>& value) {
                if constexpr (Index < sizeof ... (Args)) {
                    iterate_std_template_stuff_impl<std::tuple_element_t<Index, std::tuple<Args...>>>{}(buf, formatter, std::get<Index>(value));
                    write_tuple<Index + 1, Formatter>(buf, formatter, value);
                }
            }
            constexpr auto operator()(std::string& buf, bool bin) const {
                if (!bin) {
                    buf.append("std::tuple<");
                } else {
                    buf.push_back('\x1b');
                }
                iterate_std_template_recursive_helper<Args...>{}(buf, bin);
                buf.back() = '>';
                buf.push_back(',');
            }
            template <class Formatter>
            constexpr auto operator()(std::string& buf, Formatter formatter, const std::tuple<Args...>& value) {
                buf.push_back('{');
                write_tuple(buf, formatter, value);
                buf.back() = '}';
                buf.push_back(',');
            }
        };
        
    }

    template <class Ty>
    concept std_type = detail::std_template_library_type_traits<Ty>::value || detail::std_basic_type<Ty>;
    
    // Further type string all use this.
    template <typename Ty>
    constexpr auto std_type_name_string(bool bin = false) noexcept {
        std::string buffer;
        detail::iterate_std_template_stuff_impl<Ty>{}(buffer, bin);
        if (!bin) {
            buffer.pop_back(); // Remove last ,
        }
        else {
            buffer.back() = '\0';
        }
        return buffer;
    }

    template <typename Ty, class Formatter>
    constexpr auto std_type_value_string(const Ty& value, Formatter formatter) {
        std::string buffer;
        detail::iterate_std_template_stuff_impl<Ty>{}(buffer, formatter, value);
        buffer.back() = ';';
        return buffer;
    }

    typedef enum std_basic_type_format_flag{
        integer_binary            = 1 << 1,
        integer_heximal           = 1 << 2,
        floating_point_fixed      = 1 << 3,
        floating_point_scientific = 1 << 4,
        string_use_raw            = 1 << 5,
    } std_basic_type_format_flag;

    struct std_basic_type_text_output_formatter {
        flag_t flag{};
        
        template <detail::std_basic_type Ty>
        constexpr void operator()(std::string& buf, const Ty& value) {
            if constexpr (detail::std_string_type_traits<Ty>::value) {
                if (flag & string_use_raw) {
                    buf.append("R\"(").append(value).append(")\"");
                }
                else {
                    std::string cache;
                    cache.reserve(value.size());
                    for (std::size_t i = 0; i != value.size(); ++i) {
                        switch (value[i]) {
                        default: cache.push_back(value[i]); break;
                        case '\n': cache.append("\\n");     break;
                        case '\t': cache.append("\\t");     break;
                        case '\r': cache.append("\\r");     break;
                        case '\b': cache.append("\\b");     break;
                        case '\v': cache.append("\\v");     break;
                        case '\f': cache.append("\\f");     break;
                        case '\a': cache.append("\\a");     break;
                        case '\"': cache.append("\\\"");    break;
                        case '\\': cache.append("\\");      break;
                        }
                    }
                    buf.push_back('\"');
                    buf.append(cache);
                    buf.push_back('\"');
                }
            }
            else if constexpr (std::floating_point<Ty>) {
                std::chars_format fmt = std::chars_format::general;
                if (flag & floating_point_fixed)      { fmt = std::chars_format::fixed; }
                if (flag & floating_point_scientific) { fmt = std::chars_format::scientific; }
                char buffer[32];
                auto end = std::to_chars(buffer, buffer + 32, value, fmt).ptr;
                buf.append(buffer, end - buffer);
            }
            else if constexpr (std::integral<Ty> && !std::is_same_v<Ty, bool>) {
                int base = 10;
                if constexpr (std::is_unsigned_v<Ty>) {
                    if (flag & integer_binary)  { base = 2;   buf.append("0b"); }
                    if (flag & integer_heximal) { base = 16;  buf.append("0x"); }
                }
                char buffer[32];
                auto end = std::to_chars(buffer, buffer + 32, value, base).ptr;
                buf.append(buffer, end - buffer);
            }
            else if constexpr (std::is_same_v<Ty, bool>) {
                std::string_view v = value ? "true" : "false";
                buf.append(v);
            }
        }
    };

    /////////////////////////////////////
    /// Basic serializer specialization
    /////////////////////////////////////
    template <std_type Ty>
    struct serializer<Ty> {
        constexpr void operator()(archive& arch, std::string_view name, const Ty& v, flag_t flag) {
            std_basic_type_text_output_formatter formatter{flag};
            arch.content().append(std_type_name_string<Ty>()).push_back(' ');
            arch.content().append(name).push_back('=');
            arch.content().append(std_type_value_string(v, formatter));
        }
        constexpr void operator()(archive& arch, std::string::const_iterator& mem_begin, Ty& v) {
            // TODO IMPLEMENT ME!
        }
    };
    
}