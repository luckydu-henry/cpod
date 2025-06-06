#include "cpod.hpp"
#include <algorithm>
#include <stdexcept>

namespace cpod {

    struct quote_counter_context {
        std::size_t count = 0;

        void increase() {
            ++count;
        }
        
        bool should_increase(char* a) {
            return *a == '\"' && a[-1] != '\\'; 
        }

        bool is_quotes_matched() const {
            return (count & 1) == 0;
        }
    };
    
    struct text_integer_output_formatter {
        const char*      neat_type_string;
        const char*      raw_type_string;
        std::size_t      length = 0; // length == 0 means raw array output;
        std::uint32_t    flag; 
        text_archive*    archive_ptr;

        template <std::integral Ty>
        void output_single_value(const Ty& v, char base, bool upper_case) {
            char  value[512] = "\0";
            char* vbeg = value;
            
            switch (base) {
            case 16:
                value[0] = '0';
                value[1] = 'x';
                vbeg += 2; break;
            case 2:
                value[0] = '0';
                value[1] = 'b';
                vbeg += 2; break;
            default: break;
            }

            char*  vend = std::to_chars(vbeg, value + 512, v, base).ptr;
            if (base != 10) {
                std::for_each(vbeg, vend, [upper_case](char& a) {
                    a = upper_case ? std::toupper(a) : std::tolower(a);
                });
            }
            archive_ptr->content().append(value, vend - value);
        }
        
