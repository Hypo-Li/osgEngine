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

    protected:
        osg::ref_ptr<Shader> mShader;
        std::map<std::string, Shader::Parameter> mParameters;

        osg::ref_ptr<osg::StateSet> mOsgStateSet;

        void setRenderingPath(RenderingPath renderingPath);

        void setShadingModel(ShadingModel shadingModel);

        void setAlphaMode(AlphaMode alphaMode);

        void setDoubleSided(bool doubleSided);
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
