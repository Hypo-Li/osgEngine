#pragma once
#include "../Special.h"

namespace xxx::refl
{
    class StdVariant : public Special
    {
    public:
        virtual std::vector<Type*> getTypes() const = 0;
        virtual uint32_t getTypeIndex(const void* instance) const = 0;
        virtual void* getValuePtr(void* instance) const = 0;
        virtual const void* getValuePtr(const void* instance) const = 0;
        virtual void setValue(void* instance, uint32_t typeIndex, const void* value) const = 0;

    protected:
        StdVariant(std::string_view name, size_t size) : Special(name, size, Case::StdVariant) {}
    };

    template <typename T> requires is_instance_of_v<T, std::variant>
    class TStdVariant : public StdVariant
    {
        friend class Reflection;
        template <std::size_t... Is>
        static std::vector<Type*> getTypesImpl(std::index_sequence<Is...>)
        {
            return { Reflection::getType<std::variant_alternative_t<Is, T>>()... };
        }

        template <std::size_t I>
        static void setValueImpl(T* variant, uint32_t typeIndex, const void* value)
        {
            if (I == typeIndex)
            {
                variant->emplace<I>(*(std::variant_alternative_t<I, T>*)(value));
                return;
            }

            if constexpr (I + 1 < std::variant_size_v<T>)
                setValueImpl<I + 1>(variant, typeIndex, value);
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
            //uint32_t instance1TypeIndex = getTypeIndex(instance1);
            //if (instance1TypeIndex == getTypeIndex(instance2) && getTypes().at(instance1TypeIndex)->compare(getValuePtr(instance1), getValuePtr(instance2)))
            //    return true;
            return false;
        }

        virtual std::vector<Type*> getTypes() const override
        {
            return getTypesImpl(std::make_index_sequence<std::variant_size_v<T>>());
        }

        virtual uint32_t getTypeIndex(const void* instance) const override
        {
            return static_cast<const T*>(instance)->index();
        }

        virtual void* getValuePtr(void* instance) const override
        {
            return std::visit([](auto&& value) -> void* {
                return static_cast<void*>(&value);
                }, *static_cast<T*>(instance));
        }

        virtual const void* getValuePtr(const void* instance) const override
        {
            return std::visit([](auto&& value) -> const void* {
                return static_cast<const void*>(&value);
                }, *static_cast<const T*>(instance));
        }

        virtual void setValue(void* instance, uint32_t typeIndex, const void* value) const  override
        {
            setValueImpl<0>(static_cast<T*>(instance), typeIndex, value);
        }

    private:
        template <std::size_t... Is>
        static std::string getVariantTypeNames(std::index_sequence<Is...>)
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

        static std::string_view getName()
        {
            static std::string name = "std::variant<" + getVariantTypeNames(std::make_index_sequence<std::variant_size_v<T>>{}) + ">";
            return name;
        }

        TStdVariant() : StdVariant(getName(), sizeof(T)) {}
    };
}
