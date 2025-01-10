#pragma once
#include <Engine/Core/Component.h>
#include <Engine/Core/Engine.h>
#include <Engine/Core/Context.h>
#include <Engine/Render/Texture2D.h>
#include <Engine/Render/TextureCubemap.h>
#include <osg/CullFace>

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
        virtual void onEnable() override
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

            Pipeline::Pass* transparentPass = pipeline->getPass("Transparent");
            transparentPass->applyUniform(mColorUniform);
            transparentPass->applyUniform(mDirectionUniform);

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

            setEnableCastShadows(true);
        }

        virtual void onDisable() override
        {
            Pipeline* pipeline = Context::get().getEngine()->getPipeline();
            Pipeline::Pass* lightingPass = pipeline->getPass("Lighting");
            osg::StateSet* lightingStateSet = lightingPass->getCamera()->getOrCreateStateSet();
            lightingStateSet->removeUniform(mColorUniform);
            lightingStateSet->removeUniform(mDirectionUniform);

            Pipeline::Pass* transparentPass = pipeline->getPass("Transparent");
            osg::StateSet* transparentStateSet = transparentPass->getCamera()->getOrCreateStateSet();
            transparentStateSet->removeUniform(mColorUniform);
            transparentStateSet->removeUniform(mDirectionUniform);

            osg::Uniform* directionalLightCountUniform = lightingPass->getCamera()->getOrCreateStateSet()->getUniform("uDirectionalLightCount");
            uint32_t directionalLightCount = 0;
            directionalLightCountUniform->get(directionalLightCount);
            directionalLightCountUniform->set(directionalLightCount - 1);
            --sTotalLightCount;
        }

        virtual Type getType() const override
        {
            return Type::DirectionLight;
        }

        virtual void setEnableCastShadows(bool enable) override
        {
            mCastShadows = enable;
            if (mCastShadows)
                addShadowPasses();
            else
                removeShadowPasses();
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

        class ShadowCamerasUpdateCallback : public osg::Camera::DrawCallback
        {
            std::vector<osg::ref_ptr<osg::Camera>> mShadowCameras;
            DirectionalLight* mDirectionalLight;
        public:
            ShadowCamerasUpdateCallback(const std::vector<osg::ref_ptr<osg::Camera>>& shadowCameras, DirectionalLight* directionalLight) :
                mShadowCameras(shadowCameras), mDirectionalLight(directionalLight) {}

            virtual void operator () (const osg::Camera& camera) const
            {
                const osg::Matrixd& viewMatrix = camera.getViewMatrix();
                const osg::Matrixd& projMatrix = camera.getProjectionMatrix();
                osg::Matrixd inverseViewProjMatrix = osg::Matrixd::inverse(viewMatrix * projMatrix);
                const uint32_t cascadeCount = mShadowCameras.size();

                constexpr float distributionExponent = 3.0f;
                constexpr float shadowCastDistance = 1500.0f;
                constexpr float lightFrustumNearOffset = 800.0f;

                float weightSum = (1.0 - std::pow(distributionExponent, float(cascadeCount))) / (1.0 - distributionExponent); // exp: 1 + 3 + 9 + 27 = 40
                double col3z = projMatrix(2, 2), col4z = projMatrix(3, 2);
                double zNear = std::max(col4z / (col3z - 1.0), 0.01);
                float cascadeNear = zNear;

                float currentWeight = 0.0f;
                for (uint32_t i = 0; i < cascadeCount; ++i)
                {
                    currentWeight += std::pow(distributionExponent, float(i));
                    float cascadeFar = zNear + (currentWeight / weightSum) * shadowCastDistance;
                    double ndcNear = (-cascadeNear * projMatrix(2, 2) + projMatrix(3, 2)) / (-cascadeNear * projMatrix(2, 3) + projMatrix(3, 3));
                    double ndcFar = (-cascadeFar * projMatrix(2, 2) + projMatrix(3, 2)) / (-cascadeFar * projMatrix(2, 3) + projMatrix(3, 3));

                    osg::Vec3d worldSpace[8] = {
                        osg::Vec3d(-1.0, -1.0, ndcNear) * inverseViewProjMatrix,
                        osg::Vec3d(-1.0, -1.0, ndcFar) * inverseViewProjMatrix,
                        osg::Vec3d(-1.0, 1.0, ndcNear) * inverseViewProjMatrix,
                        osg::Vec3d(-1.0, 1.0, ndcFar) * inverseViewProjMatrix,
                        osg::Vec3d(1.0, -1.0, ndcNear) * inverseViewProjMatrix,
                        osg::Vec3d(1.0, -1.0, ndcFar) * inverseViewProjMatrix,
                        osg::Vec3d(1.0, 1.0, ndcNear) * inverseViewProjMatrix,
                        osg::Vec3d(1.0, 1.0, ndcFar) * inverseViewProjMatrix,
                    };
                    osg::Vec3d center(0.0, 0.0, 0.0);
                    for (uint32_t j = 0; j < 8; ++j)
                        center += worldSpace[j];
                    center *= 0.125;

                    double boundsSphereRadius = 0.0;
                    for (uint32_t j = 0; j < 8; ++j)
                        boundsSphereRadius = std::max(boundsSphereRadius, (center - worldSpace[j]).length());

                    osg::Vec3d eye = center - mDirectionalLight->getDirection() * lightFrustumNearOffset;
                    osg::Matrixd lightViewMatrix = osg::Matrixd::lookAt(eye, center, osg::Vec3d(0.0, 0.0, 1.0));

                    double minX = std::numeric_limits<double>::max();
                    double maxX = std::numeric_limits<double>::lowest();
                    double minY = std::numeric_limits<double>::max();
                    double maxY = std::numeric_limits<double>::lowest();
                    double minZ = std::numeric_limits<double>::max();
                    double maxZ = std::numeric_limits<double>::lowest();
                    for (uint32_t j = 0; j < 8; j++)
                    {
                        osg::Vec3d viewSpace = worldSpace[j] * lightViewMatrix;
                        minX = std::min(minX, viewSpace.x());
                        maxX = std::max(maxX, viewSpace.x());
                        minY = std::min(minY, viewSpace.y());
                        maxY = std::max(maxY, viewSpace.y());
                        minZ = std::min(minZ, viewSpace.z());
                        maxZ = std::max(maxZ, viewSpace.z());
                    }

                    double halfWidth = std::max(-minX, maxX);
                    double halfHeight = std::max(-minY, maxY);
                    double halfSize = std::max(halfWidth, halfHeight);

                    osg::Matrixd lightProjMatrix = osg::Matrixd::ortho(-halfSize, halfSize, -halfSize, halfSize, -maxZ, -minZ);
                    mShadowCameras[i]->setViewMatrix(lightViewMatrix);
                    mShadowCameras[i]->setProjectionMatrix(lightProjMatrix);

                    //float prevFrameDepth;
                    //{
                    //    double left, right, bottom, top, zNear, zFar;
                    //    mShadowCameras[i]->getProjectionMatrixAsOrtho(left, right, bottom, top, zNear, zFar);
                    //    // 用上一帧的zFar - zNear充当这一帧的光锥体深度
                    //    prevFrameDepth = zFar - zNear;
                    //}

                    cascadeNear = cascadeFar;
                }
            }
        };

        inline osg::StateSet* getShadowStateSet()
        {
            static osg::ref_ptr<osg::StateSet> stateSet = nullptr;
            if (stateSet)
                return stateSet;
            stateSet = new osg::StateSet;
            osg::Program* program = new osg::Program;
            program->addShader(osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR "Test/ShadowMap.vert.glsl"));
            program->addShader(osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR "Test/ShadowMap.frag.glsl"));
            stateSet->setAttribute(program, osg::StateAttribute::ON);
            stateSet->setDefine("SHADOW_CAST", "1");
            stateSet->setMode(GL_CULL_FACE, osg::StateAttribute::ON);
            return stateSet;
        }

        void addShadowPasses()
        {
            Pipeline* pipeline = Context::get().getEngine()->getPipeline();
            std::string namePrefix = std::string(sClass->getName()) + std::to_string(mLightIndex);
            Pipeline::Pass* gbufferPass = pipeline->getPass("GBuffer");
            uint32_t lightingPassIndex = pipeline->getPassIndex("Lighting");

            osg::ref_ptr<osg::Texture2DArray> shadowMapTextureArray = new osg::Texture2DArray;
            shadowMapTextureArray->setTextureSize(2048, 2048, 4);
            shadowMapTextureArray->setInternalFormat(GL_DEPTH_COMPONENT24);
            shadowMapTextureArray->setSourceFormat(GL_DEPTH_COMPONENT);
            shadowMapTextureArray->setFilter(osg::Texture::MIN_FILTER, osg::Texture::NEAREST);
            shadowMapTextureArray->setFilter(osg::Texture::MAG_FILTER, osg::Texture::NEAREST);

            using BufferType = Pipeline::Pass::BufferType;

            uint32_t shadowCastPassCount = 0;
            if (true)
            {
                shadowCastPassCount = 4;
                std::vector<osg::ref_ptr<osg::Camera>> shadowCameras;
                for (uint32_t i = 0; i < 4; ++i)
                {
                    Pipeline::Pass* shadowCastPass = pipeline->insertInputPass(lightingPassIndex + i, namePrefix + " ShadowCast" + std::to_string(i), SHADOW_CAST_MASK, GL_DEPTH_BUFFER_BIT, true, osg::Vec2(2048, 2048));
                    shadowCastPass->attach(BufferType::DEPTH_BUFFER, shadowMapTextureArray, 0, i);
                    shadowCastPass->getCamera()->setStateSet(getShadowStateSet());
                    shadowCastPass->getCamera()->setSmallFeatureCullingPixelSize(20);
                    shadowCameras.emplace_back(shadowCastPass->getCamera());
                }
                gbufferPass->getCamera()->addPreDrawCallback(new ShadowCamerasUpdateCallback(shadowCameras, this));
            }
            else
            {
                shadowCastPassCount = 1;
                Pipeline::Pass* shadowCastPass = pipeline->insertInputPass(lightingPassIndex, namePrefix + " ShadowCast", SHADOW_CAST_MASK, GL_DEPTH_BUFFER_BIT, true, osg::Vec2(2048, 2048));
                shadowCastPass->attach(BufferType::DEPTH_BUFFER, shadowMapTextureArray, 0, osg::Camera::FACE_CONTROLLED_BY_GEOMETRY_SHADER);
                shadowCastPass->getCamera()->setStateSet(getShadowStateSet());

            }

            osg::Program* shadowMaskProgram = new osg::Program;
            osg::Shader* screenQuadShader = osgDB::readShaderFile(osg::Shader::VERTEX, SHADER_DIR "Common/ScreenQuad.vert.glsl");
            osg::Shader* shadowMaskShader = osgDB::readShaderFile(osg::Shader::FRAGMENT, SHADER_DIR "Test/ShadowMask.frag.glsl");
            shadowMaskProgram->addShader(screenQuadShader);
            shadowMaskProgram->addShader(shadowMaskShader);
            Pipeline::Pass* shadowMaskPass = pipeline->insertWorkPass(lightingPassIndex + shadowCastPassCount, namePrefix + "ShadowMask", shadowMaskProgram, GL_COLOR_BUFFER_BIT);
            shadowMaskPass->attach(BufferType::COLOR_BUFFER0, GL_RGB8);
            shadowMaskPass->applyTexture(gbufferPass->getBufferTexture(BufferType::DEPTH_BUFFER), "uSceneDepthTexture", 0);
            shadowMaskPass->applyTexture(shadowMapTextureArray, "uShadowMapTextureArray", 1);
            shadowMaskPass->applyUniform(new osg::Uniform("uCascadeFar", osg::Vec4f(37.5, 112.5, 337.5, 1012.5)));
        }

        void removeShadowPasses()
        {
            /*Pipeline* pipeline = Context::get().getEngine()->getPipeline();
            std::string namePrefix = std::string(sClass->getName()) + std::to_string(mLightIndex);
            uint32_t lightingPassIndex = pipeline->getPassIndex(namePrefix);

            Pipeline::Pass* shadowCastPass = pipeline->getPass(lightingPassIndex - 2);
            Pipeline::Pass* shadowMaskPass = pipeline->getPass(lightingPassIndex - 1);
            pipeline->removePass(shadowCastPass);
            pipeline->removePass(shadowMaskPass);*/
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

        virtual void onEnable() override
        {
            if (!mImageCubemap)
                setImageCubemap(AssetManager::get().getAsset("Engine/Texture/TestCubemap")->getRootObjectSafety<TextureCubemap>());
            Texture2D* brdfLutTexture = AssetManager::get().getAsset("Engine/Texture/BRDFLut")->getRootObjectSafety<Texture2D>();
            Pipeline* pipeline = Context::get().getEngine()->getPipeline();
            Pipeline::Pass* lightingPass = pipeline->getPass("Lighting");
            osg::StateSet* lightingStateSet = lightingPass->getCamera()->getStateSet();
            lightingStateSet->getUniform("uEnableIBL")->set(true);
            lightingStateSet->setTextureAttribute(5, brdfLutTexture->getOsgTexture(), osg::StateAttribute::ON);
            lightingStateSet->setTextureAttribute(6, mSpecularCubemap->getOsgTexture(), osg::StateAttribute::ON);
            osg::Uniform* shCoeffUniform = lightingStateSet->getUniform("uSHCoeff");
            for (int i = 0; i < 9; ++i)
                shCoeffUniform->setElement(i, mDiffuseSHCoeff[i]);

            Pipeline::Pass* transparentPass = pipeline->getPass("Transparent");
            osg::StateSet* transparentStateSet = transparentPass->getCamera()->getStateSet();
            transparentStateSet->setTextureAttribute(14, brdfLutTexture->getOsgTexture(), osg::StateAttribute::ON);
            transparentStateSet->setTextureAttribute(15, mSpecularCubemap->getOsgTexture(), osg::StateAttribute::ON);
        }

        virtual void onDisable() override
        {
            Pipeline* pipeline = Context::get().getEngine()->getPipeline();
            Pipeline::Pass* lightingPass = pipeline->getPass("Lighting");
            osg::StateSet* lightingStateSet = lightingPass->getCamera()->getStateSet();
            lightingStateSet->getUniform("uEnableIBL")->set(false);

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
            Class* clazz = new TClass<Light, Component>("Light");
            return clazz;
        }

        template <> Type* Reflection::createType<DirectionalLight>()
        {
            Class* clazz = new TClass<DirectionalLight, Light>("DirectionalLight");
            clazz->addProperty("Intensity", &DirectionalLight::mIntensity);
            clazz->addProperty("Color", &DirectionalLight::mColor);
            clazz->addProperty("CastShadows", &DirectionalLight::mCastShadows);
            clazz->addProperty("Direction", &DirectionalLight::mDirection);
            clazz->addProperty("IsSunLight", &DirectionalLight::mIsSunLight);
            return clazz;
        }

        template <> Type* Reflection::createType<ImageBasedLight>()
        {
            Class* clazz = new TClass<ImageBasedLight, Light>("ImageBasedLight");
            return clazz;
        }
    }
}
