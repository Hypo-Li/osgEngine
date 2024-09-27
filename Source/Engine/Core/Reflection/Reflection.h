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
#include "Special/StdUnorderedMap.h"
#include "Special/StdUnorderedSet.h"
#include "Special/StdVariant.h"
#include "Special/StdVector.h"

#include <osg/ref_ptr>

#include <unordered_map>

template <typename T>
using remove_cvrefp_t = typename std::remove_cv_t<std::remove_reference_t<std::remove_pointer_t<T>>>;

template <typename T>
static constexpr bool is_special_v = std::is_same_v<T, std::string> || is_template_instance<T>;

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
    static Type* createType<type>() \
    { \
        return new FundamentalInstance<type>(#type); \
    }

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

    private:
        template <typename T, std::enable_if_t<!(is_special_v<T> || is_instance_of_v<T, osg::ref_ptr>), int> = 0>
        static Type* createType()
        {
            static_assert(false, "T is a unreflectable type.");
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

        template <typename T, std::enable_if_t<is_instance_of_v<T, osg::ref_ptr>, int> = 0>
        static Type* createType()
        {
            return createType<remove_osg_ref_ptr_t<T>>();
        }

        template <typename T, std::enable_if_t<std::is_same_v<T, std::string>, int> = 0>
         static Type* createType()
        {
            return new StdString;
        }

        template <typename T, std::enable_if_t<is_std_array_v<T>, int> = 0>
        static Type* createType()
        {
            return new StdArrayInstance<T>;
        }

        template <typename T, std::enable_if_t<is_instance_of_v<T, std::map>, int> = 0>
        static Type* createType()
        {
            return new StdMapInstance<T>;
        }

        template <typename T, std::enable_if_t<is_instance_of_v<T, std::pair>, int> = 0>
        static Type* createType()
        {
            return new StdPairInstance<T>;
        }

        template <typename T, std::enable_if_t<is_instance_of_v<T, std::set>, int> = 0>
        static Type* createType()
        {
            return new StdSetInstance<T>;
        }

        template <typename T, std::enable_if_t<is_instance_of_v<T, std::tuple>, int> = 0>
        static Type* createType()
        {
            return new StdTupleInstance<T>;
        }

        template <typename T, std::enable_if_t<is_instance_of_v<T, std::unordered_map>, int> = 0>
        static Type* createType()
        {
            return new StdUnorderedMapInstance<T>;
        }

        template <typename T, std::enable_if_t<is_instance_of_v<T, std::unordered_set>, int> = 0>
        static Type* createType()
        {
            return new StdUnorderedSetInstance<T>;
        }

        template <typename T, std::enable_if_t<is_instance_of_v<T, std::variant>, int> = 0>
        static Type* createType()
        {
            return new StdVariantInstance<T>;
        }

        template <typename T, std::enable_if_t<is_instance_of_v<T, std::vector>, int> = 0>
        static Type* createType()
        {
            return new StdVectorInstance<T>;
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

    public:
        inline static const Fundamental* BoolType = dynamic_cast<Fundamental*>(getType<bool>());
        inline static const Fundamental* CharType = dynamic_cast<Fundamental*>(getType<char>());
        inline static const Fundamental* WCharType = dynamic_cast<Fundamental*>(getType<wchar_t>());
        inline static const Fundamental* Int8Type = dynamic_cast<Fundamental*>(getType<int8_t>());
        inline static const Fundamental* Int16Type = dynamic_cast<Fundamental*>(getType<int16_t>());
        inline static const Fundamental* Int32Type = dynamic_cast<Fundamental*>(getType<int32_t>());
        inline static const Fundamental* Int64Type = dynamic_cast<Fundamental*>(getType<int64_t>());
        inline static const Fundamental* Uint8Type = dynamic_cast<Fundamental*>(getType<uint8_t>());
        inline static const Fundamental* Uint16Type = dynamic_cast<Fundamental*>(getType<uint16_t>());
        inline static const Fundamental* Uint32Type = dynamic_cast<Fundamental*>(getType<uint32_t>());
        inline static const Fundamental* Uint64Type = dynamic_cast<Fundamental*>(getType<uint64_t>());
        inline static const Fundamental* FloatType = dynamic_cast<Fundamental*>(getType<float>());
        inline static const Fundamental* DoubleType = dynamic_cast<Fundamental*>(getType<double>());
	};

    template <typename T>
    static Type* Type::getType()
    {
        return Reflection::getType<T>();
    }

}
