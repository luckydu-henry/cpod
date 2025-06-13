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

#include <string>
#include <span>
#include <ranges>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include <charconv>  // from_chars and to_chars

#include <array>
#include <span>
#include <vector>
#include <deque>
#include <list>
#include <forward_list>
#include <set>
#include <unordered_set>


namespace cpod {

    // A registry contains all your types that would be processed by the archive.
    class text_archive;
    class binary_archive;

    template <class Ty>
    struct std_string_traits : std::false_type {};

    template <typename CharT, template <class> class Allocator>
    struct std_string_traits<std::basic_string<CharT, std::char_traits<CharT>, Allocator<CharT>>> : std::true_type {};

    // We don't have std::(multi)map or std::unordered_(multi)map support.
    // Since maps are very special, you might want to serialize them with a particular custom struct.
    template <class Ty>
    struct forward_sequential_container_traits : std::false_type {};

    // Two static container
    template <class Ty, std::size_t N>
    struct forward_sequential_container_traits<std::array<Ty, N>> : std::true_type {
        static constexpr std::string_view name = "std::array";
    };

    template <class Ty, std::size_t N>
    struct forward_sequential_container_traits<std::span<Ty, N>>  : std::true_type {
        static constexpr std::string_view name = "std::span";
    };

    // We don't have container adaptors supportness i.e. stack and queue and future flat_map flat_set.
#define DEFINE_DYNAMIC_FORWARD_SEQUENTIAL_CONTAINER_TRAIT(container) \
    template <class Ty, template <class> class Allocator> \
    struct forward_sequential_container_traits<container##<Ty, Allocator<Ty>>> : std::true_type { \
        static constexpr std::string_view name = #container; \
    }

    DEFINE_DYNAMIC_FORWARD_SEQUENTIAL_CONTAINER_TRAIT(std::vector);
    DEFINE_DYNAMIC_FORWARD_SEQUENTIAL_CONTAINER_TRAIT(std::deque);
    DEFINE_DYNAMIC_FORWARD_SEQUENTIAL_CONTAINER_TRAIT(std::list);
    DEFINE_DYNAMIC_FORWARD_SEQUENTIAL_CONTAINER_TRAIT(std::forward_list);
    DEFINE_DYNAMIC_FORWARD_SEQUENTIAL_CONTAINER_TRAIT(std::set);
    DEFINE_DYNAMIC_FORWARD_SEQUENTIAL_CONTAINER_TRAIT(std::multiset);
    DEFINE_DYNAMIC_FORWARD_SEQUENTIAL_CONTAINER_TRAIT(std::unordered_set);
    DEFINE_DYNAMIC_FORWARD_SEQUENTIAL_CONTAINER_TRAIT(std::unordered_multiset);

#undef DEFINE_DYNAMIC_FORWARD_SEQUENTIAL_CONTAINER_TRAIT
    template <class Ty>
    concept forward_sequential_container = forward_sequential_container_traits<Ty>::value;
    
    template <typename Ty>
    concept basic_serializable_types =
        std::is_same_v<Ty, char>      || std::is_same_v<Ty, unsigned char>      ||
        std::is_same_v<Ty, short>     || std::is_same_v<Ty, unsigned short>     ||
        std::is_same_v<Ty, int>       || std::is_same_v<Ty, unsigned int>       ||
        std::is_same_v<Ty, long long> || std::is_same_v<Ty, unsigned long long> ||
        std::is_same_v<Ty, float>     || std::is_same_v<Ty, double>             ||
        std::is_same_v<Ty, bool>      || std_string_traits<Ty>::value;
    
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

