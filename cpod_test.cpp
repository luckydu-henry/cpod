#include <iostream>
#include <optional>
#include <string>
#include <string_view>
#include "cpod.hpp"


namespace detail {
    template <std::ranges::view V, typename Tokenizer>
    class tokenize_view : public std::ranges::view_interface<tokenize_view<V, Tokenizer>> {
        V base_;
        Tokenizer tokenizer_;
        using base_iterator = std::ranges::iterator_t<V>;
        
    public:
        tokenize_view() = default;
        tokenize_view(V base, Tokenizer tokenizer) 
            : base_(std::move(base)), tokenizer_(std::move(tokenizer)) {}

        // 移动操作支持
        tokenize_view(tokenize_view&&) = default;
        tokenize_view& operator=(tokenize_view&&) = default;
        
        // 禁止复制操作
        tokenize_view(const tokenize_view&) = delete;
        tokenize_view& operator=(const tokenize_view&) = delete;

        // 迭代器实现
        class iterator {
            base_iterator current_, next_, end_;
            const Tokenizer* tokenizer_;
            std::optional<std::string_view> current_token_;
            
            void advance() {
                // 跳过空白字符
                while (next_ != end_ && std::isspace(*next_)) {
                    ++next_;
                }
                
                current_ = next_;
                if (current_ == end_) return;
                
                // 使用分词器查找下一个token
                auto [token_end, token] = (*tokenizer_)(next_, end_);
                next_ = token_end;
                current_token_ = token;
            }
            
        public:
            using iterator_category = std::forward_iterator_tag;
            using value_type = std::string_view;
            using difference_type = std::ptrdiff_t;

            iterator() = default;
            iterator(base_iterator begin, base_iterator end, const Tokenizer& tokenizer)
                : current_(begin), next_(begin), end_(end), tokenizer_(&tokenizer) 
            {
                advance();
            }

            value_type operator*() const {
                return current_token_.value();
            }

            iterator& operator++() {
                advance();
                return *this;
            }

            iterator operator++(int) {
                auto tmp = *this;
                ++*this;
                return tmp;
            }

            bool operator==(const iterator&) const = default;
            bool operator==(std::default_sentinel_t) const {
                return current_ == end_;
            }
        };

        auto begin() { 
            return iterator(std::ranges::begin(base_), 
                           std::ranges::end(base_), 
                           tokenizer_);
        }
        
        auto end() { 
            return std::default_sentinel; 
        }
    };

    // 范围适配器闭包对象
    struct TokenizeAdapter {
        template <std::ranges::viewable_range R, typename Tokenizer>
        auto operator()(R&& rng, Tokenizer tokenizer) const {
            return tokenize_view(std::forward<R>(rng) | std::views::all, std::move(tokenizer));
        }
    };
    
    inline constexpr TokenizeAdapter tokenize{};

    
    // This is not a feature complete tokenizer.
    class cpp_subset_tokenizer {
    public:


        std::pair<const char*, std::optional<std::string_view>> 
        operator()(const char* pos, const char* end) const {
        }
        
        template <class StringIt>
        constexpr auto operator()(StringIt beg, StringIt end) const {
            auto p = operator()(static_cast<const char*>(&*beg), static_cast<const char*>((&*(end - 1)) + 2));
            return std::make_pair(beg + (p.first - &*beg), p.second);
        }
    };


}

struct cpp_subset_compiler {
    std::string src;
    std::string msg;
    std::string out;
    
    static constexpr std::string_view keywords[] = {
        "int8_t",       "uint8_t",   "int16_t",        "uint16_t",
        "int",          "uint32_t",  "int64_t",        "uint64_t",
        "float",        "double",    "bool",           "std::string",    
        // Containers.
        "std::vector", "std::deque",  "std::list", "std::forward_list", "std::set", "std::multiset",
        "std::unordered_set", "std::unordered_multiset", "std::map", "std::multimap", "std::unordered_map", "std::unordered_multimap",
        "std::pair",           "std::array",             "std::tuple"
        
        // Structure.
        "struct",
    };
    static constexpr std::string_view operators[] = {
        ",", "{", "}", "<", ">", ";", "="
    };
    static constexpr std::string_view isfx[] = {
        "u", "U", "l", "L", "ll", "LL", "z", "Z", "uz", "UZ", "ul", "UL", 
        "ull", "ULL", "llu", "LLU", "zu", "ZU"
    };

    constexpr operator bool() const noexcept {
        return msg.empty();
    }

