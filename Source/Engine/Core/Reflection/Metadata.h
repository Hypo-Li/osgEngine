#pragma once
#include <optional>
#include <variant>
#include <unordered_map>

template <typename T, typename Variant>
struct is_in_variant;

template <typename T, typename... Types>
struct is_in_variant<T, std::variant<Types...>> : std::disjunction<std::is_same<T, Types>...> {};

template <typename T, typename Variant>
constexpr bool is_in_variant_v = is_in_variant<T, Variant>::value;

namespace xxx::refl
{
    enum class MetaKey
    {
        Hidden,             /* Boolean */
        Category,           /* String */
        DisplayName,        /* String */
        ToolTip,            /* String */
        ClampMin,           /* Numeric */
        ClampMax,           /* Numeric */
    };

    class Reflection;
    class MetadataBase
    {
        friend class Reflection;
    public:
        using MetaValue = std::variant<bool, int64_t, uint64_t, float, double, std::string_view>;

        template <typename T, typename = std::enable_if_t<is_in_variant_v<T, MetaValue>>>
        std::optional<T> getMetadata(MetaKey key)
        {
            auto findResult = mMetadatas.find(key);
            if (findResult != mMetadatas.end())
            {
                T* result = std::get_if<T>(&findResult->second);
                if (result)
                    return std::optional<T>(*result);
            }
            return std::optional<T>{};
            
        }

        size_t getMetadataCount() const
        {
            return mMetadatas.size();
        }

    protected:
        template <typename T, typename = std::enable_if_t<std::is_arithmetic_v<T> || std::is_enum_v<T> || std::is_same_v<T, const char*>>>
        void addMetadata(MetaKey key, T value)
        {
            if constexpr (std::is_arithmetic_v<T>)
            {
                if constexpr (std::is_same_v<T, bool>)
                {
                    mMetadatas.emplace(key, value);
                }
                else if constexpr (std::is_integral_v<T>)
                {
                    if constexpr (std::is_signed_v<T>)
                        mMetadatas.emplace(key, static_cast<int64_t>(value));
                    else
                        mMetadatas.emplace(key, static_cast<uint64_t>(value));
                }
                else if constexpr (std::is_floating_point_v<T>)
                {
                    mMetadatas.emplace(key, value);
                }
            }
            else if constexpr (std::is_enum_v<T>)
            {
                mMetadatas.emplace(key, static_cast<int64_t>(value));
            }
            else if constexpr (std::is_same_v<T, const char*>)
            {
                mMetadatas.emplace(key, std::string_view(value));
            }
        }

        std::unordered_map<MetaKey, MetaValue> mMetadatas;
    };
}
