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

        virtual void* newInstances(size_t count) const override
        {
            return new std::string[count];
        }

        virtual void deleteInstances(void* instances) const override
        {
            delete[] static_cast<std::string*>(instances);
        }

        virtual bool compare(const void* instance1, const void* instance2) const override
        {
            return false;
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
        StdString() : Special("std::string", sizeof(std::string)) {}
    };
}
