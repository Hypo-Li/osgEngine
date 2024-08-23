#pragma once
#include "Type.h"

#include <vector>

namespace xxx::refl
{
    class Reflection;
    class Enum : public Type
    {
        friend class Reflection;
    public:
        virtual Kind getKind() const override { return Kind::Enum; }

        static constexpr uint32_t INDEX_NONE = std::numeric_limits<uint32_t>::max();
        static constexpr std::string_view NAME_NONE = std::string_view("");

        Type* getUnderlyingType() const
        {
            return mUnderlyingType;
        }

        uint32_t getIndexByName(std::string_view name) const
        {
            uint32_t count = mValues.size();
            for (uint32_t i = 0; i < count; ++i)
            {
                if (mValues[i].first == name)
                    return i;
            }
            return INDEX_NONE;
        }

        uint32_t getIndexByValue(int64_t value) const
        {
            uint32_t count = mValues.size();
            for (uint32_t i = 0; i < count; ++i)
            {
                if (mValues[i].second == value)
                    return i;
            }
            return INDEX_NONE;
        }

        std::string_view getNameByIndex(uint32_t index) const
        {
            if (index < mValues.size())
            {
                return mValues[index].first;
            }
            return NAME_NONE;
        }

        int64_t getValueByIndex(uint32_t index) const
        {
            if (index < mValues.size())
            {
                return mValues[index].second;
            }
            return INDEX_NONE;
        }

        std::string_view getNameByValue(int64_t value) const
        {
            uint32_t index = getIndexByValue(value);
            if (index != INDEX_NONE)
            {
                return mValues[index].first;
            }
            return NAME_NONE;
        }

        int64_t getValueByName(std::string_view name) const
        {
            uint32_t index = getIndexByName(name);
            if (index != INDEX_NONE)
            {
                return mValues[index].second;
            }
            return INDEX_NONE;
        }

    private:
        template <typename E, std::enable_if_t<std::is_enum_v<E>, int> = 0>
        Enum(std::string_view name, std::initializer_list<std::pair<std::string_view, E>> values) :
            Type(name, sizeof(E)),
            mUnderlyingType(Type::getType<std::underlying_type_t<E>>())
        {
            for (std::pair<std::string_view, E> value : values)
                mValues.emplace_back(value.first, static_cast<int64_t>(value.second));
        }
        virtual ~Enum() = default;

        std::vector<std::pair<std::string_view, int64_t>> mValues;
        Type* mUnderlyingType;
    };
}
