#pragma once
#include "Type.h"

#include <osg/ref_ptr>

#include <string>
#include <array>
#include <map>
#include <set>
#include <unordered_map>
#include <unordered_set>
#include <tuple>
#include <variant>
#include <vector>

template <typename T>
using remove_cvrefp_t = typename std::remove_cv_t<std::remove_reference_t<std::remove_pointer_t<T>>>;

template <typename T>
static constexpr bool is_special_v = std::is_same_v<T, std::string> || is_template_instance<T>;

namespace xxx::refl
{
    class Fundamental;
    class Enum;
    class Struct;
    class Class;
    class Special;
    class Reflection
    {
    public:
        template <typename T>
        static Type* getType()
        {
            using _T = remove_cvrefp_t<T>;
            return getOrCreateType<_T>();
        }

        template <typename T>
        static Fundamental* getFundamental();

        template <typename T>
        static Enum* getEnum();

        template <typename T>
        static Struct* getStruct();

        template <typename T>
        static Class* getClass();

        template <typename T>
        static Special* getSpecial();

        static Class* getClass(std::string_view name)
        {
            ClassMap& classMap = getClassMap();
            auto findResult = classMap.find(name);
            if (findResult == classMap.end())
                return nullptr;
            return findResult->second;
        }

        static void registerClass(std::string_view name, Class* clazz)
        {
            ClassMap& classMap = getClassMap();
            classMap.emplace(name, clazz);
        }

        inline static const Fundamental* BoolType = getFundamental<bool>();
        inline static const Fundamental* CharType = getFundamental<char>();
        inline static const Fundamental* WCharType = getFundamental<wchar_t>();
        inline static const Fundamental* Int8Type = getFundamental<int8_t>();
        inline static const Fundamental* Int16Type = getFundamental<int16_t>();
        inline static const Fundamental* Int32Type = getFundamental<int32_t>();
        inline static const Fundamental* Int64Type = getFundamental<int64_t>();
        inline static const Fundamental* Uint8Type = getFundamental<uint8_t>();
        inline static const Fundamental* Uint16Type = getFundamental<uint16_t>();
        inline static const Fundamental* Uint32Type = getFundamental<uint32_t>();
        inline static const Fundamental* Uint64Type = getFundamental<uint64_t>();
        inline static const Fundamental* FloatType = getFundamental<float>();
        inline static const Fundamental* DoubleType = getFundamental<double>();

    private:
        template <typename T, std::enable_if_t<!(is_instance_of_v<T, osg::ref_ptr> || is_special_v<T>), int> = 0>
        static Type* createType()
        {
            static_assert(false, "T is a unreflectable type.");
        }

        template <typename T, std::enable_if_t<is_instance_of_v<T, osg::ref_ptr> || is_special_v<T>, int> = 0>
        static Type * createType();

        template <typename T>
        static Type* getOrCreateType()
        {
            static Type* type = createType<T>();
            return type;
        }

        using ClassMap = std::unordered_map<std::string_view, Class*>;
        static ClassMap& getClassMap()
        {
            static ClassMap classMap;
            return classMap;
        }
    };
}

#include "Fundamental.h"
#include "Enum.h"
#include "Struct.h"
#include "Class.h"
#include "Special.h"
#include "Special/StdArray.h"
#include "Special/StdMap.h"
#include "Special/StdPair.h"
#include "Special/StdSet.h"
#include "Special/StdString.h"
#include "Special/StdTuple.h"
#include "Special/StdUnorderedMap.h"
#include "Special/StdUnorderedSet.h"
#include "Special/StdVariant.h"
#include "Special/StdVector.h"

template <typename T>
struct remove_osg_ref_ptr {
    using type = T;
};

template <typename T>
struct remove_osg_ref_ptr<osg::ref_ptr<T>> {
    using type = T;
};

template <typename T>
using remove_osg_ref_ptr_t = typename remove_osg_ref_ptr<T>::type;

#define CREATE_FUNDAMENTAL_TYPE(type) \
    template <> \
    inline static Type* Reflection::createType<type>() \
    { \
        return new FundamentalInstance<type>(#type); \
    }

namespace xxx::refl
{
    template <typename T>
    static Fundamental* Reflection::getFundamental()
    {
        return dynamic_cast<Fundamental*>(getType<T>());
    }

    template <typename T>
    static Enum* Reflection::getEnum()
    {
        return dynamic_cast<Enum*>(getType<T>());
    }

    template <typename T>
    static Struct* Reflection::getStruct()
    {
        return dynamic_cast<Struct*>(getType<T>());
    }

    template <typename T>
    static Class* Reflection::getClass()
    {
        return dynamic_cast<Class*>(getType<T>());
    }

    template <typename T>
    static Special* Reflection::getSpecial()
    {
        return dynamic_cast<Special*>(getType<T>());
    }

    CREATE_FUNDAMENTAL_TYPE(void)
    CREATE_FUNDAMENTAL_TYPE(nullptr_t)
    CREATE_FUNDAMENTAL_TYPE(bool)
    CREATE_FUNDAMENTAL_TYPE(char)
    CREATE_FUNDAMENTAL_TYPE(wchar_t)
    CREATE_FUNDAMENTAL_TYPE(int8_t)
    CREATE_FUNDAMENTAL_TYPE(int16_t)
    CREATE_FUNDAMENTAL_TYPE(int32_t)
    CREATE_FUNDAMENTAL_TYPE(int64_t)
    CREATE_FUNDAMENTAL_TYPE(uint8_t)
    CREATE_FUNDAMENTAL_TYPE(uint16_t)
    CREATE_FUNDAMENTAL_TYPE(uint32_t)
    CREATE_FUNDAMENTAL_TYPE(uint64_t)
    CREATE_FUNDAMENTAL_TYPE(float)
    CREATE_FUNDAMENTAL_TYPE(double)

    template <typename T, std::enable_if_t<is_instance_of_v<T, osg::ref_ptr> || is_special_v<T>, int>>
    static Type* Reflection::createType()
    {
        if constexpr (is_instance_of_v<T, osg::ref_ptr>)
            return createType<remove_osg_ref_ptr_t<T>>();
        else if constexpr (std::is_same_v<T, std::string>)
            return new StdString;
        else if constexpr (is_std_array_v<T>)
            return new StdArrayInstance<T>;
        else if constexpr (is_instance_of_v<T, std::map>)
            return new StdMapInstance<T>;
        else if constexpr (is_instance_of_v<T, std::pair>)
            return new StdPairInstance<T>;
        else if constexpr (is_instance_of_v<T, std::set>)
            return new StdSetInstance<T>;
        else if constexpr (is_instance_of_v<T, std::tuple>)
            return new StdTupleInstance<T>;
        else if constexpr (is_instance_of_v<T, std::unordered_map>)
            return new StdUnorderedMapInstance<T>;
        else if constexpr (is_instance_of_v<T, std::unordered_set>)
            return new StdUnorderedSetInstance<T>;
        else if constexpr (is_instance_of_v<T, std::variant>)
            return new StdVariantInstance<T>;
        else if constexpr (is_instance_of_v<T, std::vector>)
            return new StdVectorInstance<T>;
    }
}
