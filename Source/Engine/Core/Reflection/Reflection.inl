#include "Special.h"
#include "Fundamental.h"
#include "Enumeration.h"
#include "Structure.h"
#include "Class.h"
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

namespace xxx::refl
{
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
        return new TFundamental<type>(#type); \
    }

    template <typename T>
    static Fundamental* Reflection::getFundamental()
    {
        return dynamic_cast<Fundamental*>(getType<T>());
    }

    template <typename T>
    static Enumeration* Reflection::getEnumeration()
    {
        return dynamic_cast<Enumeration*>(getType<T>());
    }

    template <typename T>
    static Structure* Reflection::getStructure()
    {
        return dynamic_cast<Structure*>(getType<T>());
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

        template <typename T>
    static Type* Reflection::createType()
    {
        if constexpr (is_instance_of_v<T, osg::ref_ptr>)
            return createType<remove_osg_ref_ptr_t<T>>();
        else if constexpr (std::is_same_v<T, std::string>)
            return new StdString;
        else if constexpr (is_std_array_v<T>)
            return new TStdArray<T>;
        else if constexpr (is_instance_of_v<T, std::map>)
            return new TStdMap<T>;
        else if constexpr (is_instance_of_v<T, std::pair>)
            return new TStdPair<T>;
        else if constexpr (is_instance_of_v<T, std::set>)
            return new TStdSet<T>;
        else if constexpr (is_instance_of_v<T, std::tuple>)
            return new TStdTuple<T>;
        else if constexpr (is_instance_of_v<T, std::unordered_map>)
            return new TStdUnorderedMap<T>;
        else if constexpr (is_instance_of_v<T, std::unordered_set>)
            return new TStdUnorderedSet<T>;
        else if constexpr (is_instance_of_v<T, std::variant>)
            return new TStdVariant<T>;
        else if constexpr (is_instance_of_v<T, std::vector>)
            return new TStdVector<T>;
        else
            static_assert(false, "T is a unreflectable type.");
    }
}
