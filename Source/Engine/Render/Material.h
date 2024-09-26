#pragma once
#include "Shader.h"

namespace xxx
{
    class Material : public Object
    {
        REFLECT_CLASS(Material)
    public:

    protected:
        osg::ref_ptr<Shader> mShader;
        std::map<std::string, Shader::Parameter> mParameters;
    };

    namespace refl
    {
        template<> inline Type* Reflection::getType<Material>()
        {
            Class* clazz = new ClassInstance<Material>("Material");
            clazz->addProperty("Shader", &Material::mShader);
            clazz->addProperty("Parameters", &Material::mParameters);
            getClassMap().emplace("Material", clazz);
            return clazz;
        }
    }
}
