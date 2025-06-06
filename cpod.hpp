#pragma once

#include <charconv>         // We use charconv to deal with text 
#include <string>
#include <type_traits>
#include <unordered_map>    // In registry.
#include <span>
#include <ranges>

namespace cpod {

    // A registry contains all your types that would be processed by the archive.
    class registry;
    class text_archive;
    class binary_archive;
    

    template <typename Ty>
    concept basic_serializable_types =
        std::is_same_v<Ty, char>      || std::is_same_v<Ty, unsigned char>      ||
        std::is_same_v<Ty, short>     || std::is_same_v<Ty, unsigned short>     ||
        std::is_same_v<Ty, int>       || std::is_same_v<Ty, unsigned int>       ||
        std::is_same_v<Ty, long long> || std::is_same_v<Ty, unsigned long long> ||
        std::is_same_v<Ty, float>     || std::is_same_v<Ty, double>             ||
        std::is_same_v<Ty, bool>      || std::is_same_v<Ty, char*>;

    template <typename Ty>
    concept span_type = requires (Ty t) { t.data(); t.size();};
    
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
        floating_point_fixed      = 1 << 0, // Clip if out of range and fill 0 if not enough.
        floating_point_exponent   = 1 << 1,
    };

    enum std_strng_output_flags {
        string_raw         = 1 << 0, // Count newline as part of the string and add R() in string.
        string_zero_ending = 1 << 1, // Add zero to the end of the string for someone's C-sick.
    };

    namespace detail {

        // These are 'each-character' de-formatter, which means they will loop through the string buffer to remove
        // all unnecessary characters and so text_archive can then handle them.
        void remove_string_comments(std::string& str);
        void remove_string_useless_spaces(std::string& str); // Including \n and \t and so on.
        void combine_multiline_string_quotes(std::string& str);
        
        void text_basic_type_put(text_archive& a, std::string_view name, const char& v,               uint32_t flag);
        void text_basic_type_put(text_archive& a, std::string_view name, const unsigned char& v,      uint32_t flag);
        void text_basic_type_put(text_archive& a, std::string_view name, const short& v,              uint32_t flag);
        void text_basic_type_put(text_archive& a, std::string_view name, const unsigned short& v,     uint32_t flag);
        void text_basic_type_put(text_archive& a, std::string_view name, const int& v,                uint32_t flag);
        void text_basic_type_put(text_archive& a, std::string_view name, const unsigned int& v,       uint32_t flag);
        void text_basic_type_put(text_archive& a, std::string_view name, const long long& v,          uint32_t flag);
        void text_basic_type_put(text_archive& a, std::string_view name, const unsigned long long& v, uint32_t flag);
        void text_basic_type_put(text_archive& a, std::string_view name, const float& v,              uint32_t flag);
        void text_basic_type_put(text_archive& a, std::string_view name, const double& v,             uint32_t flag);
        void text_basic_type_put(text_archive& a, std::string_view name, const bool& v,               uint32_t flag);
        void text_basic_type_put(text_archive& a, std::string_view name, const char*& v,              uint32_t flag);

        // We use std::span to represent array types.
        void text_basic_type_span_put(text_archive& a, std::string_view name, std::span<char> v,                uint32_t flag);
        void text_basic_type_span_put(text_archive& a, std::string_view name, std::span<unsigned char> v,       uint32_t flag);
        void text_basic_type_span_put(text_archive& a, std::string_view name, std::span<short> v,               uint32_t flag);
        void text_basic_type_span_put(text_archive& a, std::string_view name, std::span<unsigned short> v,      uint32_t flag);
        void text_basic_type_span_put(text_archive& a, std::string_view name, std::span<int> v,                 uint32_t flag);
        void text_basic_type_span_put(text_archive& a, std::string_view name, std::span<unsigned int> v,        uint32_t flag);
        void text_basic_type_span_put(text_archive& a, std::string_view name, std::span<long long> v,           uint32_t flag);
        void text_basic_type_span_put(text_archive& a, std::string_view name, std::span<unsigned long long> v,  uint32_t flag);
        void text_basic_type_span_put(text_archive& a, std::string_view name, std::span<float> v,               uint32_t flag);
        void text_basic_type_span_put(text_archive& a, std::string_view name, std::span<double> v,              uint32_t flag);
        void text_basic_type_span_put(text_archive& a, std::string_view name, std::span<bool> v,                uint32_t flag);
        void text_basic_type_span_put(text_archive& a, std::string_view name, std::span<char*> v,               uint32_t flag);

        void text_basic_type_get(text_archive& a, std::string_view name, char& v);
        void text_basic_type_get(text_archive& a, std::string_view name, unsigned char& v);
        void text_basic_type_get(text_archive& a, std::string_view name, short& v);
        void text_basic_type_get(text_archive& a, std::string_view name, unsigned short& v);
        void text_basic_type_get(text_archive& a, std::string_view name, int& v);
        void text_basic_type_get(text_archive& a, std::string_view name, unsigned int& v);
        void text_basic_type_get(text_archive& a, std::string_view name, long long& v);
        void text_basic_type_get(text_archive& a, std::string_view name, unsigned long long& v);
        void text_basic_type_get(text_archive& a, std::string_view name, float& v);
        void text_basic_type_get(text_archive& a, std::string_view name, double& v);
        void text_basic_type_get(text_archive& a, std::string_view name, bool& v);
        void text_basic_type_get(text_archive& a, std::string_view name, char*& v);

        void text_basic_type_span_get(text_archive& a, std::string_view name, std::span<char> v             );
        void text_basic_type_span_get(text_archive& a, std::string_view name, std::span<unsigned char> v    );
        void text_basic_type_span_get(text_archive& a, std::string_view name, std::span<short> v            );
        void text_basic_type_span_get(text_archive& a, std::string_view name, std::span<unsigned short> v   );
        void text_basic_type_span_get(text_archive& a, std::string_view name, std::span<int> v              );
        void text_basic_type_span_get(text_archive& a, std::string_view name, std::span<unsigned int> v     );
        void text_basic_type_span_get(text_archive& a, std::string_view name, std::span<long long> v        );
        void text_basic_type_span_get(text_archive& a, std::string_view name, std::span<unsigned long long> v);
        void text_basic_type_span_get(text_archive& a, std::string_view name, std::span<float> v            );
        void text_basic_type_span_get(text_archive& a, std::string_view name, std::span<double> v           );
        void text_basic_type_span_get(text_archive& a, std::string_view name, std::span<bool> v             );
        void text_basic_type_span_get(text_archive& a, std::string_view name, std::span<char*> v            );
        
    }

    template <std::ranges::contiguous_range Rng>
    auto make_span(Rng&& rg) {
        return std::span<
            std::ranges::range_value_t<Rng>,
        std::dynamic_extent>(std::forward<Rng>(rg).begin(), std::forward<Rng>(rg).end());
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

       void          normalize_content() {
            // Normalize passes.
            detail::remove_string_comments(content_);
            detail::remove_string_useless_spaces(content_);
            detail::combine_multiline_string_quotes(content_);
        }
        
        template <typename Ty>
        text_archive& put(std::string_view name, const Ty& a, uint32_t flag = 0) {
            using type = std::remove_all_extents_t<Ty>;
            // Range type.
            if constexpr (span_type<type>) {
                static_assert(basic_serializable_types<typename type::value_type>);
                detail::text_basic_type_span_put(*this,name, a, flag);
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
            using type = std::remove_all_extents_t<Ty>;
            // Range type.
            if constexpr (span_type<type>) {
                static_assert(basic_serializable_types<typename type::value_type>);
                detail::text_basic_type_span_get(*this,name, a);
            }
            else if constexpr (basic_serializable_types<type>) {
                detail::text_basic_type_get(*this,name, a);
            } else {
                serializer<type>{}.text_put(*this, name, a);
            }
            return *this;
        }
        
    };

    class basic_binary_archive {
        std::vector<char> content_;
    public:
    };
    
}
