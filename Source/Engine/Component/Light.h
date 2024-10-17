#pragma once
#include <Engine/Core/Component.h>

namespace xxx
{
    class Light : public Component
    {
        REFLECT_CLASS(Light)
    public:

    protected:
        float mIntensity = 6.0f;
        osg::Vec3f mLightColor = osg::Vec3f(1, 1, 1);
        bool mCastShadows = true;
    };

    class DirectionalLight : public Light
    {
        REFLECT_CLASS(DirectionalLight)
    public:

    protected:
        osg::Vec3f mDirection = osg::Vec3f(0, 0, -1);
        bool mIsSunLight = true;
    };
}
