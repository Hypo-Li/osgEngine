#pragma once
#include "Shader.h"

namespace xxx
{
    enum class ShadingModel
    {
        Unlit,
        Standard,
    };

    enum class AlphaMode
    {
        Opaque,
        Mask,
        Blend,
    };

    class Material : public Object
    {
        REFLECT_CLASS(Material)
    public:
        Material() :
            mOsgStateSet(new osg::StateSet)
        {

        }

        virtual void postSerialize(Serializer* serializer)
        {
            if (serializer->isLoader())
            {
                setShadingModel(mShadingModel);
                setAlphaMode(mAlphaMode);
                setDoubleSided(mDoubleSided);
                syncShaderState();
            }
        }

        osg::StateSet* getOsgStateSet() const
        {
            return mOsgStateSet;
        }

        void setShader(Shader* shader);

        void syncShaderState();

        Shader* getShader() const
        {
            return mShader;
        }

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

        const auto& getParameters() const
        {
            return mParameters;
        }

        void setShadingModel(ShadingModel shadingModel);

        void setAlphaMode(AlphaMode alphaMode);

        void setDoubleSided(bool doubleSided);

        ShadingModel getShadingModel() const
        {
            return mShadingModel;
        }

        AlphaMode getAlphaMode() const
        {
            return mAlphaMode;
        }

        bool getDoubleSided() const
        {
            return mDoubleSided;
        }

    protected:
        osg::ref_ptr<Shader> mShader;
        std::map<std::string, Shader::Parameter> mParameters;
        ShadingModel mShadingModel = ShadingModel::Standard;
        AlphaMode mAlphaMode = AlphaMode::Opaque;
        bool mDoubleSided = false;

        osg::ref_ptr<osg::StateSet> mOsgStateSet;

        std::string getParameterTypeString(const Shader::Parameter& parameter);
    };

    namespace refl
    {
        template <>
        inline Type* Reflection::createType<ShadingModel>()
        {
            Enum* enumerate = new EnumInstance<ShadingModel>("ShadingModel", {
                {"Unlit", ShadingModel::Unlit},
                {"Standard", ShadingModel::Standard},
            });
            return enumerate;
        }

        template <>
        inline Type* Reflection::createType<AlphaMode>()
        {
            Enum* enumerate = new EnumInstance<AlphaMode>("AlphaMode", {
                {"Opaque", AlphaMode::Opaque},
                {"Mask", AlphaMode::Mask},
                {"Blend", AlphaMode::Blend},
            });
            return enumerate;
        }

        template<> inline Type* Reflection::createType<Material>()
        {
            Class* clazz = new ClassInstance<Material>("Material");
            clazz->addProperty("Shader", &Material::mShader);
            clazz->addProperty("Parameters", &Material::mParameters);
            clazz->addProperty("ShadingModel", &Material::mShadingModel);
            clazz->addProperty("AlphaMode", &Material::mAlphaMode);
            clazz->addProperty("DoubleSided", &Material::mDoubleSided);
            return clazz;
        }
    }
}