    constexpr void remove_comments() noexcept {
        out.clear();
        out.reserve(src.size());
        bool         is_within_raw = false;
        std::size_t  quote_count   = 0;
        for (std::size_t i = 0; i < src.length(); ++i) {
            switch (src[i]) {
            case 'R':
                if (src[i + 1] == '\"' && src[i + 2] == '(') {
                    is_within_raw = true;
                    i += 2; out.append("R\"(");
                } else {
                    out.push_back(src[i]);
                } break;
            case ')':
                if (src[i + 1] == '\"' && is_within_raw == true) {
                    is_within_raw = false;
                    ++i; out.append(")\"");
                } else {
                    out.push_back(src[i]);
                } break;
            case '\"':
                if (!is_within_raw) {
                    ++quote_count; 
                }
                out.push_back(src[i]); break;
            case '/':
                // Quotes matched.
                if ((quote_count & 1) == 0) {
                    // Single line comment.
                    if (src[i + 1] == '/') {
                        i = src.find('\n', i + 1);
                        if (i == std::string_view::npos) {
                            return;
                        }
                        ++i;
                    }
                    // Multi line comment.
                    else if (src[i + 1] == '*') {
                        i = src.find("*/", i + 2);
                        if (i == std::string_view::npos) {
                            return;
                        }
                        i += 2;
                    }
                    else {
                        msg = "Invalid character after /";
                        return;
                    }
                }
            default: out.push_back(src[i]); break;
            }
        }
    } // remove_comments.

    // Change all escape characters to their original forms.
    // And all string literals will be raw string (with R prefix) after this method call.
    constexpr void normalize_string_literals() noexcept {
        out.clear();
        out.reserve(out.size() + 1);
        for (std::size_t i = 0; i != src.length(); ++i) {
            switch (src[i]) {
            case 'R':
                if (src[i + 1] == '\"' && src[i + 2] == '(') {
                    std::size_t j = src.find(")\"", i + 3);
                    if (j == std::string_view::npos) {
                        msg = "Unmatched raw string literals!";
                        return;
                    }
                    out.append(std::string_view(src.data() + i, j + 2 - i));
                    i = j + 1;
                } break;
            case '\"': {
                std::size_t j = i + 1;
                out.append("\"(");
                for (; j != src.length() && (src[j] != '\"' || src[j - 1] == '\\'); ++j) {
                    if (src[j] == '\\') {
                        switch (src[j + 1]) {
                        case 'n':  out.push_back('\n'); break;
                        case 'r':  out.push_back('\r'); break;
                        case 't':  out.push_back('\t'); break;
                        case 'b':  out.push_back('\b'); break;
                        case 'f':  out.push_back('\f'); break;
                        case 'v':  out.push_back('\v'); break;
                        case '\"': out.push_back('\"'); break;
                        case '\\': out.push_back('\\'); break;
                        case '\'': out.push_back('\''); break;
                        default:
                            msg = "Invalid escape character!";
                            return;
                        }
                        ++j;
                    } else {
                        out.push_back(src[j]);
                    }
                }
                if (j == src.length()) {
                    msg = "Unmatched string quote!";
                    return;
                }
                out.append(")\"");
                i = j;
            } break;
            default:
                out.push_back(src[i]); break;
            }
        } // for loop
    } // normalize_string

    // This step must after remove comment and normalize string.
    template <typename Iter>
    constexpr void tokenize_source(Iter it) noexcept {
        for (std::size_t i = 0; i < src.length(); ++i) {
            if (std::isspace(src[i])) {
                auto p = std::find_if_not(&src[i], &src[src.length()], std::isspace);
                i = p - src.data() - 1;
            }
            else if (std::isalnum(src[i]) || src[i] == '_' || src[i] == ':') {
                auto p = std::find_if_not(&src[i], &src[src.length()], [](auto ch) {
                    return std::isalnum(ch) || ch == '_' || ch == ':';
                });
                *it++ = std::string_view(&src[i], p - &src[i]);
                i = p - src.data() - 1;
            }
            else if (src[i] == '\"') {
                // This step won't fail because we have successfully normalized all strings in normalize_string.
                std::size_t j = src.find(")\"", i + 2);
                *it++ = std::string_view(&src[i], j - i + 2);
                i = j + 1;
            }
            else if (auto op = std::string_view(&src[i], 1);
                std::find(std::begin(operators), std::end(operators), op) != std::end(operators)) {
                *it++ = op;
            }
            else if (std::isxdigit(src[i]) || src[i] == '.' || src[i] == '-' || src[i] == '+') {
                auto p = std::find_if_not(&src[i], &src[src.length()], [](auto ch) {
                    return std::isxdigit(ch) || ch == '.' || ch == '-' || ch == '+';
                });
                std::size_t j = i;
                i = p - src.data() - 1;
                if (std::find(std::begin(isfx), std::end(isfx), std::string_view(p, 1)) != std::end(isfx)) {
                    ++p; ++i;
                }
                if (std::find(std::begin(isfx), std::end(isfx), std::string_view(p, 2)) != std::end(isfx)) {
                    ++p; ++i;
                }
                if (std::find(std::begin(isfx), std::end(isfx), std::string_view(p, 3)) != std::end(isfx)) {
                    ++p; ++i;
                }
                --p;
                *it++ = std::string_view(&src[j], p - &src[j] + 1);
            }
            else {
                msg = "Invalid character!";
                return;
            }
        } // for loop.
    } // tokenize_source.

