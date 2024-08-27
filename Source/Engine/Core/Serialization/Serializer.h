#pragma once
#include "../Object.h"

#include <string>
#include <vector>
#include <map>
#include <string_view>

namespace xxx
{
    class Serializer
    {
    public:
        virtual ~Serializer() = 0;

        virtual void serialize(Object*& object) = 0;

    protected:
        
    };
}
