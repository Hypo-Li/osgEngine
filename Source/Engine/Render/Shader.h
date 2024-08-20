#pragma once
#include "Texture.h"

#include <osg/Shader>
#include <variant>

namespace xxx
{
    class Shader : public Object
    {
    public:

    private:
        using ShaderParameterType = std::variant<bool, int, float, osg::Vec2f, osg::Vec3f, osg::Vec4f, osg::ref_ptr<Texture>>;
        std::vector<std::pair<std::string, ShaderParameterType>> mShaderParameterAndDefaultValue;
        std::string mSource;
        osg::ref_ptr<osg::Shader> mOsgShader;
    };
}