#define DEFINE_STD_INTEGER_TYPE_STRING(type, r, n)               \
        template <> struct std_integer_type_string<type> {       \
            static constexpr std::string_view raw     = r;       \
            static constexpr std::string_view neat    = n;       \
            static constexpr std::string_view aliases = r##";"##n; }
        
        template <class Ty>
        struct std_integer_type_string {};
        DEFINE_STD_INTEGER_TYPE_STRING(int8_t  , "int8_t"  , "char");
        DEFINE_STD_INTEGER_TYPE_STRING(uint8_t , "uint8_t" , "unsigned char");
        DEFINE_STD_INTEGER_TYPE_STRING(int16_t , "int16_t" , "short");
        DEFINE_STD_INTEGER_TYPE_STRING(uint16_t, "uint16_t", "unsigned short");
        DEFINE_STD_INTEGER_TYPE_STRING(int     , "int"     , "int32_t");
        DEFINE_STD_INTEGER_TYPE_STRING(uint32_t, "uint32_t", "unsigned int");
        DEFINE_STD_INTEGER_TYPE_STRING(int64_t , "int64_t" , "long long");
        DEFINE_STD_INTEGER_TYPE_STRING(uint64_t, "uint64_t", "unsigned long long");
        
#undef  DEFINE_STD_INTEGER_TYPE_STRING

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
#define USELESS_SPACES_PREFIX_CHECK(c) ((c) != ';' && (c) != '{' && (c) != '}')
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

        ////////////////////////////////////
        /// Basic type reader and writer
        ///////////////////////////////////
        
        template <basic_serializable_types Ty>
        void text_basic_type_put(text_archive& a, std::string_view name, const Ty& v, std::uint32_t flag);
        template <forward_sequential_container Cont>
        void text_basic_type_fsc_put(text_archive& a, std::string_view name, const Cont& c, std::uint32_t flag);
        template <basic_serializable_types Ty>
        void text_basic_type_get(text_archive& a, std::string_view name, Ty& v);
        template <forward_sequential_container Cont>
        void text_basic_type_fsc_get(text_archive& a, std::string_view name, Cont& c);

    }

    // We only support normal character (char) as serializer type.
    
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
            if constexpr (forward_sequential_container<type>) {
                static_assert(basic_serializable_types<typename type::value_type>);
                detail::text_basic_type_fsc_put(*this,name, a, flag);
            }
            else if constexpr (basic_serializable_types<type>) {
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
            if constexpr (forward_sequential_container<type>) {
                static_assert(basic_serializable_types<typename type::value_type>);
                detail::text_basic_type_fsc_get(*this,name, a);
            }
            else if constexpr (basic_serializable_types<type>) {
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
        ///     Output Formatter
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

        template <bool IsRange>
        struct text_integer_output_formatter : text_output_formatter_base<text_integer_output_formatter<IsRange>, IsRange> {
            using text_output_formatter_base<text_integer_output_formatter, IsRange>::type_string;
            using text_output_formatter_base<text_integer_output_formatter, IsRange>::flag;
            using text_output_formatter_base<text_integer_output_formatter, IsRange>::archive_ptr;
            
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

        template <bool IsRange>
        struct text_float_output_formatter : text_output_formatter_base<text_float_output_formatter<IsRange>, IsRange> {
            using text_output_formatter_base<text_float_output_formatter, IsRange>::type_string;
            using text_output_formatter_base<text_float_output_formatter, IsRange>::flag;
            using text_output_formatter_base<text_float_output_formatter, IsRange>::archive_ptr;
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
        struct text_bool_output_formatter : text_output_formatter_base<text_bool_output_formatter<IsRange>, IsRange> {
            using text_output_formatter_base<text_bool_output_formatter, IsRange>::type_string;
            using text_output_formatter_base<text_bool_output_formatter, IsRange>::flag;
            using text_output_formatter_base<text_bool_output_formatter, IsRange>::archive_ptr;
            void context_setup() {
                type_string = "bool";
            }
            void output_single_value(const bool& v) {
                archive_ptr->content().append(v ? "true" : "false");
            }
        };

        // Raw string is not supported for now.
        template <bool IsRange>
        struct text_string_output_formatter : text_output_formatter_base<text_string_output_formatter<IsRange>, IsRange> {
            using text_output_formatter_base<text_string_output_formatter, IsRange>::type_string;
            using text_output_formatter_base<text_string_output_formatter, IsRange>::flag;
            using text_output_formatter_base<text_string_output_formatter, IsRange>::archive_ptr;
            void context_setup() {
                type_string = "string";
            }
            template <class Alloc>
            void output_single_value(const std::basic_string<char, std::char_traits<char>, Alloc>& v) {
                // Do string handling.
                std::basic_string<char, std::char_traits<char>, Alloc> strbuf(v.size() + 2, '\0');
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
        /// Input Formatter
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
                        const std::size_t off   = b + (*i).size() + 1;
                        // Means we have found one in the most outside region.
                        if (count == 0) {
                            const std::size_t end = search_range.find(';', off);
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

            template <bool IsNotSet, std::forward_iterator FwdIt, forward_sequential_container Cont>
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
            
            template <std::forward_iterator FwdIt, forward_sequential_container Cont>
            void input_values(std::string_view name, FwdIt it, Cont& cont) {
                auto big_range = search_value_range(name);
                constexpr std::string_view cont_name = forward_sequential_container_traits<Cont>::name;
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

            template <basic_serializable_types Ty>
            void input_values(std::string_view name, Ty& v) {
                auto big_range = search_value_range(name);
                input_single_value(big_range, v);
            }
        };

        template <bool IsRange>
        struct text_integer_input_formatter : text_input_formatter_base<text_integer_input_formatter<IsRange>, IsRange> {
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
        struct text_bool_input_formatter : text_input_formatter_base<text_bool_input_formatter<IsRange>, IsRange> {
            void input_single_value(std::string_view range, bool& v) {
                range = std::string_view(range.data(), range.size() - 1);
                if (range == "true") { v = true; return ; }
                if (range == "false") { v = false; return; }
                throw std::invalid_argument("Boolean type can not get anything else but true or false!");
            }
        };
        
        template <bool IsRange>
        struct text_string_input_formatter : text_input_formatter_base<text_string_input_formatter<IsRange>, IsRange> {
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

        template <bool IsRange>
        struct text_float_input_formatter : text_input_formatter_base<text_float_input_formatter<IsRange>, IsRange> {
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

        template <basic_serializable_types Ty>
        void text_basic_type_put(text_archive& a, std::string_view name, const Ty& v, std::uint32_t flag) {
            if constexpr (std::is_integral_v<Ty> && !std::is_same_v<Ty, bool>) {
                text_integer_output_formatter<false> out;
                out.raw_type_string   = std_integer_type_string<Ty>::raw;
                out.neat_type_string  = std_integer_type_string<Ty>::neat;
                out.archive_ptr       = &a;
                out.flag              = flag;
                out.output_values(name, &v); return;
            }
            if constexpr (std::is_floating_point_v<Ty>) {
                text_float_output_formatter<false> out;
                out.archive_ptr       = &a;
                out.flag              = flag;
                out.type_string       = std::is_same_v<Ty, float> ? "float" : "double";
                out.output_values(name, &v); return;
            }
            if constexpr (std_string_traits<Ty>::value) {
                text_string_output_formatter<false> out;
                out.archive_ptr       = &a;
                out.flag              = flag;
                out.output_values(name, &v); return;
            }
            if constexpr (std::is_same_v<Ty, bool>) {
                text_bool_output_formatter<false> out;
                out.archive_ptr       = &a;
                out.output_values(name, &v); return;
            }
        }
        
        template <forward_sequential_container Cont>
        void text_basic_type_fsc_put(text_archive& a, std::string_view name, const Cont& c, std::uint32_t flag) {
            using value_type = typename Cont::value_type;
            if constexpr (std::is_integral_v<value_type> && !std::is_same_v<value_type, bool>) {
                text_integer_output_formatter<true> out;
                out.raw_type_string   = std_integer_type_string<value_type>::raw;
                out.neat_type_string  = std_integer_type_string<value_type>::neat;
                out.archive_ptr       = &a;
                out.flag              = flag;
                out.length            = c.size();
                out.output_values(name, c.cbegin()); return;
            }
            if constexpr (std::is_floating_point_v<value_type>) {
                text_float_output_formatter<true> out;
                out.archive_ptr       = &a;
                out.flag              = flag;
                out.length            = c.size();
                out.type_string       = std::is_same_v<value_type, float> ? "float" : "double";
                out.output_values(name, c.cbegin()); return;
            }
            if constexpr (std_string_traits<value_type>::value) {
                text_string_output_formatter<true> out;
                out.archive_ptr       = &a;
                out.flag              = flag;
                out.length            = c.size();
                out.output_values(name, c.cbegin()); return;
            }
            if constexpr (std::is_same_v<value_type, bool>) {
                text_bool_output_formatter<true> out;
                out.archive_ptr       = &a;
                out.length            = c.size();
                out.output_values(name, c.cbegin()); return;
            }
        }

        template <basic_serializable_types Ty>
        void text_basic_type_get(text_archive& a, std::string_view name, Ty& v) {
            if constexpr (std::is_integral_v<Ty> && !std::is_same_v<Ty, bool>) {
                text_integer_input_formatter<false> in;
                in.type_string_aliases = std_integer_type_string<Ty>::aliases;
                in.search_range        = a.content();
                in.input_values(name, v);
            }
            if constexpr (std::is_floating_point_v<Ty>) {
                text_float_input_formatter<false> in;
                in.type_string_aliases = std::is_same_v<Ty, float> ? "float" : "double";
                in.search_range        = a.content();
                in.input_values(name, v);
            }
            if constexpr (std_string_traits<Ty>::value) {
                text_string_input_formatter<false> in;
                in.type_string_aliases = "string";
                in.search_range        = a.content();
                in.input_values(name, v);
            }
            if constexpr (std::is_same_v<Ty, bool>) {
                text_bool_input_formatter<false> in;
                in.type_string_aliases = "bool";
                in.search_range        = a.content();
                in.input_values(name, v);
            }
        }

        template <forward_sequential_container Cont>
        void text_basic_type_fsc_get(text_archive& a, std::string_view name, Cont& c) {
            using value_type = typename Cont::value_type;
            if constexpr (std::is_integral_v<value_type> && !std::is_same_v<value_type, bool>) {
                text_integer_input_formatter<true> in;
                in.type_string_aliases = std_integer_type_string<value_type>::aliases;
                in.search_range        = a.content();
                in.input_values(name, c.begin(), c);
            }
            if constexpr (std::is_floating_point_v<value_type>) {
                text_float_input_formatter<true> in;
                in.type_string_aliases = std::is_same_v<value_type, float> ? "float" : "double";
                in.search_range        = a.content();
                in.input_values(name, c.begin(), c);
            }
            if constexpr (std_string_traits<value_type>::value) {
                text_string_input_formatter<true> in;
                in.type_string_aliases = "string";
                in.search_range        = a.content();
                in.input_values(name, c.begin(), c);
            }
            if constexpr (std::is_same_v<value_type, bool>) {
                text_bool_input_formatter<true> in;
                in.type_string_aliases = "bool";
                in.search_range        = a.content();
                in.input_values(name, c.begin(), c);
            }
        }
    }
}
