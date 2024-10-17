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
                syncWithShader();
            }
        }

        osg::StateSet* getOsgStateSet() const
        {
            return mOsgStateSet;
        }

        void setShader(Shader* shader);

        void syncWithShader();

        Shader* getShader() const
        {
            return mShader;
        }

        template <typename T, typename = std::enable_if_t<is_shader_parameter_v<T>>>
        void setParameter(const std::string& name, T value)
        {
            auto materialParamIt = mParameters.find(name);
            if constexpr (std::is_base_of_v<xxx::Texture, std::remove_pointer_t<T>>)
            {
                if (materialParamIt != mParameters.end() && materialParamIt->second.first.index() == size_t(Shader::ParameterIndex::Texture))
                {
                    const Shader::TextureAndUnit& textureAndUnit = std::get<Shader::TextureAndUnit>(materialParamIt->second.first);
                    mParameters[name].first = std::make_pair(osg::ref_ptr<Texture>(value), textureAndUnit.second);
                    applyParameter(materialParamIt);
                }
            }
            else
            {
                if (materialParamIt != mParameters.end() && materialParamIt->second.first.index() == variant_index<T, Shader::ParameterValue>())
                {
                    mParameters[name].first = value;
                    applyParameter(materialParamIt);
                }
            }
        }

        void setParameter(const std::string& name, const Shader::ParameterValue& value)
        {
            auto materialParamIt = mParameters.find(name);
            if (materialParamIt != mParameters.end() && materialParamIt->second.first.index() == value.index())
            {
                mParameters[name].first = value;
                applyParameter(materialParamIt);
            }
        }

        void enableParameter(const std::string& name, bool enable)
        {
            auto materialParamIt = mParameters.find(name);
            if (materialParamIt != mParameters.end())
            {
                materialParamIt->second.second = enable;
                applyParameter(materialParamIt);
            }
        }

        using Parameters = std::map<std::string, std::pair<Shader::ParameterValue, bool>>;
        const Parameters& getParameters() const
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

        uint32_t getOsgNodeMask();

    protected:
        osg::ref_ptr<Shader> mShader;
        // std::map<Name, std::pair<Value, Enable>>
        Parameters mParameters;
        ShadingModel mShadingModel = ShadingModel::Standard;
        AlphaMode mAlphaMode = AlphaMode::Opaque;
        bool mDoubleSided = false;

        osg::ref_ptr<osg::StateSet> mOsgStateSet;

        std::string getParameterTypeString(const Shader::ParameterValue& parameter);

        void applyParameter(Parameters::const_iterator materialParamIt);
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
