#pragma once
#include "Type.h"

#include <string>
#include <array>
#include <vector>
#include <map>
#include <set>
#include <tuple>
#include <variant>

template <typename T>
struct container_traits<std::vector<T>> {
    using type = T;
};

template <typename T>
struct container_traits<std::set<T>> {
    using type = T;
};

template <typename T1, typename T2>
struct container_traits<std::map<T1, T2>> {
    using type1 = T1;
    using type2 = T2;
};

template <typename T1, typename T2>
struct container_traits<std::pair<T1, T2>> {
    using type1 = T1;
    using type2 = T2;
};

template <typename T, size_t N>
struct container_traits<std::array<T, N>> {
    using type = T;
    static constexpr size_t Count = N;
};

namespace xxx::refl
{
    enum class SpecialType
    {
        Std_String,
        Std_Array,
        Std_Map,
        Std_Pair,
        Std_Set,
        Std_Tuple,
        Std_Variant,
        Std_Vector,
    };

    class Special : public Type
    {
    public:
        virtual Kind getKind() const override { return Kind::Special; }
        virtual SpecialType getSpecialType() const = 0;

    protected:
        Special() = default;
        virtual ~Special() = default;
    };
}
