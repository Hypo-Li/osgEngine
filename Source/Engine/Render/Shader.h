#pragma once
#include <Engine/Core/Object.h>

#include <osg/Shader>
#include <variant>

namespace xxx
{
    class Shader : public Object
    {
        using Super = Object;
    public:
        virtual void serialize(Serializer& serializer)
        {
            Super::serialize(serializer);

        }

    private:
        using ShaderParameterType = std::variant<bool, int, float, osg::Vec2f, osg::Vec3f, osg::Vec4f>;
        std::vector<std::pair<std::string, ShaderParameterType>> mShaderParameterAndDefaultValue;
        osg::ref_ptr<osg::Shader> mOsgShader;
    };
}
