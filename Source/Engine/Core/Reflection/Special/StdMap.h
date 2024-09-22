#pragma once
#include "../Special.h"

#include <map>

template <typename T1, typename T2>
struct container_traits<std::map<T1, T2>> {
    using type1 = T1;
    using type2 = T2;
};

namespace xxx::refl
{
    class StdMap : public Special
    {
    public:
        virtual SpecialType getSpecialType() const
        {
            return SpecialType::Std_Map;
        }

        virtual Type* getKeyType() const = 0;
        virtual Type* getValueType() const = 0;
        virtual size_t getKeyValuePairCount(void* instance) const = 0;
        virtual void* getValuePtrByKey(void* instance, void* key) const = 0;
        virtual std::vector<std::pair<const void*, void*>> getKeyValuePtrs(void* instance) const = 0;

        virtual void insertKeyValuePair(void* instance, void* key, void* value) const = 0;
        virtual void removeKeyValuePairByKey(void* instance, void* key) const = 0;
    };

    class Reflection;
    template <typename T, std::enable_if_t<is_instance_of_v<T, std::map>, int> = 0>
    class StdMapInstance : public StdMap
    {
        friend class Reflection;
        using Key = container_traits_t1<T>;
        using Value = container_traits_t2<T>;
    public:
        virtual void* newInstance() const override
        {
            return new std::map<Key, Value>;
        }

        virtual void deleteInstance(void* instance) const override
        {
            delete static_cast<std::map<Key, Value>*>(instance);
        }

        virtual void* newInstances(size_t count) const override
        {
            return new std::map<Key, Value>[count];
        }

        virtual void deleteInstances(void* instances) const override
        {
            delete[] static_cast<std::map<Key, Value>*>(instances);
        }

        virtual bool compare(const void* instance1, const void* instance2) const override
        {
            return false;
        }

        virtual Type* getKeyType() const override
        {
            return Type::getType<Key>();
        }

        virtual Type* getValueType() const override
        {
            return Type::getType<Value>();
        }

        virtual size_t getKeyValuePairCount(void* instance) const override
        {
            return static_cast<std::map<Key, Value>*>(instance)->size();
        }

        virtual void* getValuePtrByKey(void* instance, void* key) const override
        {
            std::map<Key, Value>* map = static_cast<std::map<Key, Value>*>(instance);
            auto findResult = map->find(*(Key*)(key));
            if (findResult != map->end())
                return &findResult->second;
            return nullptr;
        }

        virtual std::vector<std::pair<const void*, void*>> getKeyValuePtrs(void* instance) const override
        {
            std::vector<std::pair<const void*, void*>> result;
            std::map<Key, Value>* map = static_cast<std::map<Key, Value>*>(instance);
            for (auto it = map->begin(); it != map->end(); ++it)
            {
                result.emplace_back(&it->first, &it->second);
            }
            return result;
        }

        virtual void insertKeyValuePair(void* instance, void* key, void* value) const override
        {
            std::map<Key, Value>* map = static_cast<std::map<Key, Value>*>(instance);
            map->emplace(*(Key*)(key), *(Value*)(value));
        }

        virtual void removeKeyValuePairByKey(void* instance, void* key) const override
        {
            std::map<Key, Value>* map = static_cast<std::map<Key, Value>*>(instance);
            auto findResult = map->find(*(Key*)(key));
            if (findResult != map->end())
                map->erase(findResult);
        }

    protected:
        StdMapInstance()
        {
            static std::string name = "std::map<" + std::string(Reflection::getType<Key>()->getName()) + ", " + std::string(Reflection::getType<Value>()->getName()) + ">";
            mName = name;
            mSize = sizeof(std::map<Key, Value>);
        }
    };
}
