#pragma once
#include "Any.h"

#include <type_traits>

namespace xxx::refl
{
    class Argument
    {
    public:
        template <typename T>
        Argument(T&& value)
        {
            mData = &value;
        }

        Argument(Any& any)
        {
            mData = any.getValuePtr<void*>();
        }

        template <typename T>
        T getValue() const
        {
            return *(T*)(mData);
        }

    private:
        void* mData;
    };
}
