#pragma once
#include <string_view>

namespace xxx::refl
{
    template <typename T>
    class AnyWrapperInstance;
    template <typename ClassType, typename ObjectType>
    class PropertyInstance;
    template <typename T, std::enable_if_t<std::is_member_function_pointer_v<T>, int>>
    class MethodInstance;
	class Type
	{
        template <typename T>
        friend class AnyWrapperInstance;
        template <typename ClassType, typename ObjectType>
        friend class PropertyInstance;
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
