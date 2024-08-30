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

        virtual void* newInstance() const override
        {
            return malloc(mSize);
        }

        virtual void deleteInstance(void* instance) const override
        {
            free(instance);
        }

        static constexpr size_t INDEX_NONE = std::numeric_limits<size_t>::max();
        static constexpr std::string_view NAME_NONE = std::string_view("");

        Type* getUnderlyingType() const
        {
            return mUnderlyingType;
        }

        size_t getIndexByName(std::string_view name) const
        {
            size_t count = mValues.size();
            for (size_t i = 0; i < count; ++i)
            {
                if (mValues[i].first == name)
                    return i;
            }
            return INDEX_NONE;
        }

        size_t getIndexByValue(int64_t value) const
        {
            size_t count = mValues.size();
            for (size_t i = 0; i < count; ++i)
            {
                if (mValues[i].second == value)
                    return i;
            }
            return INDEX_NONE;
        }

        std::string_view getNameByIndex(size_t index) const
        {
            if (index < mValues.size())
            {
                return mValues[index].first;
            }
            return NAME_NONE;
        }

        int64_t getValueByIndex(size_t index) const
        {
            if (index < mValues.size())
            {
                return mValues[index].second;
            }
            return INDEX_NONE;
        }

        std::string_view getNameByValue(int64_t value) const
        {
            size_t index = getIndexByValue(value);
            if (index != INDEX_NONE)
            {
                return mValues[index].first;
            }
            return NAME_NONE;
        }

        int64_t getValueByName(std::string_view name) const
        {
            size_t index = getIndexByName(name);
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
