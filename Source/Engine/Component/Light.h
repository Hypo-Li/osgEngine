#pragma once
#include <Engine/Core/Component.h>
#include <Engine/Core/Engine.h>
#include <Engine/Core/Context.h>
#include <Engine/Render/Texture.h>

namespace xxx
{
    class Light : public Component
    {
        REFLECT_CLASS(Light)
    public:
        virtual void setEnableCastShadows(bool enable) = 0;

    protected:
    };

    class DirectionalLight : public Light
    {
        REFLECT_CLASS(DirectionalLight)
    public:
        virtual bool onAddToEntity(Entity* entity) override
        {
            if (Light::onAddToEntity(entity))
            {
                mLightIndex = sTotalLightCount++;
                std::string colorUniformName = "uDirectionalLight[" + std::to_string(mLightIndex) + "].color";
                mColorUniform = new osg::Uniform(colorUniformName.c_str(), mColor * mIntensity);
                std::string directionUniformName = "uDirectionalLight[" + std::to_string(mLightIndex) + "].direction";
                mDirectionUniform = new osg::Uniform(directionUniformName.c_str(), mDirection);

                Pipeline* pipeline = Context::get().getEngine()->getPipeline();
                Pipeline::Pass* lightingPass = pipeline->getPass("Lighting");
                lightingPass->applyUniform(mColorUniform);
                lightingPass->applyUniform(mDirectionUniform);
                osg::Uniform* directionalLightCountUniform = lightingPass->getCamera()->getOrCreateStateSet()->getUniform("uDirectionalLightCount");
                if (directionalLightCountUniform)
                {
                    uint32_t directionalLightCount = 0;
                    directionalLightCountUniform->get(directionalLightCount);
                    directionalLightCountUniform->set(directionalLightCount + 1);
                }
                else
                {
                    directionalLightCountUniform = new osg::Uniform("uDirectionalLightCount", 1);
                    lightingPass->applyUniform(directionalLightCountUniform);
                }
                return true;
            }
            return false;
        }

        virtual bool onRemoveFromEntity(Entity* entity) override
        {
            if (Light::onRemoveFromEntity(entity))
            {
                Pipeline* pipeline = Context::get().getEngine()->getPipeline();
                Pipeline::Pass* lightingPass = pipeline->getPass("Lighting");
                osg::StateSet* stateSet = lightingPass->getCamera()->getOrCreateStateSet();
                stateSet->removeUniform(mColorUniform);
                stateSet->removeUniform(mDirectionUniform);
                osg::Uniform* directionalLightCountUniform = lightingPass->getCamera()->getOrCreateStateSet()->getUniform("uDirectionalLightCount");
                uint32_t directionalLightCount = 0;
                directionalLightCountUniform->get(directionalLightCount);
                directionalLightCountUniform->set(directionalLightCount - 1);
                --sTotalLightCount;
                return true;
            }
            return false;
        }

        virtual Type getType() const override
        {
            return Type::DirectionLight;
        }

        virtual void setEnableCastShadows(bool enable) override
        {
            mCastShadows = enable;
            if (mCastShadows)
                addShadowPass(mLightIndex);
            else
                removeShadowPass(mLightIndex);
        }

        float getIntensity() const
        {
            return mIntensity;
        }

        void setIntensity(float intensity)
        {
            mIntensity = intensity;
            mColorUniform->set(mColor * mIntensity);
        }

        osg::Vec3f getColor() const
        {
            return mColor;
        }

        void setColor(osg::Vec3f color)
        {
            mColor = color;
            mColorUniform->set(mColor * mIntensity);
        }

        osg::Vec3f getDirection() const
        {
            return mDirection;
        }

        void setDirection(osg::Vec3f direction)
        {
            mDirection = direction;
            mDirectionUniform->set(mDirection);
        }

    protected:
        float mIntensity = 6.0f;
        osg::Vec3f mColor = osg::Vec3f(1, 1, 1);
        bool mCastShadows = true;

        inline static uint8_t sTotalLightCount = 0;
        uint8_t mLightIndex = 0;
        osg::Vec3f mDirection = osg::Vec3f(0, 0, -1);
        bool mIsSunLight = true;
        osg::ref_ptr<osg::Uniform> mColorUniform;
        osg::ref_ptr<osg::Uniform> mDirectionUniform;

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

    class ImageBasedLight : public Light
    {
        REFLECT_CLASS(ImageBasedLight)
    public:
        virtual bool onAddToEntity(Entity* entity) override
        {
            if (!Light::onAddToEntity(entity))
                return false;
            return true;
        }

        virtual bool onRemoveFromEntity(Entity* entity) override
        {
            if (!Light::onRemoveFromEntity(entity))
                return false;
            return true;
        }

        virtual void setEnableCastShadows(bool enable) override
        {

        }

    protected:
        osg::Vec4f mDiffuseSH[9];
        osg::ref_ptr<TextureCubemap> mSpecular;
    };

    namespace refl
    {
        template <> Type* Reflection::createType<Light>()
        {
            Class* clazz = new ClassInstance<Light, Component>("Light");
            return clazz;
        }

        template <> Type* Reflection::createType<DirectionalLight>()
        {
            Class* clazz = new ClassInstance<DirectionalLight, Light>("DirectionalLight");
            clazz->addProperty("Intensity", &DirectionalLight::mIntensity);
            clazz->addProperty("Color", &DirectionalLight::mColor);
            clazz->addProperty("CastShadows", &DirectionalLight::mCastShadows);
            clazz->addProperty("Direction", &DirectionalLight::mDirection);
            clazz->addProperty("IsSunLight", &DirectionalLight::mIsSunLight);
            return clazz;
        }

        template <> Type* Reflection::createType<ImageBasedLight>()
        {
            Class* clazz = new ClassInstance<ImageBasedLight, Light>("ImageBasedLight");
            return clazz;
        }
    }
}
