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

        void setShader(Shader* shader)
        {
            mShader = shader;
            osg::Program* program = new osg::Program;
            program->addShader(shader->getOsgShader());
            // TODO: add framework shader
            //program->addShader();
            mOsgStateSet->setAttribute(program, osg::StateAttribute::ON);
            const auto& parameters = mShader->getParameters();
            //for (const auto& param : parameters)
            //{
            //    mOsgStateSet->addUniform(new osg::Uniform("u" + param.first, ))
            //}
        }

        virtual void preSerialize() override
        {
            
        }

        virtual void postSerialize() override
        {

        }

    protected:
        osg::ref_ptr<Shader> mShader;
        std::map<std::string, Shader::Parameter> mParameters;

        osg::ref_ptr<osg::StateSet> mOsgStateSet;
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
