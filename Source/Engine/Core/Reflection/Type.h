#pragma once
#include <string_view>

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
using container_traits_t = typename container_traits<T>::type;
template <typename T>
using container_traits_t1 = typename container_traits<T>::type1;
template <typename T>
using container_traits_t2 = typename container_traits<T>::type2;

namespace xxx::refl
{
    template <typename T>
    class AnyWrapperInstance;
    template <typename Owner, typename Declared, std::size_t Index>
    class PropertyMemberInstance;
    template <typename Owner, typename Declared>
    class PropertyAccessorInstance;
    template <typename T, std::enable_if_t<std::is_member_function_pointer_v<T>, int>>
    class MethodInstance;
	class Type
	{
        template <typename T>
        friend class AnyWrapperInstance;
        template <typename Owner, typename Declared, std::size_t Index>
        friend class PropertyMemberInstance;
        template <typename Owner, typename Declared>
        friend class PropertyAccessorInstance;
        template <typename T, std::enable_if_t<std::is_member_function_pointer_v<T>, int>>
        friend class MethodInstance;
    public:
        Type(const Type&) = delete;

		enum class Kind
		{
			Fundamental,
			Enum,
			Struct,
			Class,
			Special,
		};

		std::string_view getName() const { return mName; }
		size_t getSize() const { return mSize; }

		virtual Kind getKind() const = 0;
        virtual void* newInstance() const = 0;
        virtual void* newInstances(size_t count) const = 0;
        virtual void deleteInstance(void* instance) const = 0;
        virtual void deleteInstances(void* instances) const = 0;
        virtual bool compare(const void* instance1, const void* instance2) const = 0;

    protected:
        Type() = default;
        Type(std::string_view name, size_t size) : mName(name), mSize(size) {}
        virtual ~Type() = default;

		std::string_view mName;
		size_t mSize;

        template <typename T>
        static Type* getType();
	};
}
