#pragma once
#include "../Special.h"

#include <variant>

namespace xxx::refl
{
    class StdVariant : public Special
    {
    public:
        virtual SpecialType getSpecialType() const
        {
            return SpecialType::Std_Variant;
        }

        virtual std::vector<Type*> getTypes() const = 0;
        virtual uint32_t getTypeIndex(void* instance) const = 0;
        virtual void* getValuePtr(void* instance) const = 0;
        virtual void setValueByTypeIndex(void* instance, uint32_t typeIndex, void* value) const = 0;
    };

    class Reflection;
    template <typename T, std::enable_if_t<is_instance_of_v<T, std::variant>, int> = 0>
    class StdVariantInstance : public StdVariant
    {
        friend class Reflection;
        template <std::size_t... Is>
        static std::vector<Type*> getTypesImpl(std::index_sequence<Is...>)
        {
            return { Type::getType<std::variant_alternative_t<Is, T>>()... };
        }

        template <std::size_t I>
        static void* getValuePtrImpl(T* variant)
        {
            if (variant->index() == I)
                return std::get_if<I>(variant);

            if constexpr (I + 1 < std::variant_size_v<T>)
                return getValuePtrImpl<I + 1>(variant);
            return nullptr;
        }

        template <std::size_t I>
        static void setValueByTypeIndexImpl(T* variant, uint32_t typeIndex, void* value)
        {
            if (I == typeIndex)
                variant->emplace<I>(*(std::variant_alternative_t<I, T>*)(value));

            if constexpr (I + 1 < std::variant_size_v<T>)
                return setValueByTypeIndexImpl<I + 1>(variant, typeIndex, value);
        }
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
            return false;
        }

        virtual std::vector<Type*> getTypes() const override
        {
            return getTypesImpl(std::make_index_sequence<std::variant_size_v<T>>());
        }

        virtual uint32_t getTypeIndex(void* instance) const override
        {
            T* variant = static_cast<T*>(instance);
            return variant->index();
        }

        virtual void* getValuePtr(void* instance) const override
        {
            T* variant = static_cast<T*>(instance);
            return getValuePtrImpl<0>(variant);
        }

        virtual void setValueByTypeIndex(void* instance, uint32_t typeIndex, void* value) const override
        {
            T* variant = static_cast<T*>(instance);
        }

    protected:
        template <std::size_t... Is>
        static std::string createVariantArgsString(std::index_sequence<Is...>)
        {
            std::string result;
            std::vector<std::string_view> typeNames = { Reflection::getType<std::variant_alternative_t<Is, T>>()->getName()... };
            for (uint32_t i = 0; i < typeNames.size(); ++i)
            {
                result += typeNames[i];
                if (i != typeNames.size() - 1)
                    result += ", ";
            }
            return result;
        }

        StdVariantInstance()
        {
            static std::string name = "std::variant<" + createVariantArgsString(std::make_index_sequence<std::variant_size_v<T>>{}) + ">";
            mName = name;
            mSize = sizeof(T);
        }
    };
}
