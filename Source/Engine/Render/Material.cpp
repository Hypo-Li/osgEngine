#include "Material.h"

namespace xxx
{
    void Material::setShader(Shader* shader)
    {
        mShader = shader;

        osg::Program* program = new osg::Program;
        program->addShader(shader->getOsgShader());
        // TODO: add framework shader
        //program->addShader();

        setRenderingPath(mShader->getRenderingPath());
        setShadingModel(mShader->getShadingModel());
        setAlphaMode(mShader->getAlphaMode());
        setDoubleSided(mShader->getDoubleSided());

        mOsgStateSet->setAttribute(program, osg::StateAttribute::ON);
        int textureUnit = 0;
        const auto& parameters = mShader->getParameters();
        for (const auto& param : parameters)
        {
            std::string uniformName = "u" + param.first;
            osg::Uniform* uniform = nullptr;
            switch (param.second.index())
            {
            case 0:
                uniform = new osg::Uniform(uniformName.c_str(), std::get<bool>(param.second));
                break;
            case 1:
                uniform = new osg::Uniform(uniformName.c_str(), std::get<int>(param.second));
                break;
            case 2:
                uniform = new osg::Uniform(uniformName.c_str(), std::get<float>(param.second));
                break;
            case 3:
                uniform = new osg::Uniform(uniformName.c_str(), std::get<osg::Vec2f>(param.second));
                break;
            case 4:
                uniform = new osg::Uniform(uniformName.c_str(), std::get<osg::Vec3f>(param.second));
                break;
            case 5:
                uniform = new osg::Uniform(uniformName.c_str(), std::get<osg::Vec4f>(param.second));
                break;
            case 6:
            {
                Texture* texture = std::get<osg::ref_ptr<Texture>>(param.second);
                uniform = new osg::Uniform(uniformName.c_str(), textureUnit);
                mOsgStateSet->setTextureAttribute(textureUnit, texture->getOsgTexture(), osg::StateAttribute::ON);
                ++textureUnit;
                break;
            }
            default:
                break;
            }
            mOsgStateSet->addUniform(uniform, osg::StateAttribute::ON);
        }

    }



    void Material::setRenderingPath(RenderingPath renderingPath)
    {

    }

    void Material::setShadingModel(ShadingModel shadingModel)
    {
        mOsgStateSet->setDefine("SHADING_MODEL", std::to_string(int(shadingModel)));
    }

    void Material::setAlphaMode(AlphaMode alphaMode)
    {
        switch (alphaMode)
        {
        case AlphaMode::Opaque:
        case AlphaMode::Mask:
            mOsgStateSet->setMode(GL_BLEND, osg::StateAttribute::OFF);
            mOsgStateSet->setRenderingHint(osg::StateSet::OPAQUE_BIN);
            break;
        case AlphaMode::Blend:
            mOsgStateSet->setMode(GL_BLEND, osg::StateAttribute::ON);
            mOsgStateSet->setRenderingHint(osg::StateSet::TRANSPARENT_BIN);
            break;
        default:
            break;
        }
        mOsgStateSet->setDefine("ALPHA_MODE", std::to_string(int(alphaMode)));
    }

    void Material::setDoubleSided(bool doubleSided)
    {
        if (doubleSided)
            mOsgStateSet->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
        else
            mOsgStateSet->setMode(GL_CULL_FACE, osg::StateAttribute::ON);
    }
}
