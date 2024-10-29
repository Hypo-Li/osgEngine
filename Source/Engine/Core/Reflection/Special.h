#pragma once
#include "Reflection.h"

namespace xxx::refl
{
    template <class>
    constexpr bool is_std_array_v = std::false_type{};

    template <class T, size_t Size>
    constexpr bool is_std_array_v<std::array<T, Size>> = std::true_type{};

    template <class T, template<class...> class U>
    constexpr bool is_instance_of_v = std::false_type{};

    template <template <class...> class U, class... Vs>
    constexpr bool is_instance_of_v<U<Vs...>, U> = std::true_type{};

    template <typename T>
    constexpr bool is_special_v =
        std::is_same_v<T, std::string> ||
        is_std_array_v<T> ||
        is_instance_of_v<T, std::map> ||
        is_instance_of_v<T, std::pair> ||
        is_instance_of_v<T, std::set> ||
        is_instance_of_v<T, std::tuple> ||
        is_instance_of_v<T, std::unordered_map> ||
        is_instance_of_v<T, std::unordered_set> ||
        is_instance_of_v<T, std::variant> ||
        is_instance_of_v<T, std::vector>;

    // container Traits
    template <typename T>
    struct container_traits;

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
        Std_Unordered_Map,
        Std_Unordered_Set,
        Std_Variant,
        Std_Vector,
    };

    class Special : public Type
    {
    public:
        virtual SpecialType getSpecialType() const = 0;

    protected:
        Special(std::string_view name, size_t size) : Type(name, size, Kind::Special) {}
        virtual ~Special() = default;
    };
}
