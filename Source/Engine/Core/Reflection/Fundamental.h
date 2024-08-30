#pragma once
#include "Type.h"

namespace xxx::refl
{
    class Reflection;
    class Fundamental : public Type
    {
        friend class Reflection;
    public:
        virtual Kind getKind() const override { return Kind::Fundamental; }

        virtual void* newInstance() const override
        {
            return malloc(mSize);
        }

        virtual void deleteInstance(void* instance) const override
        {
            free(instance);
        }

    protected:
        Fundamental(std::string_view name, size_t size) : Type(name, size) {}
        virtual ~Fundamental() = default;
    };
}
