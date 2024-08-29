#pragma once
#include "Type.h"

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
        virtual void* newInstance() const = 0;
        virtual void deleteInstance(void* instance) const = 0;

    protected:
        virtual ~Special() = 0;
    };
}
