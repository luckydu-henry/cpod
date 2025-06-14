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

#include "cpod.hpp"

namespace cpod {

    class text_archive;
    class binary_archive;

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

    template <class Ty>
    struct std_structured_binding_type_traits : std::false_type {};

    template <typename K, typename V>
    struct std_structured_binding_type_traits<std::pair<K, V>> : std::true_type {
        static constexpr bool is_pair = true;
    };

    template <typename ... Types>
    struct std_structured_binding_type_traits<std::tuple<Types...>> : std::true_type {
        static constexpr bool          is_pair = false;
        static constexpr std::size_t   length  = sizeof...(Types);
    };
    
    // We don't have std::(multi)map or std::unordered_(multi)map support.
    // Since maps are very special, you might want to serialize them with a particular custom struct.
    template <class Ty>
    struct std_forward_container_type_traits : std::false_type {};

    // Two static container
    template <class Ty, std::size_t N>
    struct std_forward_container_type_traits<std::array<Ty, N>> : std::true_type {
        static constexpr std::string_view name = "std::array";
    };

    template <class Ty, std::size_t N>
    struct std_forward_container_type_traits<std::span<Ty, N>>  : std::true_type {
        static constexpr std::string_view name = "std::span";
    };

    // We don't have container adaptors supportness i.e. stack and queue and future flat_map flat_set.
#define SPECIALIZE_DYNAMIC_STD_FORWARD_CONTAINER_TYPE_TRAITS(container) \
    template <class Ty, template <class> class Allocator> \
    struct std_forward_container_type_traits<container##<Ty, Allocator<Ty>>> : std::true_type { \
        static constexpr std::string_view name = #container; \
    }

    SPECIALIZE_DYNAMIC_STD_FORWARD_CONTAINER_TYPE_TRAITS(std::vector);
    SPECIALIZE_DYNAMIC_STD_FORWARD_CONTAINER_TYPE_TRAITS(std::deque);
    SPECIALIZE_DYNAMIC_STD_FORWARD_CONTAINER_TYPE_TRAITS(std::list);
    SPECIALIZE_DYNAMIC_STD_FORWARD_CONTAINER_TYPE_TRAITS(std::forward_list);
    SPECIALIZE_DYNAMIC_STD_FORWARD_CONTAINER_TYPE_TRAITS(std::set);
    SPECIALIZE_DYNAMIC_STD_FORWARD_CONTAINER_TYPE_TRAITS(std::multiset);
    SPECIALIZE_DYNAMIC_STD_FORWARD_CONTAINER_TYPE_TRAITS(std::unordered_set);
    SPECIALIZE_DYNAMIC_STD_FORWARD_CONTAINER_TYPE_TRAITS(std::unordered_multiset);

