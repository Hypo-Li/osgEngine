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

    protected:
        Enum(std::string_view name, size_t size) :
            Type(name, size)
        {}

        std::vector<std::pair<std::string_view, int64_t>> mValues;
        Fundamental* mUnderlyingType;
    };

    template <typename T, typename = std::enable_if_t<std::is_enum_v<T>>>
    class EnumInstance : public Enum
    {
        friend class Reflection;
    public:
        virtual void* newInstance() const override
        {
            return new T;
        }

        virtual void deleteInstance(void* instance) const override
        {
            delete static_cast<T*>(instance);
        }

        virtual void* newInstances(size_t count) const override
        {
            return new T[count];
        }

        virtual void deleteInstances(void* instances) const override
        {
            delete[] static_cast<T*>(instances);
        }

        virtual bool compare(const void* instance1, const void* instance2) const override
        {
            return *static_cast<const T*>(instance1) == *static_cast<const T*>(instance2);
        }

    protected:
        EnumInstance(std::string_view name, std::initializer_list<std::pair<std::string_view, T>> values) :
            Enum(name, sizeof(T))
        {
            mUnderlyingType = dynamic_cast<Fundamental*>(Type::getType<std::underlying_type_t<T>>());
            for (std::pair<std::string_view, T> value : values)
                mValues.emplace_back(value.first, static_cast<int64_t>(value.second));
        }
    };
}
