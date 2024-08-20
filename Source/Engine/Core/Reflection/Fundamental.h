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

    protected:
        Fundamental(std::string_view name, size_t size) : Type(name, size) {}
        virtual ~Fundamental() = default;
    };
}
