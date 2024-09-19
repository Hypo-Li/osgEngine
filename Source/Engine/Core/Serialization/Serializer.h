#pragma once
#include "../Object.h"

#include <string>
#include <vector>
#include <map>
#include <string_view>

namespace xxx
{
    class Serializer : public osg::Referenced
    {
    public:
        virtual ~Serializer() = default;

        virtual void serializeObject(Object*& object) = 0;
        
    };
}
