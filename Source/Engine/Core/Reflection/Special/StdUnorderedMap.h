#pragma once
#include "../Special.h"

namespace xxx::refl
{
    template <typename T1, typename T2>
    struct container_traits<std::unordered_map<T1, T2>> {
        using type1 = T1;
        using type2 = T2;
    };

    class StdUnorderedMap : public Special
    {
    public:
        virtual Type* getKeyType() const = 0;
        virtual Type* getValueType() const = 0;
        virtual size_t getKeyValuePairCount(const void* instance) const = 0;
        virtual void* getValuePtr(void* instance, const void* key) const = 0;
        virtual const void* getValuePtr(const void* instance, const void* key) const = 0;
        virtual std::vector<const void*> getKeyPtrs(const void* instance) const = 0;
        virtual std::vector<void*> getValuePtrs(void* instance) const = 0;
        virtual std::vector<const void*> getValuePtrs(const void* instance) const = 0;
        virtual void insertKeyValuePair(void* instance, const void* key, const void* value) const = 0;

    protected:
        StdUnorderedMap(std::string_view name, size_t size) : Special(name, size, Case::StdUnorderedMap) {}
    };

    class Reflection;
    template <typename T> requires is_instance_of_v<T, std::unordered_map>
    class TStdUnorderedMap : public StdUnorderedMap
    {
        friend class Reflection;
        using Key = container_traits_t1<T>;
        using Value = container_traits_t2<T>;
    public:
        virtual void* newInstance() const override
        {
            return new std::unordered_map<Key, Value>;
        }

        virtual void deleteInstance(void* instance) const override
        {
            delete static_cast<std::unordered_map<Key, Value>*>(instance);
        }

        virtual void* newInstances(size_t count) const override
        {
            return new std::unordered_map<Key, Value>[count];
        }

        virtual void deleteInstances(void* instances) const override
        {
            delete[] static_cast<std::unordered_map<Key, Value>*>(instances);
        }

        virtual bool compare(const void* instance1, const void* instance2) const override
        {
            if (static_cast<const std::unordered_map<Key, Value>*>(instance1)->size() == 0 &&
                static_cast<const std::unordered_map<Key, Value>*>(instance2)->size() == 0)
                return true;
            return false;
        }

        virtual Type* getKeyType() const override
        {
            return Reflection::getType<Key>();
        }

        virtual Type* getValueType() const override
        {
            return Reflection::getType<Value>();
        }

        virtual size_t getKeyValuePairCount(const void* instance) const override
        {
            return static_cast<const std::unordered_map<Key, Value>*>(instance)->size();
        }

        virtual void* getValuePtr(void* instance, const void* key) const override
        {
            std::unordered_map<Key, Value>* map = static_cast<std::unordered_map<Key, Value>*>(instance);
            auto findResult = map->find(*static_cast<const Key*>(key));
            if (findResult != map->end())
                return &findResult->second;
            return nullptr;
        }

        virtual const void* getValuePtr(const void* instance, const void* key) const override
        {
            const std::unordered_map<Key, Value>* map = static_cast<const std::unordered_map<Key, Value>*>(instance);
            const auto findResult = map->find(*static_cast<const Key*>(key));
            if (findResult != map->end())
                return &findResult->second;
            return nullptr;
        }

        virtual std::vector<const void*> getKeyPtrs(const void* instance) const override
        {
            const std::unordered_map<Key, Value>* map = static_cast<const std::unordered_map<Key, Value>*>(instance);
            std::vector<const void*> result(map->size());
            size_t i = 0;
            for (const auto& pair : *map)
                result[i++] = &pair.first;
            return result;
        }

        virtual std::vector<void*> getValuePtrs(void* instance) const override
        {
            std::unordered_map<Key, Value>* map = static_cast<std::unordered_map<Key, Value>*>(instance);
            std::vector<void*> result(map->size());
            size_t i = 0;
            for (auto& pair : *map)
                result[i++] = &pair.second;
            return result;
        }

        virtual std::vector<const void*> getValuePtrs(const void* instance) const override
        {
            const std::unordered_map<Key, Value>* map = static_cast<const std::unordered_map<Key, Value>*>(instance);
            std::vector<const void*> result(map->size());
            size_t i = 0;
            for (const auto& pair : *map)
                result[i++] = &pair.second;
            return result;
        }

        virtual void insertKeyValuePair(void* instance, const void* key, const void* value) const override
        {
            std::unordered_map<Key, Value>* map = static_cast<std::unordered_map<Key, Value>*>(instance);
            map->emplace(*static_cast<const Key*>(key), *static_cast<const Value*>(value));
        }

    protected:
        std::string_view getName() const
        {
            static std::string name = "std::unordered_map<" + std::string(Reflection::getType<Key>()->getName()) + ", " + std::string(Reflection::getType<Value>()->getName()) + ">";
            return name;
        }

        TStdUnorderedMap() : StdUnorderedMap(getName(), sizeof(std::unordered_map<Key, Value>)) {}
    };
}
