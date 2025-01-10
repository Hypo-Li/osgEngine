#pragma once
#include "Shader.h"

#define GBUFFER_MASK            (1u << 0)
#define TRANSPARENT_MASK        (1u << 1)
#define SHADOW_CAST_MASK        (1u << 2)
#define POST_FORWARD_MASK       (1u << 3)

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
            mOsgStateSet(new osg::StateSet) {}

        virtual void postLoad() override
        {
            setShadingModel(mShadingModel);
            setAlphaMode(mAlphaMode);
            setDoubleSided(mDoubleSided);
            syncWithShader();
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
            size_t typeIndex = materialParamIt->second.first.index();
            if (materialParamIt == mParameters.end())
                return;

            if constexpr (std::is_same_v<std::remove_pointer_t<T>, xxx::Texture2D>)
            {
                if (typeIndex == size_t(Shader::ParameterType::Texture2D))
                {
                    mParameters[name].first = std::make_pair(
                        osg::ref_ptr<Texture2D>(value),
                        std::get<Shader::Texture2DUnitPair>(materialParamIt->second.first).second
                    );
                }
            }
            else if constexpr (std::is_same_v<std::remove_pointer_t<T>, xxx::Texture2DArray>)
            {
                if (typeIndex == size_t(Shader::ParameterType::Texture2DArray))
                {
                    mParameters[name].first = std::make_pair(
                        osg::ref_ptr<Texture2DArray>(value),
                        std::get<Shader::Texture2DArrayUnitPair>(materialParamIt->second.first).second
                    );
                }
            }
            else if constexpr (std::is_same_v<std::remove_pointer_t<T>, xxx::Texture3D>)
            {
                if (typeIndex == size_t(Shader::ParameterType::Texture3D))
                {
                    mParameters[name].first = std::make_pair(
                        osg::ref_ptr<Texture3D>(value),
                        std::get<Shader::Texture3DUnitPair>(materialParamIt->second.first).second
                    );
                }
            }
            else if constexpr (std::is_same_v<std::remove_pointer_t<T>, xxx::TextureCubemap>)
            {
                if (typeIndex == size_t(Shader::ParameterType::TextureCubemap))
                {
                    mParameters[name].first = std::make_pair(
                            osg::ref_ptr<TextureCubemap>(value),
                            std::get<Shader::TextureCubemapUnitPair>(materialParamIt->second.first).second
                    );
                }
            }
            else
            {
                if (materialParamIt->second.first.index() == variant_index<T, Shader::ParameterValue>())
                {
                    mParameters[name].first = value;
                }
            }

            applyParameter(materialParamIt);
            /*if (std::this_thread::get_id() == Context::get().getRenderingThreadId())
            {
                applyParameter(materialParamIt);
            }
            else
            {
                Context::get().getRenderingCommandQueue().push([this, materialParamIt]() {
                    applyParameter(materialParamIt);
                });
            }*/
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
            Enumeration* enumeration = new TEnumeration<ShadingModel>("ShadingModel", {
                {"Unlit", ShadingModel::Unlit},
                {"Standard", ShadingModel::Standard},
            });
            return enumeration;
        }

        template <>
        inline Type* Reflection::createType<AlphaMode>()
        {
            Enumeration* enumeration = new TEnumeration<AlphaMode>("AlphaMode", {
                {"Opaque", AlphaMode::Opaque},
                {"Mask", AlphaMode::Mask},
                {"Blend", AlphaMode::Blend},
            });
            return enumeration;
        }

        template<> inline Type* Reflection::createType<Material>()
        {
            Class* clazz = new TClass<Material>("Material");
            clazz->addProperty("Shader", &Material::mShader);
            clazz->addProperty("Parameters", &Material::mParameters);
            clazz->addProperty("ShadingModel", &Material::mShadingModel);
            clazz->addProperty("AlphaMode", &Material::mAlphaMode);
            clazz->addProperty("DoubleSided", &Material::mDoubleSided);
            return clazz;
        }
    }
}
