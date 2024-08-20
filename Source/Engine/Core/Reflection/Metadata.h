#pragma once
#include "Any.h"

#include <optional>
#include <variant>
#include <unordered_map>

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

        template <typename T>
        static constexpr bool is_metavalue_v =
            std::is_same_v<T, bool> ||
            std::is_same_v<T, int64_t> ||
            std::is_same_v<T, uint64_t> ||
            std::is_same_v<T, float> ||
            std::is_same_v<T, double> ||
            std::is_same_v<T, std::string_view>;
    public:
        template <typename T, std::enable_if_t<is_metavalue_v<T>, int> = 0>
        std::optional<T> getMetadata(MetaKey key)
        {
            auto findResult = mMetadatas.find(key);
            if (findResult != mMetadatas.end())
            {
                try
                {
                    T result = std::get<T>(findResult->second);
                    return result;
                }
                catch (std::bad_variant_access const& ex)
                {
                    return std::optional<T>{};
                }
            }
            return std::optional<T>{};
        }

    protected:
        template <typename T, std::enable_if_t<std::is_arithmetic_v<T> || std::is_enum_v<T> || std::is_same_v<T, const char*>, int> = 0>
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

        using MetaValue = std::variant<bool, int64_t, uint64_t, float, double, std::string_view>;
        std::unordered_map<MetaKey, MetaValue> mMetadatas;
    };
}