#undef SPECIALIZE_DYNAMIC_STD_FORWARD_CONTAINER_TYPE_TRAITS
    
    template <typename Ty>
    concept std_basic_type =
        std::is_same_v<Ty, char>      || std::is_same_v<Ty, unsigned char>      ||
        std::is_same_v<Ty, short>     || std::is_same_v<Ty, unsigned short>     ||
        std::is_same_v<Ty, int>       || std::is_same_v<Ty, unsigned int>       ||
        std::is_same_v<Ty, long long> || std::is_same_v<Ty, unsigned long long> ||
        std::is_same_v<Ty, float>     || std::is_same_v<Ty, double>             ||
        std::is_same_v<Ty, bool>      || std_string_type_traits<Ty>::value;

    template <typename Ty>
    concept std_structure = std_structured_binding_type_traits<Ty>::value;
    
    template <class Ty>
    concept std_forward_container = std_forward_container_type_traits<Ty>::value;
    
    // Serializer handles struct as its unique unit.
    // Each struct contains only basic types and string and arrays.
    template <typename Class> requires std::is_class_v<Class>
    struct serializer {
        constexpr void text_put    (std::string_view name, text_archive&,   const Class&, uint32_t);
        constexpr void text_get    (std::string_view name, text_archive&,         Class&, uint32_t);
        constexpr void binary_put  (std::string_view name, binary_archive&, const Class&, uint32_t);
        constexpr void binary_get  (std::string_view name, binary_archive&,       Class&, uint32_t);
    };

    enum std_integer_output_flags {
        integer_neat_type     = 1 << 0, // Use (u)intX_t
        integer_form_binary   = 1 << 1,
        integer_form_heximal  = 1 << 2,
        integer_case_upper    = 1 << 3, // Default is lower case.
    };

    enum std_floating_point_output_flags {
        floating_point_fixed             = 1 << 0, // Clip if out of range and fill 0 if not enough.
        floating_point_scientific        = 1 << 1,
        floating_point_char_upper        = 1 << 3
    };

    namespace detail {
        
        template <class Ty>
        struct std_basic_type_string {};

#define DEFINE_STD_TYPE_STRING_WITH_ALIASES(type, r, n)                         \
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

        //////////////////////////////
        // For string quote matching
        //////////////////////////////
    
        struct quote_counter_context {
            std::size_t count = 0;
            void increase()                           { ++count; }
            bool should_increase(const char* const a) { return *a == '\"' && a[-1] != '\\';  }
            bool is_quotes_matched() const            { return (count & 1) == 0; }
        };

        // ///////////////////////////////////
        // Text formatters and checkers
        // ///////////////////////////////////
        
        // These are 'each-character' de-formatter, which means they will loop through the string buffer to remove
        // all unnecessary characters and so text_archive can then handle them.
        inline void remove_string_comments(std::string& str) {
            quote_counter_context qc;
            for (auto i = str.begin(); i != str.end(); ++i) {
                if (qc.should_increase(&*i)) {
                    qc.increase();
                }
                // Not inside string.
                else if (qc.is_quotes_matched()) {
                    if (*i == '/') {
                        // Single line comment.
                        if (i[1] == '/') {
                            auto j = std::find(i, str.end(), '\n');
                            i = str.erase(i, j);
                        }
                        // Multiple lines comment.
                        else if (i[1] == '*') {
                            std::size_t d = i - str.begin();
                            std::size_t j = str.find("*/", d);
                            i = str.erase(str.begin() + d, str.begin() + j + 2);
                        }
                        else {
                            throw std::invalid_argument("Invalid character after /!");
                        }
                        if (i > str.begin()) { --i; }
                    }
                }
            }
        }
#define USELESS_SPACES_PREFIX_CHECK(c) ((c) != ';' && (c) != '{' && (c) != '}' && (c) != ',' && (c) != '<' && (c) != '>')
        inline void remove_string_useless_spaces(std::string& str) { // Including \n and \t and so on.
            // Remove space characters at the end since *end() can't be empty.
            for (std::int8_t
                i = str.back();
                std::isspace(i);
                i = str.back()) { str.pop_back(); }
        
            quote_counter_context qc;
            for (auto i = str.begin(); i != str.end(); ++i) {
                if (qc.should_increase(&*i)) {
                    qc.increase();
                }
                // Not inside string.
                else if (qc.is_quotes_matched()) {
                    if (std::isspace(*i)) {
                        auto j = std::find_if_not(i, str.end(), std::isspace);
                        if ((std::isalpha(*j) || *j == '_') && i != str.begin() &&
                            USELESS_SPACES_PREFIX_CHECK(i[-1])) {
                            ++i;
                            }
                        // This is not quite elegant.
                        const bool bool_check_true = std::string_view(j, j + std::min<std::size_t>(str.end() - j, 4)) == "true";
                        const bool bool_check_false = std::string_view(j, j + std::min<std::size_t>(str.end() - j, 5)) == "false";
                        if  (bool_check_false || bool_check_true) {
                            --i; // Remove this additional space.
                        }
                        j = str.erase(i, j);
                        if (j > str.begin()) { --i; }
                    }
                }
            }
#undef USELESS_SPACES_PREFIX_CHECK
        }
        
        inline void combine_multiline_string_quotes(std::string& str) {
            quote_counter_context qc;
            for (auto i = str.begin(); i != str.end(); ++i) {
                if (qc.should_increase(&*i)) {
                    if (!qc.is_quotes_matched() && i[1] == '\"') {
                        auto j = str.erase(i, i + 2);
                        i = j > str.begin() ? --i : i;
                    }
                    else {
                        qc.increase();
                    }
                }
            }
        }
        
        inline int  check_curly_bracket_matching(std::string_view rng) {
            quote_counter_context qc;
            int bracket_count = 0;
            for (const char& i : rng) {
                if (qc.should_increase(&i)) {
                    qc.increase();
                }
                else if (qc.is_quotes_matched()) {
                    switch (i) {
                    case '{': ++bracket_count; break;
                    case '}': --bracket_count; break;
                    default : break;
                    }
                }
            }
            return bracket_count;
        }

        inline auto check_quote_context(std::string_view rng) {
            quote_counter_context qc;
            for (const char& i : rng) {
                if (qc.should_increase(&i)) {
                    qc.increase();
                }
            }
            return qc;
        }

        ////////////////////////////////////
        /// Basic type reader and writer
        ///////////////////////////////////
        
        template <std_basic_type Ty>
        void text_basic_type_put(text_archive& a, std::string_view name, const Ty& v, std::uint32_t flag);
        template <std_forward_container Cont>
        void text_basic_type_fsc_put(text_archive& a, std::string_view name, const Cont& c, std::uint32_t flag);
        template <std_structure Sb>
        void text_basic_structure_put(text_archive& a, std::string_view name, const Sb& v);
        
        template <std_basic_type Ty>
        void text_basic_type_get(text_archive& a, std::string_view name, Ty& v);
        template <std_forward_container Cont>
        void text_basic_type_fsc_get(text_archive& a, std::string_view name, Cont& c);

    }

    // Wide string is a mess and also we don't have convenient std::(to)from_wchars functionalaties
    // So we don't support wchar_t as serialization type nor archive character type.
    
    class text_archive {
        std::string           content_;
    public:

        text_archive() = default;
        text_archive(const std::string& right) : content_(right) {}
        
        inline std::string&          content() noexcept { return content_; }
        
        inline text_archive&         app(std::string_view s) {
            content_.append(s);
            return *this;
        }

        text_archive& operator<<(std::string_view s) {
            return app(s);
        }

       void          normalize() {
            // Normalize passes.
            detail::remove_string_comments(content_);
            detail::remove_string_useless_spaces(content_);
            detail::combine_multiline_string_quotes(content_);
        }
        
        template <typename Ty>
        text_archive& put(std::string_view name, const Ty& a, uint32_t flag = 0) {
            using type = std::remove_cvref_t<Ty>;
            // Range type.
            if constexpr (std_forward_container<type>) {
                static_assert(std_basic_type<typename type::value_type>);
                detail::text_basic_type_fsc_put(*this,name, a, flag);
            }
            else if constexpr (std_basic_type<type>) {
                detail::text_basic_type_put(*this,name, a, flag);
            } else {
                serializer<type>{}.text_put(*this, name, a, flag);
            }
            return *this;
        }

        // We only care about memory representation inside our program, so flags will be ignored,
        // Which means you can't get flag you used to send text to the buffer.
        // Name won't be saved, this is just a search reference.
        template <typename Ty>
        text_archive& get(std::string_view name, Ty& a) {
            // We assume this method handles buffer that already normalized.
            using type = std::remove_cvref_t<Ty>;
            // Range type.
            if constexpr (std_forward_container<type>) {
                static_assert(std_basic_type<typename type::value_type>);
                detail::text_basic_type_fsc_get(*this,name, a);
            }
            else if constexpr (std_basic_type<type>) {
                detail::text_basic_type_get(*this,name, a);
            } else {
                serializer<type>{}.text_put(*this, name, a);
            }
            return *this;
        }
        
    };

    //////////////////////////////
    // Implementation namespace
    /////////////////////////////
    
    namespace detail {
        
        //////////////////////////////////
        ///     Output Formatter Base
        //////////////////////////////////

        template <class Sub, bool IsRange>
        struct text_output_formatter_base {
            std::string_view type_string;
            std::size_t      length = 0;
            std::uint32_t    flag   = 0;
            text_archive*    archive_ptr;

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
                    archive_ptr->content().pop_back();
                    archive_ptr->content().append("};");
                }
            }
        };

        /////////////////////////////////////////////
        /// Output Formater Implementation
        ///////////////////////////////////////////// 
        
        template <typename Ty, bool IsRange>
        struct text_output_formatter {};

        template <std::integral Int, bool IsRange> requires !std::is_same_v<Int, bool>
        struct text_output_formatter<Int, IsRange> : text_output_formatter_base<text_output_formatter<Int, IsRange>, IsRange> {
            using text_output_formatter_base<text_output_formatter, IsRange>::type_string;
            using text_output_formatter_base<text_output_formatter, IsRange>::flag;
            using text_output_formatter_base<text_output_formatter, IsRange>::archive_ptr;
            
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
            using text_output_formatter_base<text_output_formatter, IsRange>::type_string;
            using text_output_formatter_base<text_output_formatter, IsRange>::flag;
            using text_output_formatter_base<text_output_formatter, IsRange>::archive_ptr;
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
            using text_output_formatter_base<text_output_formatter, IsRange>::type_string;
            using text_output_formatter_base<text_output_formatter, IsRange>::flag;
            using text_output_formatter_base<text_output_formatter, IsRange>::archive_ptr;
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
            using text_output_formatter_base<text_output_formatter, IsRange>::type_string;
            using text_output_formatter_base<text_output_formatter, IsRange>::flag;
            using text_output_formatter_base<text_output_formatter, IsRange>::archive_ptr;
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

        

        ////////////////////////////////////
        /// Input Formatter Base
        ////////////////////////////////////
        
        template <class Sub, bool IsRange>
        struct text_input_formatter_base {
            std::string_view search_range;
            std::string_view type_string_aliases;
            std::size_t      length = 0;

            template <class Ty>
            void input_single_value(std::string_view name, Ty& v) {
                static_cast<Sub*>(this)->input_single_value(name, v);
            }
            std::string_view search_value_range(std::string_view name) {
                // TODO: MAKE IT UTF-8 SAFE!
                using view = std::string_view;
                // Get aliases range.
                auto alias_subrange =  type_string_aliases
                | std::views::split(';')
                | std::views::transform([name](const auto& s) {
                     return (std::string(s.begin(),s.end()) + " ").append(name);
                });
                std::uint8_t result_mask = 0;
                for (auto i = alias_subrange.begin(); i != alias_subrange.end(); ++i) {
                    for (std::size_t
                        b = search_range.find(*i);
                        b != view::npos;
                        b = search_range.find(*i)) {
                        const int         count = detail::check_curly_bracket_matching(view(search_range.data(), b));
                              std::size_t off   = b + (*i).size() + 1;
                        // Means we have found one in the most outside region.
                        if (count == 0) {
                            std::size_t beg = off;
                            std::size_t end = search_range.find(';', beg);
                            // If ; inside a string we should do further searching.
                            for(;end != view::npos && !check_quote_context(view(search_range.data(), end)).is_quotes_matched();
                                 end = search_range.find(';', beg)) {
                                beg = end + 1;
                            }
                            if (end == view::npos) {
                                throw std::invalid_argument("Missing ; after a variable filed!");
                            }
                            if constexpr (!IsRange) {
                                return view(search_range.data() + off, end - off + 1);   
                            }
                            const std::size_t index_end = search_range.find(']', off);
                            std::from_chars(search_range.data() + off, search_range.data() + index_end, length);
                            return view(search_range.data() + index_end + 3, end - index_end - 3);
                        }
                        // Means a need for further searching.
                        search_range = view(search_range.begin() + static_cast<std::ptrdiff_t>(off), search_range.end());
                    } // for (search_range)
                    result_mask |= (1 << std::distance(alias_subrange.begin(), i));
                } // for (alias_subrange)
                
                // Check result status.
                switch (result_mask) {
                default: 
                case 0x03: throw std::invalid_argument("Variable not found, probably not in the outside scope!");
                case 0x00: throw std::invalid_argument("Variable with same type defined repeatedly!");
                }
            }

            template <bool IsNotSet, std::forward_iterator FwdIt, std_forward_container Cont>
            void container_insert_value(std::string_view string_value, FwdIt& it, Cont& cont, std::size_t i) {
                if constexpr (std::contiguous_iterator<FwdIt>) {
                    input_single_value(string_value, it[i]);
                }
                else { // Not vector nor deque but is capable of using iterator fill.
                    if constexpr (IsNotSet) {
                        input_single_value(string_value, *it++);
                    }
                    else { // Sets can only use insert.
                        std::iter_value_t<FwdIt> cache;
                        input_single_value(string_value, cache);
                        cont.insert(cache);
                    }
                }
            }
            
            template <std::forward_iterator FwdIt, std_forward_container Cont>
            void input_values(std::string_view name, FwdIt it, Cont& cont) {
                auto big_range = search_value_range(name);
                constexpr std::string_view cont_name = std_forward_container_type_traits<Cont>::name;
                constexpr bool container_is_not_set =
                    cont_name != "std::set" && cont_name != "std::multiset" &&
                    cont_name != "std::unordered_set" && cont_name != "std::unordered_multiset";
                // Only dynamic containers and not set containers supports resize construction mode.
                if constexpr (container_is_not_set && cont_name != "std::array" && cont_name != "std::span") {
                    cont.resize(length);
                }
                // In case our iterator gets useless.
                it = cont.begin();
                for (std::size_t i = 0; i != length - 1; ++i) {  
                    const std::size_t       value_len    = big_range.find(',') + 1;
                    const std::string_view  string_value = std::string_view(big_range.data(), value_len); 
                    container_insert_value<container_is_not_set>(string_value, it, cont, i);
                    big_range = std::string_view(big_range.begin() + static_cast<std::ptrdiff_t>(value_len), big_range.end()); 
                }
                const std::size_t bracket_len = big_range.find('}') + 1;
                container_insert_value<container_is_not_set>(std::string_view(big_range.data(), bracket_len), it, cont, length - 1);
            }

            template <std_basic_type Ty>
            void input_values(std::string_view name, Ty& v) {
                auto big_range = search_value_range(name);
                input_single_value(big_range, v);
            }
        };

        /////////////////////////////////////////
        /// Input Formatter Implementation
        /////////////////////////////////////////

        template <typename Ty, bool IsRange>
        struct text_input_formatter {};

        template <std::integral Int, bool IsRange> requires !std::is_same_v<Int, bool>
        struct text_input_formatter<Int, IsRange> : text_input_formatter_base<text_input_formatter<Int, IsRange>, IsRange> {
            template <std::integral Ty>
            void input_single_value(std::string_view range, Ty& v) {
                char         base = 10;
                const char*  beg = range.data();
                if (range[0] == '0') {
                    base = (range[1] == 'x' || range[1] == 'X') ? 16 : (range[1] == 'b' || range[1] == 'B') ? 2 : 10;
                    beg += 2;
                }
                // Input , or ; into this.
                if (auto res = std::from_chars(beg, beg + range.size() - 1, v, base);
                    static_cast<std::size_t>(res.ptr - beg) > (range.size() - 1) || res.ec != std::errc()) {
                    const std::string msg =
                        "Invalid integer value (" + std::to_string(static_cast<int>(res.ec)) + ")!, probably missing ';' after variable field!";
                    throw std::invalid_argument(msg);
                }
            }
        };

        template <bool IsRange>
        struct text_input_formatter<bool, IsRange> : text_input_formatter_base<text_input_formatter<bool, IsRange>, IsRange> {
            void input_single_value(std::string_view range, bool& v) {
                range = std::string_view(range.data(), range.size() - 1);
                if (range == "true") { v = true; return ; }
                if (range == "false") { v = false; return; }
                throw std::invalid_argument("Boolean type can not get anything else but true or false!");
            }
        };
        
        template <class String, bool IsRange> requires std_string_type_traits<String>::value
        struct text_input_formatter<String, IsRange> : text_input_formatter_base<text_input_formatter<String, IsRange>, IsRange> {
            template <class Alloc>
            void input_single_value(std::string_view range, std::basic_string<char, std::char_traits<char>, Alloc>& v) {
                // Ignore two quotes.
                v = std::string_view(range.data() + 1, range.size() - 3);
                for (auto i = v.begin(); i != v.end(); ++i) {
                    if (*i == '\\') {
                        i = v.erase(i);
                        switch (*i) {
                        default:
                        case '\"':
                        case '\\': break;
                        case 't': *i = '\t'; break;
                        case 'n': *i = '\n'; break;
                        case 'r': *i = '\r'; break;
                        case 'v': *i = '\v'; break;
                        case 'f': *i = '\f'; break;
                        case 'a': *i = '\a'; break;
                        case 'b': *i = '\b'; break;
                        }
                    }
                }
            }
        };

        template <std::floating_point Float, bool IsRange>
        struct text_input_formatter<Float, IsRange> : text_input_formatter_base<text_input_formatter<Float, IsRange>, IsRange> {
            template <std::floating_point Ty>
            void input_single_value(std::string_view range, Ty& v) {
                range = std::string_view(range.data(), range.size() - 1);
                if (auto res = std::from_chars(range.data(), range.data() + range.size(), v, std::chars_format::general); res.ec != std::errc()) {
                    const std::string msg =
                        "Invalid floating point value (" + std::to_string(static_cast<int>(res.ec)) + ")!, probably missing ';' after variable field!";
                    throw std::invalid_argument(msg);
                }
            }
        };

        ////////////////////////////////////////////
        /// Implementation of basic put and get
        ///////////////////////////////////////////

        template <std_basic_type Ty>
        void text_basic_type_put(text_archive& a, std::string_view name, const Ty& v, std::uint32_t flag) {
            text_output_formatter<Ty, false> out;
            out.archive_ptr       = &a;
            out.flag              = flag;
            if constexpr (std::is_integral_v<Ty> && !std::is_same_v<Ty, bool>) {
                out.raw_type_string   = std_basic_type_string<Ty>::raw;
                out.neat_type_string  = std_basic_type_string<Ty>::neat;
            }
            if constexpr (std::is_floating_point_v<Ty>) {
                out.type_string       = std_basic_type_string<Ty>::default_name;
            }
            out.output_values(name, &v);
        }
        
        template <std_forward_container Cont>
        void text_basic_type_fsc_put(text_archive& a, std::string_view name, const Cont& c, std::uint32_t flag) {
            using value_type = typename Cont::value_type;
            text_output_formatter<value_type, true> out;
            out.archive_ptr       = &a;
            out.flag              = flag;
            out.length            = c.size();
            if constexpr (std::is_integral_v<value_type> && !std::is_same_v<value_type, bool>) {
                out.raw_type_string   = std_basic_type_string<value_type>::raw;
                out.neat_type_string  = std_basic_type_string<value_type>::neat;
            }
            if constexpr (std::is_floating_point_v<value_type>) {
                out.type_string       = std_basic_type_string<value_type>::default_name;
            }
            out.output_values(name, c.cbegin());
        }

        template <std_basic_type Ty>
        void text_basic_type_get(text_archive& a, std::string_view name, Ty& v) {
            if constexpr (std_string_type_traits<Ty>::value) {
                static_assert(!std_string_type_traits<Ty>::is_view, "Can't use string_view to accept data from archive!");
            }
            text_input_formatter<Ty, false> in;
            in.search_range        = a.content();
            if constexpr (std::is_integral_v<Ty> && !std::is_same_v<Ty, bool>) {
                in.type_string_aliases = std_basic_type_string<Ty>::aliases;
            } else {
                in.type_string_aliases = std_basic_type_string<Ty>::default_name;
            }
            in.input_values(name, v);
        }

        template <std_forward_container Cont>
        void text_basic_type_fsc_get(text_archive& a, std::string_view name, Cont& c) {
            using value_type = typename Cont::value_type;
            if constexpr (std_string_type_traits<value_type>::value) {
                static_assert(!std_string_type_traits<value_type>::is_view, "Can't use string_view to accept data from archive!");
            }
            text_input_formatter<value_type, true> in;
            in.search_range        = a.content();
            if constexpr (std::is_integral_v<value_type> && !std::is_same_v<value_type, bool>) {
                in.type_string_aliases = std_basic_type_string<value_type>::aliases;
            } else {
                in.type_string_aliases = std_basic_type_string<value_type>::default_name;
            }
            in.input_values(name, c.begin(), c);
        }

        template <std_structure Sb>
        void text_basic_structure_put(text_archive& a, std::string_view name, const Sb& v) {
            
        }
    }
}
