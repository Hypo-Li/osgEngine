#pragma once
#include "Type.h"

#include <typeinfo>

namespace xxx::refl
{
    class AnyWrapper
    {
    public:
        virtual ~AnyWrapper() = default;
        virtual Type* getType() const = 0;
        virtual void* getValuePtr() = 0;
        virtual const void* getValuePtr() const = 0;
    };

    template <typename T>
    class AnyWrapperInstance : public AnyWrapper
    {
    public:
        AnyWrapperInstance(T&& value) : mValue(std::forward<T>(value)) {}
        virtual ~AnyWrapperInstance() = default;

        virtual Type* getType() const override
        {
            return Type::getType<T>();
        }

        virtual void* getValuePtr() override
        {
            return &mValue;
        }

        virtual const void* getValuePtr() const override
        {
            return &mValue;
        }

    private:
        T mValue;
    };

    class Any
    {
    public:
        Any() : mWrapper(nullptr) {}

        template <typename T>
        Any(T&& value) : mWrapper(new AnyWrapperInstance(std::forward<T>(value))) {}

        Any(const Any&) = delete;

        Any(Any&& other) noexcept : mWrapper(other.mWrapper)
        {
            other.mWrapper = nullptr;
        }

        ~Any()
        {
            if (mWrapper)
                delete mWrapper;
        }

        AnyWrapper* getWrapper() const
        {
            return mWrapper;
        }

        bool isValid() const
        {
            return mWrapper != nullptr;
        }

        Type* getType() const
        {
            return mWrapper ? mWrapper->getType() : nullptr;
        }

        template <typename T>
        T& getValue()
        {
            if (getType() == Type::getType<T>())
                return *(T*)mWrapper->getValuePtr();
            else
                throw std::bad_cast{};
        }

        template <typename T>
        const T& getValue() const
        {
            if (getType() == Type::getType<T>())
                return *(const T*)mWrapper->getValuePtr();
            else
                throw std::bad_cast{};
        }

        template <typename T, std::enable_if_t<std::is_pointer_v<T>, int> = 0>
        T getValuePtr()
        {
            if (mWrapper)
                return mWrapper->getValuePtr();
            else
                throw std::bad_cast{};
        }

        template <typename T, std::enable_if_t<std::is_pointer_v<T>, int> = 0>
        const T getValuePtr() const
        {
            if (mWrapper)
                return mWrapper->getValuePtr();
            else
                throw std::bad_cast{};
        }

    private:
        AnyWrapper* mWrapper;
    };
}
