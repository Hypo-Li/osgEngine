#pragma once
#include "../Special.h"

#include <string>

namespace xxx::refl
{
    class Reflection;
    class StdString : public Special
    {
        friend class Reflection;
    public:
        virtual void* newInstance() const override
        {
            return new std::string;
        }

        virtual void deleteInstance(void* instance) const override
        {
            delete static_cast<std::string*>(instance);
        }

        virtual SpecialType getSpecialType() const
        {
            return SpecialType::Std_String;
        }

        size_t getLength(void* string) const
        {
            return static_cast<std::string*>(string)->size();
        }

        const char* getCStr(void* string) const
        {
            return static_cast<std::string*>(string)->c_str();
        }

    protected:
        StdString()
        {
            mName = "std::string";
            mSize = sizeof(std::string);
        }
    };
}
