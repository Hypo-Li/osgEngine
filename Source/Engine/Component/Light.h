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
        virtual Type getType() const override
        {
            return Type::ImageBasedLight;
        }

        virtual bool onAddToEntity(Entity* entity) override
        {
            if (Light::onAddToEntity(entity))
            {
                if (!mImageCubemap)
                    setImageCubemap(AssetManager::get().getAsset("Engine/Texture/TestCubemap")->getRootObject<TextureCubemap>());
                Texture2D* brdfLutTexture = AssetManager::get().getAsset("Engine/Texture/BRDFLut")->getRootObject<Texture2D>();
                Pipeline* pipeline = Context::get().getEngine()->getPipeline();
                Pipeline::Pass* lightingPass = pipeline->getPass("Lighting");
                osg::StateSet* stateSet = lightingPass->getCamera()->getStateSet();
                stateSet->getUniform("uEnableIBL")->set(true);
                stateSet->setTextureAttribute(5, brdfLutTexture->getOsgTexture(), osg::StateAttribute::ON);
                stateSet->setTextureAttribute(6, mSpecularCubemap->getOsgTexture(), osg::StateAttribute::ON);
                osg::Uniform* shCoeffUniform = stateSet->getUniform("uSHCoeff");
                for (int i = 0; i < 9; ++i)
                    shCoeffUniform->setElement(i, mDiffuseSHCoeff[i]);

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
                osg::StateSet* stateSet = lightingPass->getCamera()->getStateSet();
                stateSet->getUniform("uEnableIBL")->set(false);
                return true;
            }
            return false;
        }

        virtual void setEnableCastShadows(bool enable) override
        {

        }

        void setImageCubemap(TextureCubemap* cubemap)
        {
            if (cubemap == mImageCubemap)
                return;

            mImageCubemap = cubemap;
            generateDiffuseSHCoeff();
            generateSpecularCubemap();
        }

    protected:
        osg::ref_ptr<TextureCubemap> mImageCubemap;
        std::array<osg::Vec4f, 9> mDiffuseSHCoeff;
        osg::ref_ptr<TextureCubemap> mSpecularCubemap;

        void generateDiffuseSHCoeff()
        {
            osg::ref_ptr<osg::Vec4Array> shCoeffBuffer = new osg::Vec4Array(16 * 16 * 9);
            osg::ref_ptr<osg::ShaderStorageBufferObject> shCoeffSSBO = new osg::ShaderStorageBufferObject;
            shCoeffBuffer->setBufferObject(shCoeffSSBO);
            osg::ref_ptr<osg::ShaderStorageBufferBinding> shCoeffSSBB = new osg::ShaderStorageBufferBinding(0, shCoeffBuffer, 0, shCoeffBuffer->size() * sizeof(osg::Vec4));

            osg::State* state = Context::get().getGraphicsContext()->getState();
            osg::GLExtensions* extensions = state->get<osg::GLExtensions>();

            osg::ref_ptr<osg::Program> diffuseSHCoeffProgram = new osg::Program;
            diffuseSHCoeffProgram->addShader(osgDB::readShaderFile(osg::Shader::COMPUTE, SHADER_DIR "SphericalHarmonics/DiffuseSHCoeff.comp.glsl"));
            diffuseSHCoeffProgram->apply(*state);
            osg::ref_ptr<osg::Uniform> uniform = new osg::Uniform("uCubemapTexture", 0);
            diffuseSHCoeffProgram->getPCP(*state)->apply(*uniform);
            shCoeffSSBB->apply(*state);
            state->setActiveTextureUnit(0);
            mImageCubemap->getOsgTexture()->apply(*state);
            state->applyMode(GL_TEXTURE_CUBE_MAP_SEAMLESS, true);

            extensions->glDispatchCompute(16, 16, 1);
            extensions->glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

            shCoeffSSBO->getGLBufferObject(state->getContextID())->bindBuffer();
            extensions->glGetBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, shCoeffBuffer->getTotalDataSize(), &shCoeffBuffer->at(0));

            const osg::Vec4f* shCoeffTemp = &shCoeffBuffer->at(0);
            for (int i = 0; i < 16 * 16; ++i)
            {
                mDiffuseSHCoeff[0] += shCoeffTemp[i * 9 + 0];
                mDiffuseSHCoeff[1] += shCoeffTemp[i * 9 + 1];
                mDiffuseSHCoeff[2] += shCoeffTemp[i * 9 + 2];
                mDiffuseSHCoeff[3] += shCoeffTemp[i * 9 + 3];
                mDiffuseSHCoeff[4] += shCoeffTemp[i * 9 + 4];
                mDiffuseSHCoeff[5] += shCoeffTemp[i * 9 + 5];
                mDiffuseSHCoeff[6] += shCoeffTemp[i * 9 + 6];
                mDiffuseSHCoeff[7] += shCoeffTemp[i * 9 + 7];
                mDiffuseSHCoeff[8] += shCoeffTemp[i * 9 + 8];
            }
            return;
        }

        void generateSpecularCubemap()
        {
            if (!mSpecularCubemap)
            {
                osg::TextureCubeMap* textureCubemap = new osg::TextureCubeMap;
                textureCubemap->setTextureSize(512, 512);
                textureCubemap->setInternalFormat(GL_RGBA16F);
                textureCubemap->setSourceFormat(GL_RGB);
                textureCubemap->setSourceType(GL_HALF_FLOAT);
                textureCubemap->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR_MIPMAP_LINEAR);
                textureCubemap->setUseHardwareMipMapGeneration(false);
                textureCubemap->setNumMipmapLevels(5);

                mSpecularCubemap = new TextureCubemap(textureCubemap);
                mSpecularCubemap->apply();
            }

            int numGroups[5][3] = {
                {16, 16, 6},
                {8, 8, 6},
                {32, 32, 6},
                {16, 16, 6},
                {8, 8, 6}
            };
            
            osg::State* state = Context::get().getGraphicsContext()->getState();
            osg::GLExtensions* extensions = state->get<osg::GLExtensions>();

            state->setActiveTextureUnit(0);
            mImageCubemap->getOsgTexture()->apply(*state);
            state->applyMode(GL_TEXTURE_CUBE_MAP_SEAMLESS, true);
            osg::ref_ptr<osg::Uniform> uniform = new osg::Uniform("uCubemapTexture", 0);

            for (int i = 0; i < 5; ++i)
            {
                osg::ref_ptr<osg::BindImageTexture> prefilterImage = new osg::BindImageTexture(0, mSpecularCubemap->getOsgTexture(), osg::BindImageTexture::WRITE_ONLY, GL_RGBA16F, i, true);
                osg::ref_ptr<osg::Program> prefilterProgram = new osg::Program;
                prefilterProgram->addShader(osgDB::readShaderFile(osg::Shader::COMPUTE, SHADER_DIR "IBL/Prefilter" + std::to_string(i) + ".comp.glsl"));
                prefilterProgram->apply(*state);
                prefilterProgram->getPCP(*state)->apply(*uniform);
                prefilterImage->apply(*state);

                extensions->glDispatchCompute(numGroups[i][0], numGroups[i][1], numGroups[i][2]);
                extensions->glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
            }
        }
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
