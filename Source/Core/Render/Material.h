#pragma once
#include <osg/StateSet>
#include <osg/Texture2D>

namespace xxx
{
    class Material : public osg::StateSet
    {
    public:
        Material() = default;
        virtual ~Material() = default;

        enum ParameterType
        {
            Float,
            Float2,
            Float3,
            Float4,
            Bool,
            Int,
            Uint,
            Color3,
            Color4,
            Texture2D,
        };

        struct Parameter
        {
            std::string_view name;
            ParameterType type;
            union
            {
                float floatValue;
                osg::Vec2 float2Value;
                osg::Vec3 float3Value;
                osg::Vec4 float4Value;
                bool boolValue;
                int intValue;
                unsigned int uintValue;
                osg::Vec3 color3Value;
                osg::Vec4 color4Value;
                osg::Texture2D* texture2DValue;
            }defaultValue;
        };

    private:
        std::vector<Parameter> _parameters;
    };
}
