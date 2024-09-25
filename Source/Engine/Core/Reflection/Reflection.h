#pragma once
#include "Type.h"
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
#include "Special/StdVariant.h"
#include "Special/StdVector.h"

#include <osg/ref_ptr>

#include <unordered_map>

template <typename T>
using remove_cvrefp_t = typename std::remove_cv_t<std::remove_reference_t<std::remove_pointer_t<T>>>;

template <typename T>
static constexpr bool is_special_v =
std::is_same_v<T, std::string> ||
is_std_array_v<T> ||
is_instance_of_v<T, std::map> ||
is_instance_of_v<T, std::pair> ||
is_instance_of_v<T, std::set> ||
is_instance_of_v<T, std::tuple> ||
is_instance_of_v<T, std::variant> ||
is_instance_of_v<T, std::vector>;

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

namespace xxx::refl
{
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
        static Type* getType(T&& t)
        {
            using _T = remove_cvrefp_t<T>;
            return getOrCreateType<_T>();
        }

        template <typename T>
        static Struct* getStruct()
        {
            return dynamic_cast<Struct*>(getType<T>());
        }

        template <typename T>
        static Class* getClass()
        {
            return dynamic_cast<Class*>(getType<T>());
        }

        static Class* getClass(std::string_view name)
        {
            auto& classMap = getClassMap();
            auto findResult = classMap.find(name);
            if (findResult == classMap.end())
                return nullptr;
            return findResult->second;
        }

        template <typename T>
        static Special* getSpecial()
        {
            return dynamic_cast<Special*>(getType<T>());
        }

    public:
        static const refl::Fundamental* BoolType;
        static const refl::Fundamental* CharType;
        static const refl::Fundamental* WCharType;
        static const refl::Fundamental* Int8Type;
        static const refl::Fundamental* Int16Type;
        static const refl::Fundamental* Int32Type;
        static const refl::Fundamental* Int64Type;
        static const refl::Fundamental* Uint8Type;
        static const refl::Fundamental* Uint16Type;
        static const refl::Fundamental* Uint32Type;
        static const refl::Fundamental* Uint64Type;
        static const refl::Fundamental* FloatType;
        static const refl::Fundamental* DoubleType;

    private:
        template <typename T, std::enable_if_t<!(is_special_v<T> || is_instance_of_v<T, osg::ref_ptr>), int> = 0>
        static Type* createType()
        {
            static_assert(false, "T is a unreflectable type.");
        }

        template <typename T, std::enable_if_t<is_special_v<T> || is_instance_of_v<T, osg::ref_ptr>, int> = 0>
        static Type* createType()
        {
            if constexpr (std::is_same_v<T, std::string>)
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
            else if constexpr (is_instance_of_v<T, std::variant>)
                return new StdVariantInstance<T>;
            else if constexpr (is_instance_of_v<T, std::vector>)
                return new StdVectorInstance<T>;
            else if constexpr (is_instance_of_v<T, osg::ref_ptr>)
                return createType<remove_osg_ref_ptr_t<T>>();
        }

        template <typename T>
        static Type* getOrCreateType()
        {
            static Type* type = createType<T>();
            return type;
        }

        static std::unordered_map<std::string_view, Class*>& getClassMap()
        {
            static std::unordered_map<std::string_view, Class*> classMap;
            return classMap;
        }
	};

    template <typename T>
    static Type* Type::getType()
    {
        return Reflection::getType<T>();
    }

#define REFLECT_FUNDAMENTAL(type) \
    template <> \
    inline Type* Reflection::createType<type>() \
    { \
        return new FundamentalInstance<type>(#type); \
    }

    REFLECT_FUNDAMENTAL(void)
    REFLECT_FUNDAMENTAL(nullptr_t)
    REFLECT_FUNDAMENTAL(bool)
    REFLECT_FUNDAMENTAL(char)
    REFLECT_FUNDAMENTAL(wchar_t)
    REFLECT_FUNDAMENTAL(int8_t)
    REFLECT_FUNDAMENTAL(int16_t)
    REFLECT_FUNDAMENTAL(int32_t)
    REFLECT_FUNDAMENTAL(int64_t)
    REFLECT_FUNDAMENTAL(uint8_t)
    REFLECT_FUNDAMENTAL(uint16_t)
    REFLECT_FUNDAMENTAL(uint32_t)
    REFLECT_FUNDAMENTAL(uint64_t)
    REFLECT_FUNDAMENTAL(float)
    REFLECT_FUNDAMENTAL(double)
}
