#pragma once
#include "../Special.h"

#include <unordered_map>

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
        virtual SpecialType getSpecialType() const
        {
            return SpecialType::Std_Unordered_Map;
        }

        virtual Type* getKeyType() const = 0;
        virtual Type* getValueType() const = 0;
        virtual size_t getKeyValuePairCount(const void* instance) const = 0;
        virtual void* getValuePtrByKey(void* instance, void* key) const = 0;
        virtual std::vector<std::pair<const void*, void*>> getKeyValuePtrs(void* instance) const = 0;

        virtual void insertKeyValuePair(void* instance, void* key, void* value) const = 0;
        virtual void removeKeyValuePairByKey(void* instance, void* key) const = 0;

    protected:
        StdUnorderedMap(std::string_view name, size_t size) : Special(name, size) {}
    };

    class Reflection;
    template <typename T, typename = std::enable_if_t<is_instance_of_v<T, std::unordered_map>>>
    class StdUnorderedMapInstance : public StdUnorderedMap
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
            if (static_cast<const std::unordered_map<Key, Value>*>(instance1)->size() == 0 && static_cast<const std::unordered_map<Key, Value>*>(instance2)->size() == 0)
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

        virtual void* getValuePtrByKey(void* instance, void* key) const override
        {
            std::unordered_map<Key, Value>* unorderedMap = static_cast<std::unordered_map<Key, Value>*>(instance);
            auto findResult = unorderedMap->find(*(Key*)(key));
            if (findResult != unorderedMap->end())
                return &findResult->second;
            return nullptr;
        }

        virtual std::vector<std::pair<const void*, void*>> getKeyValuePtrs(void* instance) const override
        {
            std::vector<std::pair<const void*, void*>> result;
            std::unordered_map<Key, Value>* unorderedMap = static_cast<std::unordered_map<Key, Value>*>(instance);
            for (auto it = unorderedMap->begin(); it != unorderedMap->end(); ++it)
            {
                result.emplace_back(&it->first, &it->second);
            }
            return result;
        }

        virtual void insertKeyValuePair(void* instance, void* key, void* value) const override
        {
            std::unordered_map<Key, Value>* unorderedMap = static_cast<std::unordered_map<Key, Value>*>(instance);
            unorderedMap->emplace(*(Key*)(key), *(Value*)(value));
        }

        virtual void removeKeyValuePairByKey(void* instance, void* key) const override
        {
            std::unordered_map<Key, Value>* unorderedMap = static_cast<std::unordered_map<Key, Value>*>(instance);
            auto findResult = unorderedMap->find(*(Key*)(key));
            if (findResult != unorderedMap->end())
                unorderedMap->erase(findResult);
        }

    protected:
        std::string_view genName() const
        {
            static std::string name = "std::unordered_map<" + std::string(Reflection::getType<Key>()->getName()) + ", " + std::string(Reflection::getType<Value>()->getName()) + ">";
            return name;
        }

        StdUnorderedMapInstance() : StdUnorderedMap(genName(), sizeof(std::unordered_map<Key, Value>)) {}
    };
}
