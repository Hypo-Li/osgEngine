#pragma once
#include <string_view>
#include <unordered_map>

namespace xxx::refl
{
    template <typename>
    class TAnyWrapper;
    template <typename, typename, std::size_t>
    class TProperty;
    template <typename, typename>
    class TMethod;
	class Type
	{
        template <typename>
        friend class TAnyWrapper;
        template <typename, typename, std::size_t>
        friend class TProperty;
        template <typename, typename>
        friend class TMethod;
    public:
        Type(const Type&) = delete;

		enum class Kind
		{
			Fundamental,
			Enumeration,
			Structure,
			Class,
			Special,
		};

		std::string_view getName() const { return mName; }
		size_t getSize() const { return mSize; }
        Kind getKind() const { return mKind; }

        virtual void* newInstance() const = 0;
        virtual void* newInstances(size_t count) const = 0;
        virtual void deleteInstance(void* instance) const = 0;
        virtual void deleteInstances(void* instances) const = 0;
        virtual bool compare(const void* instance1, const void* instance2) const = 0;

    protected:
        Type(std::string_view name, size_t size, Kind kind) : mName(name), mSize(size), mKind(kind) {}
        virtual ~Type() = default;

		std::string_view mName;
		size_t mSize;
        Kind mKind;
	};
}
