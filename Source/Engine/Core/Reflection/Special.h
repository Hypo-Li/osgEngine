#pragma once
#include "Type.h"

#include <string>
#include <array>
#include <vector>
#include <map>
#include <set>
#include <tuple>
#include <variant>

namespace xxx::refl
{
    template <class>
    inline constexpr bool is_std_array_v = std::false_type{};

    template <class T, size_t Size>
    inline constexpr bool is_std_array_v<std::array<T, Size>> = std::true_type{};

    template <class T, template<class...> class U>
    inline constexpr bool is_instance_of_v = std::false_type{};

    template <template <class...> class U, class... Vs>
    inline constexpr bool is_instance_of_v<U<Vs...>, U> = std::true_type{};

    // container Traits
    template <typename T>
    struct container_traits;

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

    template <typename T>
    using container_traits_t = typename container_traits<T>::type;
    template <typename T>
    using container_traits_t1 = typename container_traits<T>::type1;
    template <typename T>
    using container_traits_t2 = typename container_traits<T>::type2;

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
