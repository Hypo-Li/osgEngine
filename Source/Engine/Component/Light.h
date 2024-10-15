#pragma once
#include <Engine/Core/Component.h>

namespace xxx
{
    class Light : public Component
    {
        REFLECT_CLASS(Light)
    public:
        
    };

    class DirectionalLight : public Light
    {
        REFLECT_CLASS(DirectionalLight)
    public:

    };
}
