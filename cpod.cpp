#include "cpod.hpp"
#include <algorithm>
#include <stdexcept>

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

    static inline bool  useless_spaces_prefix_check(const char c) { return c != ';' && c != '{' && c != '}'; }
    
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
                        useless_spaces_prefix_check(i[-1])) {
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
        text_archive*    archive_ptr;

        template <std::floating_point Ty>
        void output_single_value(const Ty& v) {
            
        }

        template <std::floating_point Ty>
        void output_values(std::string_view name, const Ty* v) {
            
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

    struct text_string_output_formatter {
        std::size_t      length = 0;
        std::uint32_t    flag;
        text_archive*    archive_ptr;

        void output_single_value(const char*& v) {
            
        }

        void output_values(std::string_view name, const char** values) {
            
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
            using view = std::string_view;
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
                    // Means we need further searching.
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

    void detail::text_basic_type_put(text_archive& a, std::string_view name, const bool& v, uint32_t flag) {
        text_bool_output_formatter out;
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

    void detail::text_basic_type_span_put(text_archive& a, std::string_view name, std::span<bool> v, uint32_t flag) {
        text_bool_output_formatter out;
        out.archive_ptr   = &a;
        out.length        = v.size();
        out.output_values(name, v.data());
    }

#define TEXT_INTEGER_SINGLE_VALUE_IN(ts)     \
    do {                                     \
        text_integer_input_formatter in;     \
        in.type_string_aliases = ts;         \
        in.search_range     = a.content();   \
        in.input_values(name, &v);           \
    } while (false) 

#define TEXT_INTEGER_ARRAY_VALUES_IN(ts)     \
    do {                                     \
        text_integer_input_formatter in;     \
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
        TEXT_INTEGER_SINGLE_VALUE_IN("char;int8_t");
    }

    void detail::text_basic_type_get(text_archive& a, std::string_view name, unsigned char& v) {
        TEXT_INTEGER_SINGLE_VALUE_IN("unsigned char;uint8_t");
    }
    
    void detail::text_basic_type_get(text_archive& a, std::string_view name, short& v) {
        TEXT_INTEGER_SINGLE_VALUE_IN("short;int16_t");
    }
    
    void detail::text_basic_type_get(text_archive& a, std::string_view name, unsigned short& v) {
        TEXT_INTEGER_SINGLE_VALUE_IN("unsigned short;uint16_t");
    }
    
    void detail::text_basic_type_get(text_archive& a, std::string_view name, int& v) {
        TEXT_INTEGER_SINGLE_VALUE_IN("int;int32_t");
    }
    
    void detail::text_basic_type_get(text_archive& a, std::string_view name, unsigned int& v) {
        TEXT_INTEGER_SINGLE_VALUE_IN("unsigned int;uint32_t");
    }
    
    void detail::text_basic_type_get(text_archive& a, std::string_view name, long long& v) {
        TEXT_INTEGER_SINGLE_VALUE_IN("long;int64_t");
    }
    
    void detail::text_basic_type_get(text_archive& a, std::string_view name, unsigned long long& v) {
        TEXT_INTEGER_SINGLE_VALUE_IN("unsigned long;uint64_t");
    }

    void detail::text_basic_type_get(text_archive& a, std::string_view name, bool& v) {
        text_bool_input_formatter in;
        in.type_string_aliases = "bool";
        in.search_range     = a.content();
        in.input_values(name, &v);
    }

    void detail::text_basic_type_span_get(text_archive& a, std::string_view name, std::span<char>& v) {
        TEXT_INTEGER_ARRAY_VALUES_IN("char;int8_t");
    }

    void detail::text_basic_type_span_get(text_archive& a, std::string_view name, std::span<unsigned char>& v) {
        TEXT_INTEGER_ARRAY_VALUES_IN("unsigned char;uint8_t");
    }
    
    void detail::text_basic_type_span_get(text_archive& a, std::string_view name, std::span<short>& v) {
        TEXT_INTEGER_ARRAY_VALUES_IN("short;int16_t");
    }
    
    void detail::text_basic_type_span_get(text_archive& a, std::string_view name, std::span<unsigned short>& v) {
        TEXT_INTEGER_ARRAY_VALUES_IN("unsigned short;uint16_t");
    }
    
    void detail::text_basic_type_span_get(text_archive& a, std::string_view name, std::span<int>& v) {
        TEXT_INTEGER_ARRAY_VALUES_IN("int;int32_t");
    }
    
    void detail::text_basic_type_span_get(text_archive& a, std::string_view name, std::span<unsigned int>& v) {
        TEXT_INTEGER_ARRAY_VALUES_IN("unsigned int;uint32_t");
    }
    
    void detail::text_basic_type_span_get(text_archive& a, std::string_view name, std::span<long long>& v) {
        TEXT_INTEGER_ARRAY_VALUES_IN("long;int64_t");
    }
    
    void detail::text_basic_type_span_get(text_archive& a, std::string_view name, std::span<unsigned long long>& v) {
        TEXT_INTEGER_ARRAY_VALUES_IN("unsigned long;uint64_t");
    }

    void detail::text_basic_type_span_get(text_archive& a, std::string_view name, std::span<bool>& v) {
        text_bool_input_formatter in;
        in.type_string_aliases = "bool";
        in.is_range = true;
        in.search_range     = a.content();
        in.input_values(name, v.data());
        v = std::remove_cvref_t<decltype(v)>(v.begin(), in.length);
    }
}