        template <std::integral Ty>
        void output_values(std::string_view name, const Ty* values) {
            const char* type_str = (flag & integer_neat_type) ? neat_type_string : raw_type_string;
            const char  base     = (flag & integer_form_binary) ? 2 : (flag & integer_form_heximal) ? 16 : 10;

            archive_ptr->content().append(type_str).push_back(' ');
            archive_ptr->content().append(name);

            if (length == 0) {
                archive_ptr->content().push_back('=');
                output_single_value(*values, base, flag & integer_case_upper);
                archive_ptr->content().push_back(';');
                return;
            }

            char  length_str[32] = "\0"; *length_str = '[';
            char* length_end = std::to_chars(length_str + 1, length_str + 32, length).ptr;
            *length_end = ']'; ++length_end;
            *length_end = '='; ++length_end;
            *length_end = '{'; ++length_end;
            
            archive_ptr->content().append(length_str);

            for (std::size_t i = 0; i != length; ++i) {
                output_single_value(values[i], base, flag & integer_case_upper);
                archive_ptr->content().push_back(',');
            }
            archive_ptr->content().pop_back();
            archive_ptr->content().append("};");
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
            archive_ptr->content().append("bool").push_back(' ');
            archive_ptr->content().append(name);
            
            if (length == 0) {
                archive_ptr->content().push_back('=');
                output_single_value(*values);
                archive_ptr->content().push_back(';'); return;
            }

            for (std::size_t i = 0; i != length; ++i) {
                char  length_str[32] = "\0"; *length_str = '[';
                char* length_end = std::to_chars(length_str + 1, length_str + 32, length).ptr;
                *length_end = ']'; ++length_end;
                *length_end = '='; ++length_end;
                *length_end = '{'; ++length_end;
            
                archive_ptr->content().append(length_str);

                for (std::size_t i = 0; i != length; ++i) {
                    output_single_value(values[i]);
                    archive_ptr->content().push_back(',');
                }
                archive_ptr->content().pop_back();
                archive_ptr->content().append("};");
            }
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

    struct text_integer_input_formatter {
        std::string_view search_range;
        const char*      neat_type_string;
        const char*      raw_type_string;
        std::size_t*     length = nullptr;

        // We assume your file is already normalized.
        // Returns value_range.
        std::string_view search_value_range(std::string_view name) {
            std::string passes[2] = {
                (std::string(neat_type_string) + " ").append(name),
                (std::string(raw_type_string) + " ").append(name)
            };
            std::uint8_t result_mask = 0;
            for (size_t i = 0; i != 2; ++i) {
                std::size_t beg = search_range.find(passes[i]);
                if (beg == std::string::npos) {
                    result_mask |= (1 << i);
                } else {
                    // TODO: Check curly bracket matching.
                    std::size_t off = beg + passes[i].size() + 1;
                    std::size_t end = search_range.find(';', off);
                    if (end == std::string::npos) {
                        throw std::invalid_argument("Missing ; after a variable filed!");
                    }
                    return std::string_view(search_range.data() + off, end - off + 1);
                }
            }
            switch (result_mask) {
            default:
            case 0b00000011: throw std::invalid_argument("Variable not found!");
            case 0b00000000: throw std::invalid_argument("Variable with same type defined repeatedly!");
            }
        }

        template <std::integral Ty>
        void input_single_value(std::string_view range, Ty& v) {
            char         base = 10;
            const char*  beg = range.data();
            if (range[0] == '0') {
                if (range[1] == 'x') {
                    base = 16;
                }
                else if (range[1] == 'b') {
                    base = 2;
                }
                beg += 2;
            }
            // Input , or ; into this.
            std::from_chars_result result = std::from_chars(beg, beg + range.size() - 1, v, base);
            if ((result.ptr - beg) > (range.size() - 1)) {
                throw std::invalid_argument("Invalid integer value!, probably missing ';' after variable field!");
            }
        }

        template <std::integral Ty>
        void input_values(std::string_view name, Ty& v) {
            auto big_range = search_value_range(name);
            // Now work with only single value.
            input_single_value(big_range, v);
        }
        
    };

#define TEXT_INTEGER_VALUE_OUT(ns, rs, l, d) \
    do {                                 \
    text_integer_output_formatter out;   \
    out.neat_type_string = ns;           \
    out.raw_type_string  = rs;           \
    out.length           = l;            \
    out.flag             = flag;         \
    out.archive_ptr      = &a;           \
    out.output_values(name, d);} \
    while (false)                       

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

    void detail::remove_string_useless_spaces(std::string& str) {
#define prefix_satisfy(c) (c != ';' && c != '{' && c != '}')
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
                        prefix_satisfy(i[-1])) {
                        ++i;
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
                } else {
                    qc.increase();
                }
            }
        }
    }

    void detail::text_basic_type_put(text_archive& a, std::string_view name, const char& v, uint32_t flag) {
        TEXT_INTEGER_VALUE_OUT("int8_t", "char", 0, &v);
    }

    void detail::text_basic_type_put(text_archive& a, std::string_view name, const unsigned char& v, uint32_t flag) {
        TEXT_INTEGER_VALUE_OUT("uint8_t", "unsigned char", 0, &v);
    }
    
    void detail::text_basic_type_put(text_archive& a, std::string_view name, const short& v, uint32_t flag) {
        TEXT_INTEGER_VALUE_OUT("int16_t", "short", 0, &v);
    }
    
    void detail::text_basic_type_put(text_archive& a, std::string_view name, const unsigned short& v, uint32_t flag) {
        TEXT_INTEGER_VALUE_OUT("uint16_t", "unsigned short", 0, &v);
    }
    
    void detail::text_basic_type_put(text_archive& a, std::string_view name, const int& v, uint32_t flag) {
        TEXT_INTEGER_VALUE_OUT("int32_t", "int", 0, &v);
    }
    
    void detail::text_basic_type_put(text_archive& a, std::string_view name, const unsigned int& v, uint32_t flag) {
        TEXT_INTEGER_VALUE_OUT("uint32_t", "unsigned int", 0, &v);
    }
    
    void detail::text_basic_type_put(text_archive& a, std::string_view name, const long long& v, uint32_t flag) {
        TEXT_INTEGER_VALUE_OUT("int64_t", "long", 0, &v);
    }

    void detail::text_basic_type_put(text_archive& a, std::string_view name, const unsigned long long& v, uint32_t flag) {
        TEXT_INTEGER_VALUE_OUT("uint64_t", "unsigned long", 0, &v);
    }

    void detail::text_basic_type_put(text_archive& a, std::string_view name, const bool& v, uint32_t flag) {
        text_bool_output_formatter out;
        out.archive_ptr = &a;
        out.length = 0;
        out.output_values(name, &v);
    }

    // Span serializers
    

    void detail::text_basic_type_span_put(text_archive& a, std::string_view name, std::span<char> v, uint32_t flag) {
        TEXT_INTEGER_VALUE_OUT("int8_t", "char", v.size(), v.data());
    }

    void detail::text_basic_type_span_put(text_archive& a, std::string_view name, std::span<unsigned char> v, uint32_t flag) {
        TEXT_INTEGER_VALUE_OUT("uint8_t", "unsigned char", v.size(), v.data());
    }

    void detail::text_basic_type_span_put(text_archive& a, std::string_view name, std::span<short> v, uint32_t flag) {
        TEXT_INTEGER_VALUE_OUT("int16_t", "short", v.size(), v.data());
    }

    void detail::text_basic_type_span_put(text_archive& a, std::string_view name, std::span<unsigned short> v, uint32_t flag) {
        TEXT_INTEGER_VALUE_OUT("uint16_t", "unsigned short", v.size(), v.data());
    }

    void detail::text_basic_type_span_put(text_archive& a, std::string_view name, std::span<int> v, uint32_t flag) {
        TEXT_INTEGER_VALUE_OUT("int32_t", "int", v.size(), v.data());
    }

    void detail::text_basic_type_span_put(text_archive& a, std::string_view name, std::span<unsigned int> v, uint32_t flag) {
        TEXT_INTEGER_VALUE_OUT("uint32_t", "unsigned int", v.size(), v.data());
    }

    void detail::text_basic_type_span_put(text_archive& a, std::string_view name, std::span<long long> v, uint32_t flag) {
        TEXT_INTEGER_VALUE_OUT("int64_t", "long", v.size(), v.data());
    }

    void detail::text_basic_type_span_put(text_archive& a, std::string_view name, std::span<unsigned long long> v, uint32_t flag) {
        TEXT_INTEGER_VALUE_OUT("uint64_t", "unsigned long", v.size(), v.data());
    }

    void detail::text_basic_type_span_put(text_archive& a, std::string_view name, std::span<bool> v, uint32_t flag) {
        text_bool_output_formatter out;
        out.archive_ptr   = &a;
        out.length        = v.size();
        out.output_values(name, v.data());
    }

    void detail::text_basic_type_get(text_archive& a, std::string_view name, char& v) {
        text_integer_input_formatter in;
        in.neat_type_string = "int8_t";
        in.raw_type_string  = "char";
        in.search_range     = a.content();
        in.input_values(name, v);
    }

    void detail::text_basic_type_span_get(text_archive& a, std::string_view name, std::span<char> v) {
        
    }
}
