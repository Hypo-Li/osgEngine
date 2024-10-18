#pragma once
#include <Engine/Core/Component.h>
#include <Engine/Core/Engine.h>
#include <Engine/Core/Context.h>

namespace xxx
{
    class Light : public Component
    {
        REFLECT_CLASS(Light)
    public:
        virtual void setEnableCastShadows(bool enable) = 0;

    protected:
        float mIntensity = 6.0f;
        osg::Vec3f mLightColor = osg::Vec3f(1, 1, 1);
        bool mCastShadows = true;
    };

    struct DirectionLightParameters
    {
        osg::Vec3f lightColor;
        osg::Vec3f direction;
    };

    class DirectionalLight : public Light
    {
        REFLECT_CLASS(DirectionalLight)
    public:
        DirectionalLight()
        {
            /*Pipeline* pipeline = Context::get().getEngine()->getPipeline();
            uint32_t transparentPassIndex = pipeline->getPassIndex("Transparent");
            std::string lightingPassName = std::string(sClass->getName()) + std::to_string(mLightIndex);
                
            osg::Program* lightingProgram = new osg::Program;
            lightingProgram->addShader();
            lightingProgram->addShader(osgDB::readShaderFile(osg::Shader::FRAGMENT, ""));
            Pipeline::Pass* lightingPass = pipeline->insertWorkPass(transparentPassIndex, lightingPassName, lightingProgram, GL_COLOR_BUFFER_BIT);
            lightingPass->applyUniform(new osg::Uniform("uLightDirection", mDirection));
            lightingPass->applyUniform(new osg::Uniform("uIsSunLight", mIsSunLight));

            if (mCastShadows)
                addShadowPass(mLightIndex);*/
        }

        virtual void setEnableCastShadows(bool enable) override
        {
            mCastShadows = enable;
            if (mCastShadows)
                addShadowPass(mLightIndex);
            else
                removeShadowPass(mLightIndex);
        }

    protected:
        uint8_t mLightIndex = 0;
        osg::Vec3f mDirection = osg::Vec3f(0, 0, -1);
        bool mIsSunLight = true;

        static void addShadowPass(uint32_t lightIndex)
        {
            Pipeline* pipeline = Context::get().getEngine()->getPipeline();
            std::string namePrefix = std::string(sClass->getName()) + std::to_string(lightIndex);
            uint32_t lightingPassIndex = pipeline->getPassIndex(namePrefix);

            //Pipeline::Pass* shadowCastPass = pipeline->insertInputPass(lightingPassIndex, namePrefix + " ShadowCast");
            //Pipeline::Pass* shadowMaskPass = pipeline->insertWorkPass(lightingPassIndex + 1, namePrefix + " ShadowMask");
        }

        static void removeShadowPass(uint32_t lightIndex)
        {
            Pipeline* pipeline = Context::get().getEngine()->getPipeline();
            std::string namePrefix = std::string(sClass->getName()) + std::to_string(lightIndex);
            uint32_t lightingPassIndex = pipeline->getPassIndex(namePrefix);

            Pipeline::Pass* shadowCastPass = pipeline->getPass(lightingPassIndex - 2);
            Pipeline::Pass* shadowMaskPass = pipeline->getPass(lightingPassIndex - 1);
            pipeline->removePass(shadowCastPass);
            pipeline->removePass(shadowMaskPass);
            // replace shadow mask texture
        }
    };
}