    template <cpod::detail::std_basic_type Ty>
    static constexpr Ty compile_basic_value(std::string_view value) {
        if constexpr (std::integral<Ty> && !std::is_same_v<Ty, bool>) {
            const char* beg  = value.data();
            int   base = 10;
            if (value[0] == '0') {
                if (value[1] == 'x' || value[1] == 'X') {
                    base = 16; beg += 2;
                }
                else if (value[1] == 'b' || value[1] == 'B') {
                    base = 2;  beg += 2;
                }
            }
            Ty result = 0;
            std::from_chars(beg, value.data() + value.length(), result, base);
            return result;
        }
        if constexpr (std::floating_point<Ty>) {
            Ty result = 0;
            std::from_chars(value.data(), value.data() + value.length(), result, std::chars_format::general);
            return result;
        }
        if constexpr (std::is_same_v<Ty, bool>) {
            if (value == "true") { return true; }
            if (value == "false") { return false; }
        }
    }
    
    static constexpr void compile_basic_type_to_buffer(std::string_view type, std::string_view value, std::string& buf) {
#define DEFINE_COMPILE_FIXED_VALUE(t)                             \
    do {                                                          \
    if (type == #t) {                                             \
        const auto v = compile_basic_value<t>(value);             \
        buf.append(reinterpret_cast<const char*>(&v), sizeof(v)); \
        return;                                                   \
    }} while(false)                                               

        DEFINE_COMPILE_FIXED_VALUE(int8_t);
        DEFINE_COMPILE_FIXED_VALUE(uint8_t);
        DEFINE_COMPILE_FIXED_VALUE(int16_t);
        DEFINE_COMPILE_FIXED_VALUE(uint16_t);
        DEFINE_COMPILE_FIXED_VALUE(int);
        DEFINE_COMPILE_FIXED_VALUE(uint32_t);
        DEFINE_COMPILE_FIXED_VALUE(int64_t);
        DEFINE_COMPILE_FIXED_VALUE(uint64_t);
        DEFINE_COMPILE_FIXED_VALUE(float);
        DEFINE_COMPILE_FIXED_VALUE(double);
        DEFINE_COMPILE_FIXED_VALUE(bool);
#undef DEFINE_COMPILE_FIXED_VALUE
        // String requires special handling.
        if (type == "std::string") {
            buf.append(value.data() + 2, value.length() - 4);
            buf.push_back('\0');
        }
    }

    template <char B1, char B2, class Iter>
    constexpr auto find_matching_bracket(Iter b, Iter e) {
        std::size_t brace_count = 1;
        auto i = std::next(b);
        for (; i != e && brace_count != 0; ++i) {
            if ((*i)[0] == B1) { ++brace_count; }
            if ((*i)[0] == B2) { --brace_count; }
        }
        return std::prev(i);
    }

    template <class Iter>
    constexpr auto compile_values_recursive(Iter ttb, Iter tte, Iter vtb, Iter vte, std::string& buf) {
        // Means basic type -- recursive end scenario.
        const std::size_t tid = std::find(std::begin(keywords), std::end(keywords), *ttb) - std::begin(keywords) + 1;
        if (tid < 13) {
            compile_basic_type_to_buffer(*ttb, *vtb, buf);
            return std::next(vtb);
        }
        // Template type.
        if (tid > 12 && tid < 26) {
            // Sequential containers (not map nor pair && tuple && array)
            if (tid > 12 && tid < 21) {
                std::size_t n = 0;
                auto ntte = find_matching_bracket<'<', '>'>(std::next(ttb), tte);
                auto nvte = find_matching_bracket<'{', '}'>(vtb, vte);
                std::string cache;
                ttb = std::next(ttb, 2);
                for (auto k = vtb; k != nvte; ++n) {
                    k = compile_values_recursive(ttb, ntte, std::next(k), nvte, cache);
                }
                buf.append(reinterpret_cast<const char*>(&n), sizeof(n));
                buf.append(cache);
                return std::next(nvte);
            }
        }
    }

