#pragma once
#include "Shader.h"

namespace xxx
{
    class Material : public Object
    {
        REFLECT_CLASS(Material)
    public:
        Material() :
            mOsgStateSet(new osg::StateSet)
        {

        }

        osg::StateSet* getOsgStateSet() const
        {
            return mOsgStateSet;
        }

        void setShader(Shader* shader);

        template <typename T, typename = std::enable_if_t<is_shader_parameter_v<T>>>
        void setParameter(const std::string& name, T value)
        {
            Shader::Parameter parameterValue;
            if constexpr (std::is_base_of_v<xxx::Texture, std::remove_pointer_t<T>>)
                parameterValue = osg::ref_ptr<Texture>(value);
            else
                parameterValue = value;

            const auto& shaderParameters = mShader->getParameters();
            auto findResult = shaderParameters.find(name);
            if (findResult != shaderParameters.end() && findResult->second.index() == parameterValue.index())
            {
                mParameters[name] = parameterValue;
                std::string uniformName = "u" + name;
                osg::Uniform* uniform = mOsgStateSet->getUniform(uniformName);

                if constexpr (std::is_base_of_v<xxx::Texture, std::remove_pointer_t<T>>)
                {
                    int textureUnit = 0;
                    for (auto it = shaderParameters.begin(); it != shaderParameters.end(); ++it)
                    {
                        if (it == findResult)
                            break;
                        if (it.second.index() == 6)
                            ++textureUnit;
                    }
                    uniform->set(textureUnit);
                    mOsgStateSet->setTextureAttribute(textureUnit, static_cast<Texture*>(value)->getOsgTexture(), osg::StateAttribute::ON);
                }
                else
                    uniform->set(value);
            }
        }

    protected:
        osg::ref_ptr<Shader> mShader;
        std::map<std::string, Shader::Parameter> mParameters;
        RenderingPath mRenderingPath = RenderingPath::Deferred;
        ShadingModel mShadingModel = ShadingModel::Standard;
        AlphaMode mAlphaMode = AlphaMode::Opaque;
        bool mDoubleSided = false;

        osg::ref_ptr<osg::StateSet> mOsgStateSet;

        void setRenderingPath(RenderingPath renderingPath);

        void setShadingModel(ShadingModel shadingModel);

        void setAlphaMode(AlphaMode alphaMode);

        void setDoubleSided(bool doubleSided);

        std::string getParameterTypeString(const Shader::Parameter& parameter)
        {
            switch (parameter.index())
            {
            case 0:
                return "bool";
            case 1:
                return "int";
            case 2:
                return "float";
            case 3:
                return "vec2";
            case 4:
                return "vec3";
            case 5:
                return "vec4";
            case 6:
            {
                switch (std::get<osg::ref_ptr<Texture>>(parameter)->getOsgTexture()->getTextureTarget())
                {
                case GL_TEXTURE_2D:
                    return "sampler2D";
                case GL_TEXTURE_2D_ARRAY:
                    return "sampler2DArray";
                case GL_TEXTURE_3D:
                    return "sampler3D";
                case GL_TEXTURE_CUBE_MAP:
                    return "samplerCube";
                default:
                    return "";
                }
            }
            default:
                return "";
            }
        }
    };

    namespace refl
    {
        template<> inline Type* Reflection::createType<Material>()
        {
            Class* clazz = new ClassInstance<Material>("Material");
            clazz->addProperty("Shader", &Material::mShader);
            clazz->addProperty("Parameters", &Material::mParameters);
            getClassMap().emplace("Material", clazz);
            return clazz;
        }
    }
}
