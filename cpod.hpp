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
        std::string::iterator  it;
        std::errc              ec;
    } compile_result;

    // Compiler declaration namespace.
    namespace detail {

        // Remove comments and useless spaces and
        std::string remove_useless_data(std::string_view src);
        // Replace '\n' '\t' to there actual memory form and convert all string to 'R' string.
        std::string normalize_string(std::string_view src);
        // Lexer that split tokens apart.
        std::string tokenizer_split_source(std::string_view src);
        // Generate final result.
        std::string generate_byte_stream  (std::string_view src);
        
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
struct std_basic_type_string {};

#define DEFINE_STD_TYPE_STRING_WITH_ALIASES(type, r, n)                     \
template <> struct std_basic_type_string<type> {                        \
static constexpr std::string_view raw                  = r;         \
static constexpr std::string_view neat                 = n;         \
static constexpr std::string_view aliases              = r##";"##n; \
static constexpr std::string_view default_name         = raw;       \
}
    
        DEFINE_STD_TYPE_STRING_WITH_ALIASES(int8_t  , "int8_t"  , "char");
        DEFINE_STD_TYPE_STRING_WITH_ALIASES(uint8_t , "uint8_t" , "unsigned char");
        DEFINE_STD_TYPE_STRING_WITH_ALIASES(int16_t , "int16_t" , "short");
        DEFINE_STD_TYPE_STRING_WITH_ALIASES(uint16_t, "uint16_t", "unsigned short");
        DEFINE_STD_TYPE_STRING_WITH_ALIASES(int     , "int"     , "int32_t");
        DEFINE_STD_TYPE_STRING_WITH_ALIASES(uint32_t, "uint32_t", "unsigned int");
        DEFINE_STD_TYPE_STRING_WITH_ALIASES(int64_t , "int64_t" , "long long");
        DEFINE_STD_TYPE_STRING_WITH_ALIASES(uint64_t, "uint64_t", "unsigned long long");
        // Future floating point may have these aliases but not now.
        DEFINE_STD_TYPE_STRING_WITH_ALIASES(float   , "float"   , "float32_t");
        DEFINE_STD_TYPE_STRING_WITH_ALIASES(double  , "double"  , "float64_t");

        template <>
        struct std_basic_type_string<bool> {
            static constexpr std::string_view default_name = "bool";
        };
        template <template <class> class Allocator>
        struct std_basic_type_string<std::basic_string<char, std::char_traits<char>, Allocator<char>>> {
            static constexpr std::string_view default_name = "string";
        };
        template <>
        struct std_basic_type_string<std::basic_string_view<char>> {
            static constexpr std::string_view default_name = "string";
        };
#undef  DEFINE_STD_TYPE_STRING_WITH_ALIASES

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

        template <typename T>
        struct std_pair_type_traits : std::false_type {};

        template <typename T1, typename T2>
        struct std_pair_type_traits<std::pair<T1, T2>> : std::true_type {};

        template <typename T>
        struct std_tuple_type_traits : std::false_type {};

        template <typename... Args>
        struct std_tuple_type_traits<std::tuple<Args...>> : std::true_type {};

        template <class Ty>
        struct std_structured_binding_type_traits : std::false_type {};

        template <typename K, typename V>
        struct std_structured_binding_type_traits<std::pair<K, V>> : std::true_type {
        };

        template <typename ... Types>
        struct std_structured_binding_type_traits<std::tuple<Types...>> : std::true_type {
        };

        template <class Ty>
        struct std_forward_container_type_traits : std::false_type {};

        // Two static container
        template <class Ty, std::size_t N>
        struct std_forward_container_type_traits<std::array<Ty, N>> : std::true_type {
            static constexpr bool is_map                = false;
            static constexpr bool is_set                = false;
            static constexpr bool is_static_sized       = true;
        };

        template <class Ty, std::size_t N>
        struct std_forward_container_type_traits<std::span<Ty, N>>  : std::true_type {
            static constexpr bool is_map                = false;
            static constexpr bool is_set                = false;
            static constexpr bool is_static_sized       = true;
        };

        // Contains to flat and get type string.
        template <typename Ty>
        struct to_flat_structure_stuff_impl {};

        template <typename Value> requires !std_structured_binding_type_traits<Value>::value
        struct to_flat_structure_stuff_impl<Value> {
            constexpr auto operator()(const Value& value) const {
                return std::tuple<Value>(value);
            }
            constexpr auto operator()(std::string& buf) const {
                buf.append(std_basic_type_string<std::remove_const_t<Value>>::default_name);
                buf.push_back(',');
            }
            template <class Fmt>
            constexpr auto operator()(const Value& value, std::string& buf, Fmt formatter) {
                std::invoke(formatter, value);
                buf.push_back(',');
            }
        };

        template <class ... Args>
        struct get_tree_structure_string_helper {};

        template <class Last>
        struct get_tree_structure_string_helper<Last> {
            constexpr auto operator()(std::string& buf) const {
                to_flat_structure_stuff_impl<Last>{}(buf);
            }
        };
        
        template <class First, typename ... Rest>
        struct get_tree_structure_string_helper<First, Rest...> {
            constexpr auto operator()(std::string& buf) const {
                to_flat_structure_stuff_impl<First>{}(buf);
                get_tree_structure_string_helper<Rest...>{}(buf);
            }
        };

        template <typename K, typename V>
        struct to_flat_structure_stuff_impl<std::pair<K, V>> {
            constexpr auto operator()(const std::pair<K, V>& p) const {
                return std::tuple_cat(to_flat_structure_stuff_impl<K>{}(p.first), to_flat_structure_stuff_impl<V>{}(p.second));
            }
            constexpr auto operator()(std::string& buf) const {
                buf.append("pair<");
                get_tree_structure_string_helper<K, V>{}(buf);
                buf.back() = '>';
                buf.push_back(',');
            }
            template <class Fmt>
            constexpr auto operator()(const std::pair<K, V>& p, std::string& buf, Fmt formatter) {
                buf.push_back('{');
                to_flat_structure_stuff_impl<K>{}(p.first, buf, formatter);
                to_flat_structure_stuff_impl<V>{}(p.second, buf, formatter);
                buf.back() = '}';
                buf.push_back(',');
            }
        };
        
        template <typename ... Args>
        struct to_flat_structure_stuff_impl<std::tuple<Args...>> {
            template <std::size_t Index = 0, class Fmt>
            constexpr auto write_tuple_impl(const std::tuple<Args...>& t, std::string& buf, Fmt formatter) {
                if constexpr (Index != std::tuple_size_v<std::tuple<Args...>>) {
                    to_flat_structure_stuff_impl<std::tuple_element_t<Index, std::tuple<Args...>>>{}(std::get<Index>(t), buf, formatter);
                    write_tuple_impl<Index + 1, Fmt>(t, buf, formatter);
                } else {
                    buf.back() = '}';
                    buf.push_back(',');
                }
            }
            constexpr auto operator()(const std::tuple<Args...>& t) const {
                return std::apply([]<typename... T0>(const T0&... args) {
                    return std::tuple_cat(to_flat_structure_stuff_impl<T0>{}(args)...);
                }, t);
            }
            constexpr auto operator()(std::string& buf) const {
                buf.append("tuple<");
                get_tree_structure_string_helper<Args...>{}(buf);
                buf.back() = '>';
                buf.push_back(',');
            }
            template <class Fmt>
            constexpr auto operator()(const std::tuple<Args...>& t, std::string& buf, Fmt formatter) {
                buf.push_back('{');
                write_tuple_impl(t, buf, formatter);
            }
        };
        
        template <typename Ty>
        auto to_flat_structure(const Ty& value) {
            return to_flat_structure_stuff_impl<Ty>{}(value);
        }

        template <typename Ty, class Fmt>
        auto get_tree_structure_value_string(const Ty& v, std::string& buf, Fmt f) {
            to_flat_structure_stuff_impl<Ty>{}(v, buf, f);
            buf.pop_back(); // remove last ,
        }

        template <typename Ty>
        auto get_tree_structure_type_string() {
            std::string buf;
            to_flat_structure_stuff_impl<Ty>{}(buf);
            buf.pop_back(); // remove last ,
            return buf;
        }

        template <std::size_t Index, typename Ty>
        struct to_tree_structure_impl {};

        template <std::size_t Index, typename Ty> requires !std_structured_binding_type_traits<Ty>::value
        struct to_tree_structure_impl<Index, Ty> {
            template <class Tuple>
            constexpr void operator()(Ty& v, const Tuple& tup) const {
                v = std::get<Index>(tup);
            }
            static constexpr std::size_t next_index = Index + 1;
        };

        template <std::size_t Index, typename K, typename V> 
        struct to_tree_structure_impl<Index, std::pair<K, V>> {
            using first_unflatten_t  = to_tree_structure_impl<Index, K>;
            using second_unflatten_t = to_tree_structure_impl<first_unflatten_t::next_index, V>;
            template <class Tuple>
            constexpr void operator()(std::pair<K, V>& p, const Tuple& tup) const {
                first_unflatten_t{}(p.first, tup);
                second_unflatten_t{}(p.second, tup);
            }
            static constexpr std::size_t next_index = second_unflatten_t::next_index;
        };

        // Helper template to calculate next index.
        template <std::size_t StartIndex, typename... Args>
        struct tuple_index_accumulator;

        // End when no more elements.
        template <std::size_t StartIndex>
        struct tuple_index_accumulator<StartIndex> {
            static constexpr std::size_t value = StartIndex;
        };

        // Handle each element.
        template <std::size_t StartIndex, typename First, typename... Rest>
        struct tuple_index_accumulator<StartIndex, First, Rest...> {
            static constexpr std::size_t after_first = 
                to_tree_structure_impl<StartIndex, First>::next_index;
            
            static constexpr std::size_t value = 
                tuple_index_accumulator<after_first, Rest...>::value;
        };

        template <std::size_t StartIndex, typename... Args>
        struct to_tree_structure_tuple_helper_impl;

        template <std::size_t StartIndex>
        struct to_tree_structure_tuple_helper_impl<StartIndex> {
            template <class Tuple, class FlatTuple>
            constexpr void operator()(Tuple&, const FlatTuple&) const {}
            static constexpr std::size_t next_index = StartIndex;
        };

        template <std::size_t StartIndex, typename First, typename... Rest>
        struct to_tree_structure_tuple_helper_impl<StartIndex, First, Rest...> {
            using first_unflatten_t = to_tree_structure_impl<StartIndex, First>;
            using rest_unflatten_t = to_tree_structure_tuple_helper_impl<first_unflatten_t::next_index, Rest...>;
            
            template <class Tuple, class FlatTuple>
            constexpr void operator()(Tuple& t, const FlatTuple& flat_tup) const {
                // Handle first element.
                first_unflatten_t{}(std::get<0>(t), flat_tup);
                // Handle the rest.
                if constexpr (sizeof...(Rest) > 0) {
                    std::apply([&](auto&&, auto&&... args) {
                        auto rest_tuple = std::tie(args...);
                        rest_unflatten_t{}(rest_tuple, flat_tup);
                    }, t);
                }
            }
            static constexpr std::size_t next_index = rest_unflatten_t::next_index;
        };

        template <std::size_t Index, typename... Args>
        struct to_tree_structure_impl<Index, std::tuple<Args...>> {
            using impl_t = to_tree_structure_tuple_helper_impl<Index, Args...>;
            
            template <class Tuple>
            constexpr void operator()(std::tuple<Args...>& t, const Tuple& tup) const {
                impl_t{}(t, tup);
            }
            
            // Calculate next index with helper template.
            static constexpr std::size_t next_index = 
                tuple_index_accumulator<Index, Args...>::value;
        };

        // The inverse of to_flat_structure
        template <std::size_t Index = 0, class Ty, class Tuple>
        void to_tree_structure(Ty& unflattend, const Tuple& flattend) {
            to_tree_structure_impl<Index, Ty>{}(unflattend, flattend);
        }
            
        // We don't have container adaptors supported i.e. stack and queue and future flat_map flat_set.
#define SPECIALIZE_DYNAMIC_STD_FORWARD_CONTAINER_TYPE_TRAITS(container, iss) \
template <class Ty, template <class> class Allocator> \
struct std_forward_container_type_traits<container##<Ty, Allocator<Ty>>> : std::true_type { \
static constexpr bool is_map                = false; \
static constexpr bool is_set                = iss;   \
static constexpr bool is_static_sized       = false; \
}
#define SPECIALIZE_DYNAMIC_STD_MAP_TYPE_TRAITS(container) \
template <class ... Args> \
struct std_forward_container_type_traits<container##<Args...>> : std::true_type { \
static constexpr bool is_map  = true;                   \
static constexpr bool is_set  = false;                  \
static constexpr bool is_static_sized       = false;    \
}

        SPECIALIZE_DYNAMIC_STD_FORWARD_CONTAINER_TYPE_TRAITS(std::vector, false);
        SPECIALIZE_DYNAMIC_STD_FORWARD_CONTAINER_TYPE_TRAITS(std::deque, false);
        SPECIALIZE_DYNAMIC_STD_FORWARD_CONTAINER_TYPE_TRAITS(std::list, false);
        SPECIALIZE_DYNAMIC_STD_FORWARD_CONTAINER_TYPE_TRAITS(std::forward_list, false);
        SPECIALIZE_DYNAMIC_STD_FORWARD_CONTAINER_TYPE_TRAITS(std::set, true);
        SPECIALIZE_DYNAMIC_STD_FORWARD_CONTAINER_TYPE_TRAITS(std::multiset, true);
        SPECIALIZE_DYNAMIC_STD_FORWARD_CONTAINER_TYPE_TRAITS(std::unordered_set, true);
        SPECIALIZE_DYNAMIC_STD_FORWARD_CONTAINER_TYPE_TRAITS(std::unordered_multiset, true);
    
        SPECIALIZE_DYNAMIC_STD_MAP_TYPE_TRAITS(std::map);
        SPECIALIZE_DYNAMIC_STD_MAP_TYPE_TRAITS(std::multimap);
        SPECIALIZE_DYNAMIC_STD_MAP_TYPE_TRAITS(std::unordered_map);
        SPECIALIZE_DYNAMIC_STD_MAP_TYPE_TRAITS(std::unordered_multimap);

