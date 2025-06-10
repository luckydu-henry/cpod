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
#include <algorithm>
#include <stdexcept>
#include <charconv>  // from_chars and to_chars

#include "cpod.hpp"

namespace cpod {

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
    
    void detail::remove_string_comments(std::string& str) {
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
    void detail::remove_string_useless_spaces(std::string& str) {
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
                    const bool bool_check_true = std::string_view(j, j + 4) == "true";
                    const bool bool_check_false = std::string_view(j, j + 5) == "false";
                    if  (bool_check_false || bool_check_true) {
                        --i; // Remove this additional space.
                    }
                    j = str.erase(i, j);
                    i = j > str.begin() ? --i : i;
                }
            }
        }
    }

    void detail::combine_multiline_string_quotes(std::string& str) {
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

    int detail::check_curly_bracket_matching(std::string_view rng) {
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

    //////////////////////////////////
    ///     Output Formatter
    //////////////////////////////////

#define TEXT_OUTPUT_VALUES_TEMPLATE_DEFINE(name, vs, type_str)                   \
    archive_ptr->content().append(type_str).push_back(' '); \
    archive_ptr->content().append(name);                    \
    if (length == 0) {                                      \
        archive_ptr->content().push_back('=');              \
        output_single_value(*vs);                           \
        archive_ptr->content().push_back(';');              \
        return;                                             \
    }                                                       \
    char  length_str[32] = "\0"; *length_str = '[';         \
    char* length_end = std::to_chars(length_str + 1, length_str + 32, length).ptr; \
    *length_end = ']'; ++length_end; \
    *length_end = '='; ++length_end; \
    *length_end = '{'; ++length_end; \
    archive_ptr->content().append(length_str);  \
    for (std::size_t i = 0; i != length; ++i) { \
        output_single_value(vs[i]);             \
        archive_ptr->content().push_back(',');  \
    }                                           \
    archive_ptr->content().pop_back();          \
    archive_ptr->content().append("};")        
    
    struct text_integer_output_formatter {
        const char*      neat_type_string;
        const char*      raw_type_string;
        std::size_t      length = 0; // length == 0 means raw array output;
        std::uint32_t    flag;
        char             base = 10;
        text_archive*    archive_ptr;

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
        
        template <std::integral Ty>
        void output_values(std::string_view name, const Ty* values) {
            const char* type_str = (flag & integer_neat_type) ? neat_type_string : raw_type_string;
                        base     = (flag & integer_form_binary) ? 2 : (flag & integer_form_heximal) ? 16 : 10;
            TEXT_OUTPUT_VALUES_TEMPLATE_DEFINE(name, values, type_str);
        }
    };

    struct text_float_output_formatter {
        std::size_t      length = 0; // length == 0 means raw array output;
        std::uint32_t    flag;
        const char*      type_str;
        text_archive*    archive_ptr;

        template <std::floating_point Ty>
        void output_single_value(const Ty& v) {
            std::chars_format fmt = std::chars_format::general;
            if (flag & floating_point_fixed)      { fmt = std::chars_format::fixed; }
            if (flag & floating_point_scientific) { fmt = std::chars_format::scientific; }

            char  value[512] = "\0";
            char* value_end = std::to_chars(value, value + 512, v, fmt).ptr;
            // Change exponent to upper case.
            if (flag & floating_point_char_upper)  {
                *std::find(value, value_end, 'e') = 'E';
            }
            archive_ptr->content().append(value, value_end - value);
        }
        template <std::floating_point Ty>
        void output_values(std::string_view name, const Ty* v) {
            TEXT_OUTPUT_VALUES_TEMPLATE_DEFINE(name, v, type_str);
        }
    };

    struct text_bool_output_formatter {
        std::size_t      length = 0;
        text_archive*    archive_ptr;
        void output_single_value(const bool& v) {
            const char* bool_str = v ? "true" : "false";
            archive_ptr->content().append(bool_str);
        }
        void output_values(std::string_view name, const bool* values) {
            TEXT_OUTPUT_VALUES_TEMPLATE_DEFINE(name, values, "bool");
        }
    };

    // Raw string is not supported for now.
    struct text_string_output_formatter {
        std::size_t      length = 0;
        text_archive*    archive_ptr;

        template <class Alloc>
        void output_single_value(const std::basic_string<char, std::char_traits<char>, Alloc>& v) {
            // Do string handling.
            std::basic_string<char, std::char_traits<char>, Alloc> strbuf(v.size() + 2, '\0');
            std::copy_n(v.begin(), v.size(), strbuf.begin() + 1);
#define OUTPUT_CHANGE_CASE(cs, c, it) case cs : *it = c; it = strbuf.insert(it, '\\'); ++it; break 
            for (auto i = strbuf.begin(); i != strbuf.end(); ++i) {
                switch (*i) {
                    default: break;
                    OUTPUT_CHANGE_CASE('\"', '\"', i);
                    OUTPUT_CHANGE_CASE('\\', '\\', i);
                    OUTPUT_CHANGE_CASE('\t',  't', i);
                    OUTPUT_CHANGE_CASE('\n',  'n', i);
                    OUTPUT_CHANGE_CASE('\r',  'r', i);
                    OUTPUT_CHANGE_CASE('\v',  'v', i);
                    OUTPUT_CHANGE_CASE('\f',  'f', i);
                    OUTPUT_CHANGE_CASE('\a',  'a', i);
                    OUTPUT_CHANGE_CASE('\b',  'b', i);
                }
            }
            // Add quote.
            strbuf.front() = '\"';
            strbuf.back()  = '\"';
            archive_ptr->content().append(strbuf);
        }
        template <class Alloc>
        void output_values(std::string_view name, const std::basic_string<char, std::char_traits<char>, Alloc>* values) {
            TEXT_OUTPUT_VALUES_TEMPLATE_DEFINE(name, values, "string");
        }
        
    };

    ////////////////////////////////////
    /// Input Formatter
    ////////////////////////////////////
    
    struct text_input_formatter_base {
        std::string_view search_range;
        std::string_view type_string_aliases;
        std::size_t      length = 0;
        bool             is_range = false;

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
                        if (!is_range) {
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
        
    };

#define TEXT_INPUT_VALUES_TEMPLATE_DEFINE(name, v)   \
    do { auto big_range = search_value_range(name);  \
    if (!is_range) {                                 \
        input_single_value(big_range, *v);           \
        return;                                      \
    }                                                \
    for (std::size_t i = 0; i != length - 1; ++i) {  \
        const std::size_t value_len = big_range.find(',') + 1; \
        input_single_value(std::string_view(big_range.data(), value_len), v[i]); \
        big_range = std::string_view(big_range.begin() + static_cast<std::ptrdiff_t>(value_len), big_range.end()); \
    }\
    const std::size_t bracket_len = big_range.find('}') + 1; \
    input_single_value(std::string_view(big_range.data(), bracket_len), v[length - 1]); } while (false)
    
    struct text_integer_input_formatter : public text_input_formatter_base {
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

        template <std::integral Ty>
        void input_values(std::string_view name, Ty* v) {
            TEXT_INPUT_VALUES_TEMPLATE_DEFINE(name, v);
        }
    };

    struct text_bool_input_formatter : public text_input_formatter_base {
        void input_single_value(std::string_view range, bool& v) {
            range = std::string_view(range.data(), range.size() - 1);
            if (range == "true") { v = true; return ; }
            if (range == "false") { v = false; return; }
            throw std::invalid_argument("Boolean type can not get anything else but true or false!");
        }
        void input_values(std::string_view name, bool* v) {
            TEXT_INPUT_VALUES_TEMPLATE_DEFINE(name, v);
        }
    };

    struct text_string_input_formatter : text_input_formatter_base {
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
        template <class Alloc>
        void input_values(std::string_view name, std::basic_string<char, std::char_traits<char>, Alloc>* v) {
            TEXT_INPUT_VALUES_TEMPLATE_DEFINE(name, v);
        }
    };

    struct text_float_input_formatter : text_input_formatter_base {
        template <std::floating_point Ty>
        void input_single_value(std::string_view range, Ty& v) {
            range = std::string_view(range.data(), range.size() - 1);
            if (auto res = std::from_chars(range.data(), range.data() + range.size(), v, std::chars_format::general); res.ec != std::errc()) {
                const std::string msg =
                    "Invalid floating point value (" + std::to_string(static_cast<int>(res.ec)) + ")!, probably missing ';' after variable field!";
                throw std::invalid_argument(msg);
            }
        }
        template <std::floating_point Ty>
        void input_values(std::string_view name, Ty* v) {
            TEXT_INPUT_VALUES_TEMPLATE_DEFINE(name, v);
        }
    };

#define TEXT_INTEGER_VALUES_OUT(ns, rs, l, d) \
do {                                 \
text_integer_output_formatter out;   \
out.neat_type_string = ns;           \
out.raw_type_string  = rs;           \
out.length           = l;            \
out.flag             = flag;         \
out.archive_ptr      = &a;           \
out.output_values(name, d);} \
while (false)                       

    void detail::text_basic_type_put(text_archive& a, std::string_view name, const char& v, uint32_t flag) {
        TEXT_INTEGER_VALUES_OUT("int8_t", "char", 0, &v);
    }

    void detail::text_basic_type_put(text_archive& a, std::string_view name, const unsigned char& v, uint32_t flag) {
        TEXT_INTEGER_VALUES_OUT("uint8_t", "unsigned char", 0, &v);
    }
    
    void detail::text_basic_type_put(text_archive& a, std::string_view name, const short& v, uint32_t flag) {
        TEXT_INTEGER_VALUES_OUT("int16_t", "short", 0, &v);
    }
    
    void detail::text_basic_type_put(text_archive& a, std::string_view name, const unsigned short& v, uint32_t flag) {
        TEXT_INTEGER_VALUES_OUT("uint16_t", "unsigned short", 0, &v);
    }
    
    void detail::text_basic_type_put(text_archive& a, std::string_view name, const int& v, uint32_t flag) {
        TEXT_INTEGER_VALUES_OUT("int32_t", "int", 0, &v);
    }
    
    void detail::text_basic_type_put(text_archive& a, std::string_view name, const unsigned int& v, uint32_t flag) {
        TEXT_INTEGER_VALUES_OUT("uint32_t", "unsigned int", 0, &v);
    }
    
    void detail::text_basic_type_put(text_archive& a, std::string_view name, const long long& v, uint32_t flag) {
        TEXT_INTEGER_VALUES_OUT("int64_t", "long", 0, &v);
    }

    void detail::text_basic_type_put(text_archive& a, std::string_view name, const unsigned long long& v, uint32_t flag) {
        TEXT_INTEGER_VALUES_OUT("uint64_t", "unsigned long", 0, &v);
    }

    void detail::text_basic_type_put(text_archive& a, std::string_view name, const float& v, uint32_t flag) {
        text_float_output_formatter out;
        out.archive_ptr = &a;
        out.flag = flag;
        out.length = 0;
        out.type_str = "float";
        out.output_values(name, &v);
    }

    void detail::text_basic_type_put(text_archive& a, std::string_view name, const double& v, uint32_t flag) {
        text_float_output_formatter out;
        out.archive_ptr = &a;
        out.flag = flag;
        out.length = 0;
        out.type_str = "double";
        out.output_values(name, &v);
    }

    void detail::text_basic_type_put(text_archive& a, std::string_view name, const bool& v, uint32_t flag) {
        text_bool_output_formatter out;
        out.archive_ptr = &a;
        out.length = 0;
        out.output_values(name, &v);
    }

    void detail::text_basic_type_put(text_archive& a, std::string_view name, const std::string& v, uint32_t flag) {
        text_string_output_formatter out;
        out.archive_ptr = &a;
        out.length = 0;
        out.output_values(name, &v);
    }

    ///////////////////////////////////////////
    //       Span output serializers 
    ///////////////////////////////////////////
    
    void detail::text_basic_type_span_put(text_archive& a, std::string_view name, std::span<char> v, uint32_t flag) {
        TEXT_INTEGER_VALUES_OUT("int8_t", "char", v.size(), v.data());
    }

    void detail::text_basic_type_span_put(text_archive& a, std::string_view name, std::span<unsigned char> v, uint32_t flag) {
        TEXT_INTEGER_VALUES_OUT("uint8_t", "unsigned char", v.size(), v.data());
    }

    void detail::text_basic_type_span_put(text_archive& a, std::string_view name, std::span<short> v, uint32_t flag) {
        TEXT_INTEGER_VALUES_OUT("int16_t", "short", v.size(), v.data());
    }

    void detail::text_basic_type_span_put(text_archive& a, std::string_view name, std::span<unsigned short> v, uint32_t flag) {
        TEXT_INTEGER_VALUES_OUT("uint16_t", "unsigned short", v.size(), v.data());
    }

    void detail::text_basic_type_span_put(text_archive& a, std::string_view name, std::span<int> v, uint32_t flag) {
        TEXT_INTEGER_VALUES_OUT("int32_t", "int", v.size(), v.data());
    }

    void detail::text_basic_type_span_put(text_archive& a, std::string_view name, std::span<unsigned int> v, uint32_t flag) {
        TEXT_INTEGER_VALUES_OUT("uint32_t", "unsigned int", v.size(), v.data());
    }

    void detail::text_basic_type_span_put(text_archive& a, std::string_view name, std::span<long long> v, uint32_t flag) {
        TEXT_INTEGER_VALUES_OUT("int64_t", "long", v.size(), v.data());
    }

    void detail::text_basic_type_span_put(text_archive& a, std::string_view name, std::span<unsigned long long> v, uint32_t flag) {
        TEXT_INTEGER_VALUES_OUT("uint64_t", "unsigned long", v.size(), v.data());
    }

    void detail::text_basic_type_span_put(text_archive& a, std::string_view name, std::span<float> v, uint32_t flag) {
        text_float_output_formatter out;
        out.archive_ptr   = &a;
        out.length        = v.size();
        out.flag          = flag;
        out.type_str      = "float";
        out.output_values(name, v.data());
    }
    
    void detail::text_basic_type_span_put(text_archive& a, std::string_view name, std::span<double> v, uint32_t flag) {
        text_float_output_formatter out;
        out.archive_ptr   = &a;
        out.length        = v.size();
        out.flag          = flag;
        out.type_str      = "double";
        out.output_values(name, v.data());
    }

    void detail::text_basic_type_span_put(text_archive& a, std::string_view name, std::span<bool> v, uint32_t flag) {
        text_bool_output_formatter out;
        out.archive_ptr   = &a;
        out.length        = v.size();
        out.output_values(name, v.data());
    }

    void detail::text_basic_type_span_put(text_archive& a, std::string_view name, std::span<std::string> v, uint32_t flag) {
        text_string_output_formatter out;
        out.archive_ptr   = &a;
        out.length        = v.size();
        out.output_values(name, v.data());
    }

#define TEXT_SINGLE_VALUE_IN(formatter, ts)     \
    do {                                     \
        text_##formatter##_input_formatter in;     \
        in.type_string_aliases = ts;         \
        in.search_range     = a.content();   \
        in.input_values(name, &v);           \
    } while (false) 

#define TEXT_ARRAY_VALUES_IN(formatter, ts)     \
    do {                                     \
        text_##formatter##_input_formatter in;     \
        in.type_string_aliases = ts;         \
        in.is_range = true;                  \
        in.search_range     = a.content();   \
        in.input_values(name, v.data());     \
        v = std::remove_cvref_t<decltype(v)>(v.begin(), in.length); \
    } while (false) 

    ///////////////////////////////////////////
    //       Span input serializers 
    ///////////////////////////////////////////
    
    void detail::text_basic_type_get(text_archive& a, std::string_view name, char& v) {
        TEXT_SINGLE_VALUE_IN(integer, "int8_t;char");
    }

    void detail::text_basic_type_get(text_archive& a, std::string_view name, unsigned char& v) {
        TEXT_SINGLE_VALUE_IN(integer, "uint8_t;unsigned char");
    }
    
    void detail::text_basic_type_get(text_archive& a, std::string_view name, short& v) {
        TEXT_SINGLE_VALUE_IN(integer,"int16_t;short");
    }
    
    void detail::text_basic_type_get(text_archive& a, std::string_view name, unsigned short& v) {
        TEXT_SINGLE_VALUE_IN(integer,"uint16_t;unsigned short");
    }
    
    void detail::text_basic_type_get(text_archive& a, std::string_view name, int& v) {
        TEXT_SINGLE_VALUE_IN(integer,"int;int32_t");
    }
    
    void detail::text_basic_type_get(text_archive& a, std::string_view name, unsigned int& v) {
        TEXT_SINGLE_VALUE_IN(integer,"uint32_t;unsigned int");
    }
    
    void detail::text_basic_type_get(text_archive& a, std::string_view name, long long& v) {
        TEXT_SINGLE_VALUE_IN(integer,"int64_t;long");
    }
    
    void detail::text_basic_type_get(text_archive& a, std::string_view name, unsigned long long& v) {
        TEXT_SINGLE_VALUE_IN(integer,"uint64_t;unsigned long");
    }

    void detail::text_basic_type_get(text_archive& a, std::string_view name, float& v) {
        TEXT_SINGLE_VALUE_IN(float, "float");
    }
    
    void detail::text_basic_type_get(text_archive& a, std::string_view name, double& v) {
        TEXT_SINGLE_VALUE_IN(float, "double");
    }

    void detail::text_basic_type_get(text_archive& a, std::string_view name, bool& v) {
        TEXT_SINGLE_VALUE_IN(bool, "bool");
    }

    void detail::text_basic_type_get(text_archive& a, std::string_view name, std::string& v) {
        TEXT_SINGLE_VALUE_IN(string, "string");
    }

    void detail::text_basic_type_span_get(text_archive& a, std::string_view name, std::span<char>& v) {
        TEXT_ARRAY_VALUES_IN(integer,"int8_t;char");
    }

    void detail::text_basic_type_span_get(text_archive& a, std::string_view name, std::span<unsigned char>& v) {
        TEXT_ARRAY_VALUES_IN(integer,"uint8_t;unsigned char");
    }
    
    void detail::text_basic_type_span_get(text_archive& a, std::string_view name, std::span<short>& v) {
        TEXT_ARRAY_VALUES_IN(integer,"int16_t;short");
    }
    
    void detail::text_basic_type_span_get(text_archive& a, std::string_view name, std::span<unsigned short>& v) {
        TEXT_ARRAY_VALUES_IN(integer,"uint16_t;unsigned short");
    }
    
    void detail::text_basic_type_span_get(text_archive& a, std::string_view name, std::span<int>& v) {
        TEXT_ARRAY_VALUES_IN(integer,"int;int32_t");
    }
    
    void detail::text_basic_type_span_get(text_archive& a, std::string_view name, std::span<unsigned int>& v) {
        TEXT_ARRAY_VALUES_IN(integer,"uint32_t;unsigned int");
    }
    
    void detail::text_basic_type_span_get(text_archive& a, std::string_view name, std::span<long long>& v) {
        TEXT_ARRAY_VALUES_IN(integer,"int64_t;long");
    }
    
    void detail::text_basic_type_span_get(text_archive& a, std::string_view name, std::span<unsigned long long>& v) {
        TEXT_ARRAY_VALUES_IN(integer,"uint64_t;unsigned long");
    }

    void detail::text_basic_type_span_get(text_archive& a, std::string_view name, std::span<float>& v) {
        TEXT_ARRAY_VALUES_IN(float, "float");
    }
    
    void detail::text_basic_type_span_get(text_archive& a, std::string_view name, std::span<double>& v) {
        TEXT_ARRAY_VALUES_IN(float, "double");
    }

    void detail::text_basic_type_span_get(text_archive& a, std::string_view name, std::span<bool>& v) {
        TEXT_ARRAY_VALUES_IN(bool, "bool");
    }

    void detail::text_basic_type_span_get(text_archive& a, std::string_view name, std::span<std::string>& v) {
        TEXT_ARRAY_VALUES_IN(string, "string");
    }
}
