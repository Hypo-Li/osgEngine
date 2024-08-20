#pragma once
#include "../Special.h"
#include "../Argument.h"

#include <map>

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
        virtual void* getValuePtrByKey(void* instance, Argument key) const = 0;
        virtual std::vector<std::pair<const void*, void*>> getKeyValuePtrs(void* instance) const = 0;

        virtual void insertKeyValuePair(void* instance, Argument key, Argument value) const = 0;
        virtual void removeKeyValuePairByKey(void* instance, Argument key) const = 0;
    };

    class Reflection;
    template <typename T, std::enable_if_t<is_instance_of_v<T, std::map>, int> = 0>
    class StdMapInstance : public StdMap
    {
        friend class Reflection;
        using KeyType = container_traits_t1<T>;
        using ValueType = container_traits_t2<T>;
    public:
        virtual Type* getKeyType() const override
        {
            return Type::getType<KeyType>();
        }
        virtual Type* getValueType() const override
        {
            return Type::getType<ValueType>();
        }
        virtual size_t getKeyValuePairCount(void* instance) const override
        {
            return static_cast<std::map<KeyType, ValueType>*>(instance)->size();
        }
        virtual void* getValuePtrByKey(void* instance, Argument key) const override
        {
            std::map<KeyType, ValueType>* map = static_cast<std::map<KeyType, ValueType>*>(instance);
            auto findResult = map->find(key.getValue<KeyType>());
            if (findResult != map->end())
                return &findResult->second;
            return nullptr;
        }
        virtual std::vector<std::pair<const void*, void*>> getKeyValuePtrs(void* instance) const override
        {
            std::vector<std::pair<const void*, void*>> result;
            std::map<KeyType, ValueType>* map = static_cast<std::map<KeyType, ValueType>*>(instance);
            for (auto it = map->begin(); it != map->end(); ++it)
            {
                result.emplace_back(&it->first, &it->second);
            }
            return result;
        }

        virtual void insertKeyValuePair(void* instance, Argument key, Argument value) const override
        {
            std::map<KeyType, ValueType>* map = static_cast<std::map<KeyType, ValueType>*>(instance);
            map->emplace(key.getValue<KeyType>(), value.getValue<ValueType>());
        }
        virtual void removeKeyValuePairByKey(void* instance, Argument key) const override
        {
            std::map<KeyType, ValueType>* map = static_cast<std::map<KeyType, ValueType>*>(instance);
            auto findResult = map->find(key.getValue<KeyType>());
            if (findResult != map->end())
                map->erase(findResult);
        }

    protected:
        StdMapInstance()
        {
            static std::string name = "std::map<" + std::string(Reflection::getType<KeyType>()->getName()) + ", " + std::string(Reflection::getType<ValueType>()->getName()) + ">";
            mName = name; // typeid(std::map<KeyType, ValueType>).name();
            mSize = sizeof(std::map<KeyType, ValueType>);
        }
    };
}