#undef SPECIALIZE_DYNAMIC_STD_FORWARD_CONTAINER_TYPE_TRAITS
#undef SPECIALIZE_DYNAMIC_STD_MAP_TYPE_TRAITS
        
    }

    template <typename Ty>
    concept std_basic_type =
        std::is_same_v<Ty, char>      || std::is_same_v<Ty, unsigned char>      ||
        std::is_same_v<Ty, short>     || std::is_same_v<Ty, unsigned short>     ||
        std::is_same_v<Ty, int>       || std::is_same_v<Ty, unsigned int>       ||
        std::is_same_v<Ty, long long> || std::is_same_v<Ty, unsigned long long> ||
        std::is_same_v<Ty, float>     || std::is_same_v<Ty, double>             ||
        std::is_same_v<Ty, bool>      || detail::std_string_type_traits<Ty>::value;

    template <typename Ty>
    concept std_basic_structure = detail::std_structured_binding_type_traits<Ty>::value;
    
    template <class Ty>
    concept std_forward_container = detail::std_forward_container_type_traits<Ty>::value;

    typedef enum std_integer_output_flags {
        integer_neat_type     = 1 << 0, // Use (u)intX_t
        integer_form_binary   = 1 << 1,
        integer_form_heximal  = 1 << 2,
        integer_case_upper    = 1 << 3, // Default is lower case.
    } std_integer_output_flags;

    typedef enum std_floating_point_output_flags {
        floating_point_fixed             = 1 << 0, // Clip if out of range and fill 0 if not enough.
        floating_point_scientific        = 1 << 1,
        floating_point_char_upper        = 1 << 3
    } std_floating_point_output_flags;
    
    ////////////////////////////////////
    ///    Detail implementation
    ////////////////////////////////////    
    namespace detail {

        //////////////////////////////////
        ///     Output Formatter Base
        //////////////////////////////////

        template <class Sub, bool IsRange>
        struct text_output_formatter_base {
            std::string      type_string;
            std::size_t      length = 0;
            flag_t           flag   = 0;
            archive*         archive_ptr;

            // Can be unrequired, but I just want to add this since most output formatter contains this.
            void context_setup() {
                static_cast<Sub*>(this)->context_setup();
            }
            
            template <typename Ty>
            void output_single_value(const Ty& v) {
                static_cast<Sub*>(this)->output_single_value(v);
            }

            template <std::forward_iterator FwdIt>
            void output_values(std::string_view name, FwdIt it) {
                context_setup();
                archive_ptr->content().append(type_string).push_back(' ');
                archive_ptr->content().append(name);
                if constexpr (!IsRange) {
                    archive_ptr->content().push_back('=');
                    output_single_value(*it);                
                    archive_ptr->content().push_back(';');
                } else {
                    char  length_str[32] = "\0"; *length_str = '[';
                    char* length_end = std::to_chars(length_str + 1, length_str + 32, length).ptr;
                    *length_end = ']'; ++length_end;
                    *length_end = '='; ++length_end;
                    *length_end = '{'; ++length_end;
                    archive_ptr->content().append(length_str); 
                    for (std::size_t i = 0; i != length; ++i) {
                        if constexpr (std::contiguous_iterator<FwdIt>) {
                            output_single_value(it[i]);
                        } else {
                            output_single_value(*it++);
                        }
                        archive_ptr->content().push_back(',');
                    }
                    archive_ptr->content().back() = '}';
                    archive_ptr->content().push_back(';');
                }
            }
        };

        /////////////////////////////////////////////
        /// Output Formater Implementation
        ///////////////////////////////////////////// 
        
        template <typename Ty, bool IsRange>
        struct text_output_formatter {};

#define TEXT_OUTPUT_FORMATTER_INHERIT_METHODS() \
        using text_output_formatter_base<text_output_formatter, IsRange>::type_string; \
        using text_output_formatter_base<text_output_formatter, IsRange>::flag;        \
        using text_output_formatter_base<text_output_formatter, IsRange>::archive_ptr  \
        
        template <std::integral Int, bool IsRange> requires !std::is_same_v<Int, bool>
        struct text_output_formatter<Int, IsRange> : text_output_formatter_base<text_output_formatter<Int, IsRange>, IsRange> {
            TEXT_OUTPUT_FORMATTER_INHERIT_METHODS();
            std::string_view      neat_type_string;
            std::string_view      raw_type_string;
            char                  base = 10;

            void context_setup() {
                type_string = (flag & integer_neat_type) ? neat_type_string : raw_type_string;
                base        = (flag & integer_form_binary) ? 2 : (flag & integer_form_heximal) ? 16 : 10;
            }
            
            template <std::integral Ty>
            void output_single_value(const Ty& v) {
                char  value[512] = "\0";
                char* value_begin = value;
                switch (base) {
                case 16:
                    value[0] = '0';
                    value[1] = 'x';
                    value_begin += 2; break;
                case 2:
                    value[0] = '0';
                    value[1] = 'b';
                    value_begin += 2; break;
                default: break;
                }

                char*  vend = std::to_chars(value_begin, value + 512, v, base).ptr;
                if (base != 10) {
                    std::for_each(value_begin, vend, [this](char& a) {
                        a = (flag & integer_case_upper) ? std::toupper(a) : std::tolower(a);
                    });
                }
                archive_ptr->content().append(value, vend - value);
            }
        };

        template <std::floating_point Float, bool IsRange>
        struct text_output_formatter<Float, IsRange> : text_output_formatter_base<text_output_formatter<Float, IsRange>, IsRange> {
            TEXT_OUTPUT_FORMATTER_INHERIT_METHODS();
            std::chars_format fmt = std::chars_format::general;
            
            void context_setup() {
                if (flag & floating_point_fixed)      { fmt = std::chars_format::fixed; }
                if (flag & floating_point_scientific) { fmt = std::chars_format::scientific; }
            }
            template <std::floating_point Ty>
            void output_single_value(const Ty& v) {
                char  value[512] = "\0";
                char* value_end = std::to_chars(value, value + 512, v, fmt).ptr;
                // Change exponent to upper case.
                if (flag & floating_point_char_upper)  {
                    *std::find(value, value_end, 'e') = 'E';
                }
                archive_ptr->content().append(value, value_end - value);
            }
        };

        template <bool IsRange>
        struct text_output_formatter<bool, IsRange> : text_output_formatter_base<text_output_formatter<bool, IsRange>, IsRange> {
            TEXT_OUTPUT_FORMATTER_INHERIT_METHODS();
            void context_setup() {
                type_string = "bool";
            }
            void output_single_value(const bool& v) {
                archive_ptr->content().append(v ? "true" : "false");
            }
        };

        // Raw string is not supported for now.
        template <class String, bool IsRange> requires std_string_type_traits<String>::value
        struct text_output_formatter<String, IsRange> : text_output_formatter_base<text_output_formatter<String, IsRange>, IsRange> {
            TEXT_OUTPUT_FORMATTER_INHERIT_METHODS();
            void context_setup() {
                type_string = "string";
            }
            void output_single_value(std::string_view v) {
                // Do string handling.
                std::string strbuf(v.size() + 2, '\0');
                std::copy_n(v.begin(), v.size(), strbuf.begin() + 1);
                for (auto i = strbuf.begin(); i != strbuf.end(); ++i) {
                    switch (*i) {
                        default: break;
                        case '\"' : *i = '\"'; i = strbuf.insert(i, '\\'); ++i; break;
                        case '\\' : *i = '\\'; i = strbuf.insert(i, '\\'); ++i; break;
                        case '\t' : *i = 't';  i = strbuf.insert(i, '\\'); ++i; break;
                        case '\n' : *i = 'n';  i = strbuf.insert(i, '\\'); ++i; break;
                        case '\r' : *i = 'r';  i = strbuf.insert(i, '\\'); ++i; break;
                        case '\v' : *i = 'v';  i = strbuf.insert(i, '\\'); ++i; break;
                        case '\f' : *i = 'f';  i = strbuf.insert(i, '\\'); ++i; break;
                        case '\a' : *i = 'a';  i = strbuf.insert(i, '\\'); ++i; break;
                        case '\b' : *i = 'b';  i = strbuf.insert(i, '\\'); ++i; break;
                    }
                }
                // Add quote.
                strbuf.front() = '\"';
                strbuf.back()  = '\"';
                archive_ptr->content().append(strbuf);
            }
        };

        template <std_basic_structure Structure, bool IsRange>
        struct text_output_formatter<Structure, IsRange> : text_output_formatter_base<text_output_formatter<Structure, IsRange>, IsRange> {
            TEXT_OUTPUT_FORMATTER_INHERIT_METHODS();
            void context_setup() {
                type_string = detail::get_tree_structure_type_string<Structure>();
            }
            void output_single_value(const Structure& v) {
                auto formatter = [this]<typename T0>(const T0& lv) {
                    text_output_formatter<T0, false> out;
                    out.archive_ptr       = archive_ptr;
                    out.flag              = {};
                    out.output_single_value(lv);
                };
                detail::get_tree_structure_value_string(v, archive_ptr->content(), formatter);
            }
        };
        
#undef TEXT_OUTPUT_FORMATTER_INHERIT_METHODS
    }

    /////////////////////////////////////
    /// Basic serializer specialization
    /////////////////////////////////////
    
    template <std_basic_type Ty>
    struct serializer<Ty>  {
        constexpr void operator()(archive& arch, std::string_view name, const Ty& v, flag_t flag) {
            detail::text_output_formatter<Ty, false> out;
            out.archive_ptr       = &arch;
            out.flag              = flag;
            if constexpr (std::is_integral_v<Ty> && !std::is_same_v<Ty, bool>) {
                out.raw_type_string   = detail::std_basic_type_string<Ty>::raw;
                out.neat_type_string  = detail::std_basic_type_string<Ty>::neat;
            }
            if constexpr (std::is_floating_point_v<Ty>) {
                out.type_string       = detail::std_basic_type_string<Ty>::default_name;
            }
            out.output_values(name, &v);
        }
        constexpr void operator()(archive& arch, std::string::const_iterator& mem_begin, Ty& v) {
            // TODO IMPLEMENT ME!
        }
    };

    template <std_forward_container SimpleContainer> requires std_basic_type<typename SimpleContainer::value_type>
    struct serializer<SimpleContainer> {
        constexpr void operator()(archive& arch, std::string_view name, const SimpleContainer& c, flag_t flag) {
            using value_type = typename SimpleContainer::value_type;
            detail::text_output_formatter<value_type, true> out;
            out.archive_ptr       = &arch;
            out.flag              = flag;
            out.length            = c.size();
            if constexpr (std::is_integral_v<value_type> && !std::is_same_v<value_type, bool>) {
                out.raw_type_string   = detail::std_basic_type_string<value_type>::raw;
                out.neat_type_string  = detail::std_basic_type_string<value_type>::neat;
            }
            if constexpr (std::is_floating_point_v<value_type>) {
                out.type_string       = detail::std_basic_type_string<value_type>::default_name;
            }
            out.output_values(name, c.cbegin());
        }
        constexpr void operator()(archive& arch, std::string::const_iterator& mem_begin, SimpleContainer& v) {
            // TODO IMPLEMENT ME!
        }
    };

    template <std_basic_structure Structure>
    struct serializer<Structure> {
        constexpr void operator()(archive& arch, std::string_view name, const Structure& v, flag_t flag) {
            detail::text_output_formatter<Structure, false> out;
            out.archive_ptr       = &arch;
            out.flag              = flag;
            out.output_values(name, &v);
        }
        constexpr void operator()(archive& arch, std::string::const_iterator& mem_begin, Structure& v) {
            // TODO IMPLEMENT ME!
        }
    };

    template <std_forward_container StructureContainer> requires std_basic_structure<typename StructureContainer::value_type>
    struct serializer<StructureContainer> {
        constexpr void operator()(archive& arch, std::string_view name, const StructureContainer& v, flag_t flag) {
            detail::text_output_formatter<typename StructureContainer::value_type, true> out;
            out.archive_ptr       = &arch;
            out.flag              = flag;
            out.length            = v.size();
            out.output_values(name, v.cbegin());
        }
        constexpr void operator()(archive& arch, std::string::const_iterator& mem_begin, StructureContainer& v) {
            // TODO IMPLEMENT ME!
        }
    };
}