    template <class Iter>
    constexpr std::string compile_type_name(Iter ttb, Iter tte) {
        std::string buf;
        for (auto it = ttb; it != tte; ++it) {
            if      (*it == ",") { buf.push_back(','); }
            else if (*it == "<") { buf.push_back('<'); }
            else if (*it == ">") { buf.push_back('>'); }
            else {
                const std::uint8_t t = static_cast<std::uint8_t>(std::find(std::begin(keywords), std::end(keywords), *it) - std::begin(keywords)) + 1;
                buf.push_back(*reinterpret_cast<const char*>(&t));
            }
        }
        buf.push_back('\0');
        return buf;
    }
    
    template <class Container>
    constexpr void generate_byte_code(const Container& tokens) {
        out.clear();
        out.reserve(tokens.size());
        for (auto t = tokens.begin(); t != tokens.end(); ++t) {
            // Ignore struct for now.
            if (auto i = std::find(std::begin(keywords), std::end(keywords), *t); i != std::end(keywords)) {
                if (*t == "struct") {
                }
                else {
                    std::string value_cache;
                    auto assign = std::find(t, tokens.end(), "=");
                    if (assign == tokens.end()) {
                        msg = "Missing assign operator (=).";
                        return;
                    }
                    auto semico = std::find(assign, tokens.end(), ";");
                    if (semico == tokens.end()) {
                        msg = "Missing ; after expression.";
                        return;
                    }
                    std::string type_cache = compile_type_name(t, std::prev(assign));
                    compile_values_recursive(t, std::prev(assign), std::next(assign), semico, value_cache);
                    const std::size_t offset = type_cache.size() + std::prev(assign)->size() + value_cache.size() + 1;
                    out.append(reinterpret_cast<const char*>(&offset), sizeof(std::size_t));
                    out.append(type_cache);
                    out.append(*std::prev(assign)).push_back('\0');
                    out.append(value_cache);
                    t = semico;
                }
            }
        }
    }
    
};

int main() {
    using namespace std::string_view_literals;
    using namespace std::string_literals;

    // std::string cpod_src = R"(
    // int a=-1928l;
    // std::string str = "Hello World!";
    // )";
    // std::map<std::string_view, float> Map = {
    //     {"Hello\\\"", 1.F},
    //     {"GoodJob!", 5.F}
    // };
    //
    // cpod::archive arch;
    // constexpr bool v= cpod::std_type<std::string_view>;
    //
    // arch << "// This is a cpod file.\n"
    //      << cpod::var("Int_Value", 19) << '\n'
    //      << cpod::var("Map", Map) << '\n'
    //      << cpod::var("DynamicVecF", std::vector<float>{1,2,3,4}) << '\n'
    //      << cpod::var("StaticVecF", std::array<float, 4>{1,2,3,4});
    //
    //

    std::string cpod_src = R"(std::vector<std::list<std::set<int8_t>>> a = {{{1, 2}, {1,2, 3, 4, 5, 6, 7}}};)";
    
    std::vector<std::string_view> token_list;
    
    cpp_subset_compiler compiler(std::move(cpod_src));
    compiler.remove_comments();            compiler.src = compiler.out;
    compiler.normalize_string_literals();  compiler.src = compiler.out;
    compiler.tokenize_source(std::back_inserter(token_list));
    compiler.generate_byte_code(token_list);
    
    if (compiler) {
        // for (auto i : token_list) {
        //     std::cout << i << '\n';
        // }
        for (uint8_t i : compiler.out) {
            std::cout << int(i) << '\n';
        }
    } else {
        std::cout << compiler.msg << '\n';
    }

    // for (uint8_t i : cpod::std_type_name_string<int>(true)) {
    //     std::cout << int(i) << std::endl;
    // }
    

    
    
    //
    // cpp_subset_compiler comp(std::move(cpod_src));
    // comp.remove_comments();
    //
    // std::cout << comp.out << '\n';
    
    // std::unordered_map<int, std::tuple<float, float>> map = { {1, std::make_tuple(1.F, 2.F)}, {3, std::make_tuple(1.F, 2.F)} };
    //
    // cpod::archive arch;
    // arch << cpod::var("AMap", map);
    //
    // std::cout << arch.content() << std::endl;
}