#pragma once
#include "Type.h"

namespace xxx::refl
{
    template <typename T>
    using remove_cvrefp_t = typename std::remove_cv_t<std::remove_reference_t<std::remove_pointer_t<T>>>;

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
            static Type* type = createType<_T>();
            return type;
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
        template <typename T>
        static Type * createType();

        using ClassMap = std::unordered_map<std::string_view, Class*>;
        static ClassMap& getClassMap()
        {
            static ClassMap classMap;
            return classMap;
        }
    };
}

#include "Reflection.inl"
