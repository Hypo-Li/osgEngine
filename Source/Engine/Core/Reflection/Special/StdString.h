#pragma once
#include "../Special.h"

namespace xxx::refl
{
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
            return *static_cast<const std::string*>(instance1) == *static_cast<const std::string*>(instance2);
        }

    private:
        StdString() : Special("std::string", sizeof(std::string), Case::StdString) {}
    };
}
