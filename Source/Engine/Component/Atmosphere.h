#pragma once
#include "Light.h"

#include <osg/DispatchCompute>
#include <osg/Texture2D>
#include <osg/Texture3D>
#include <osg/TextureCubemap>
#include <osg/BindImageTexture>
#include <osg/BufferIndexBinding>
#include <osgDB/ReadFile>

namespace xxx
{
    struct AtmosphereParameters
    {
        float groundRadius;
        float atmosphereHeight;
        osg::Vec3f mieScattCoeff;
        float mieScattScale;
        osg::Vec3f mieAbsorCoeff;
        float mieAbsorScale;
        osg::Vec3f rayScattCoeff;
        float rayScattScale;
        osg::Vec3f ozoAbsorCoeff;
        float ozoAbsorScale;
        float rayDensityH;
        float mieDensityH;
        float ozoneCenterHeight;
        float ozoneThickness;
        osg::Vec3f groundAlbedo;
    };

    struct AtmosphereUniformBuffer
    {
        
    };

    class Atmosphere : public Component
    {
        REFLECT_CLASS(Atmosphere)
    public:
        Atmosphere()
        {
            osg::ref_ptr<osg::FloatArray> atmosphereUniformBuffer = new osg::FloatArray(sizeof(AtmosphereUniformBuffer) / sizeof(float));
            osg::ref_ptr<osg::UniformBufferObject> atmosphereUBO = new osg::UniformBufferObject;
            // TODO: how to process uniform buffer binding index?
            mAtmosphereUBB = new osg::UniformBufferBinding(1, atmosphereUniformBuffer, 0, sizeof(AtmosphereUniformBuffer));

            updateAtmosphereUniformBuffer();

            // Transmittance Lut
            {
                mTransmittanceLutTexture = new osg::Texture2D;
                mTransmittanceLutTexture->setTextureSize(256, 64);
                mTransmittanceLutTexture->setInternalFormat(GL_RGBA16F);
                mTransmittanceLutTexture->setSourceFormat(GL_RGBA);
                mTransmittanceLutTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);

                mTransmittanceLutDispatch = new osg::DispatchCompute(8, 2, 1);
                osg::StateSet* stateSet = mTransmittanceLutDispatch->getOrCreateStateSet();
                osg::ref_ptr<osg::Program> program = new osg::Program;
                program->addShader(osgDB::readShaderFile(osg::Shader::COMPUTE, ""));
                stateSet->setAttribute(program, osg::StateAttribute::ON);
                osg::ref_ptr<osg::BindImageTexture> image = new osg::BindImageTexture(0, mTransmittanceLutTexture, osg::BindImageTexture::WRITE_ONLY, GL_RGBA16F);
                stateSet->setAttribute(image, osg::StateAttribute::ON);
                //stateSet->setAttribute(); UBB

                mTransmittanceLutDispatch->setCullingActive(false);
                mTransmittanceLutDispatch->setNodeMask(0x00000001);
                mOsgComponentGroup->addChild(mTransmittanceLutDispatch);
            }
            
            // MultiScattering Lut
            {
                mMultiScatteringLutTexture = new osg::Texture2D;
                mMultiScatteringLutTexture->setTextureSize(32, 32);
                mMultiScatteringLutTexture->setInternalFormat(GL_RGBA16F);
                mMultiScatteringLutTexture->setSourceFormat(GL_RGBA);
                mMultiScatteringLutTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);

                mMultiScatteringLutDispatch = new osg::DispatchCompute(32, 32, 1);
                osg::StateSet* stateSet = mMultiScatteringLutDispatch->getOrCreateStateSet();
                osg::ref_ptr<osg::Program> program = new osg::Program;
                program->addShader(osgDB::readShaderFile(osg::Shader::COMPUTE, ""));
                stateSet->setAttribute(program, osg::StateAttribute::ON);
                osg::ref_ptr<osg::BindImageTexture> image = new osg::BindImageTexture(0, mMultiScatteringLutTexture, osg::BindImageTexture::WRITE_ONLY, GL_RGBA16F);
                stateSet->setAttribute(image, osg::StateAttribute::ON);
                stateSet->addUniform(new osg::Uniform("uTransmittanceLutTexture", 0));
                stateSet->setTextureAttribute(0, mTransmittanceLutTexture, osg::StateAttribute::ON);
                //stateSet->setAttribute();

                mMultiScatteringLutDispatch->setCullingActive(false);
                mMultiScatteringLutDispatch->setNodeMask(0x00000001);
                mOsgComponentGroup->addChild(mMultiScatteringLutDispatch);
            }

            // AerialPerspective Lut
            {
                mAerialPerspectiveLutTexture = new osg::Texture3D;
                mAerialPerspectiveLutTexture->setTextureSize(32, 32, 32);
                mAerialPerspectiveLutTexture->setInternalFormat(GL_RGBA16F);
                mAerialPerspectiveLutTexture->setSourceFormat(GL_RGBA);
                mAerialPerspectiveLutTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);

                mAerialPerspectiveLutDispatch = new osg::DispatchCompute(2, 2, 8);
                osg::StateSet* stateSet = mAerialPerspectiveLutDispatch->getOrCreateStateSet();
                osg::ref_ptr<osg::Program> program = new osg::Program;
                program->addShader(osgDB::readShaderFile(osg::Shader::COMPUTE, ""));
                stateSet->setAttributeAndModes(program);
                osg::ref_ptr<osg::BindImageTexture> image = new osg::BindImageTexture(0, mAerialPerspectiveLutTexture, osg::BindImageTexture::WRITE_ONLY, GL_RGBA16F, 0, true);
                stateSet->setAttributeAndModes(image);
                stateSet->addUniform(new osg::Uniform("uTransmittanceLutTexture", 0));
                stateSet->setTextureAttribute(0, mTransmittanceLutTexture, osg::StateAttribute::ON);
                stateSet->addUniform(new osg::Uniform("uMultiScatteringLutTexture", 1));
                stateSet->setTextureAttribute(1, mMultiScatteringLutTexture, osg::StateAttribute::ON);
                //stateSet->setAttributeAndModes(CConfig::Get()->_viewDataUBB);
                //stateSet->setAttributeAndModes(CConfig::Get()->_atmosphereParametersUBB);

                mAerialPerspectiveLutDispatch->setCullingActive(false);
                mAerialPerspectiveLutDispatch->setNodeMask(0x00000001);
                mOsgComponentGroup->addChild(mAerialPerspectiveLutDispatch);
            }

            // SkyView Lut
            {
                mSkyViewLutTexture = new osg::Texture2D;
                mSkyViewLutTexture->setTextureSize(192, 108);
                mSkyViewLutTexture->setInternalFormat(GL_RGBA16F);
                mSkyViewLutTexture->setSourceFormat(GL_RGBA);
                mSkyViewLutTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);

                mSkyViewLutDispatch = new osg::DispatchCompute(6, 4, 1);
                osg::StateSet* stateSet = mSkyViewLutDispatch->getOrCreateStateSet();
                osg::ref_ptr<osg::Program> program = new osg::Program;
                program->addShader(osgDB::readShaderFile(osg::Shader::COMPUTE, ""));
                stateSet->setAttribute(program, osg::StateAttribute::ON);
                osg::ref_ptr<osg::BindImageTexture> image = new osg::BindImageTexture(0, mSkyViewLutTexture, osg::BindImageTexture::WRITE_ONLY, GL_RGBA16F);
                stateSet->setAttribute(image, osg::StateAttribute::ON);
                stateSet->addUniform(new osg::Uniform("uTransmittanceLutTexture", 0));
                stateSet->setTextureAttribute(0, mTransmittanceLutTexture, osg::StateAttribute::ON);
                stateSet->addUniform(new osg::Uniform("uMultiScatteringLutTexture", 1));
                stateSet->setTextureAttribute(1, mMultiScatteringLutTexture, osg::StateAttribute::ON);
                //stateSet->setAttributeAndModes(CConfig::Get()->_viewDataUBB);
                //stateSet->setAttributeAndModes(CConfig::Get()->_atmosphereParametersUBB);

                mSkyViewLutDispatch->setCullingActive(false);
                mSkyViewLutDispatch->setNodeMask(0x00000001);
                mOsgComponentGroup->addChild(mSkyViewLutDispatch);
            }
        }

        enum class CoordinateMode
        {
            Planet_Top_At_World_Origin,
            Planet_Center_At_World_Origin,
            Planet_Top_At_Translation,
            Planet_Center_At_Translation,
        };

        void setCoordinateMode(CoordinateMode coordinateMode)
        {
            mCoordinateMode = coordinateMode;
            mOsgComponentGroup->getOrCreateStateSet()->setDefine("COORDINATE_MODE", std::to_string(int64_t(coordinateMode)), osg::StateAttribute::ON);
        }

        virtual void postSerialize(Serializer* serializer)
        {
            if (serializer->isLoader())
            {
                updateAtmosphereUniformBuffer();
                setCoordinateMode(mCoordinateMode);
            }
        }

        void updateAtmosphereUniformBuffer()
        {
            osg::FloatArray* atmosphereUniformBuffer = static_cast<osg::FloatArray*>(mAtmosphereUBB->getBufferData());
            calcAtmosphereUniformBuffer(&mAtmosphereParameters, (AtmosphereUniformBuffer*)(atmosphereUniformBuffer->getDataPointer()));
            atmosphereUniformBuffer->dirty();
        }

    protected:
        AtmosphereParameters mAtmosphereParameters;
        CoordinateMode mCoordinateMode;

        osg::ref_ptr<osg::UniformBufferBinding> mAtmosphereUBB;

        osg::ref_ptr<osg::Texture2D> mTransmittanceLutTexture;
        osg::ref_ptr<osg::DispatchCompute> mTransmittanceLutDispatch;
        osg::ref_ptr<osg::Texture2D> mMultiScatteringLutTexture;
        osg::ref_ptr<osg::DispatchCompute> mMultiScatteringLutDispatch;
        osg::ref_ptr<osg::Texture3D> mAerialPerspectiveLutTexture;
        osg::ref_ptr<osg::DispatchCompute> mAerialPerspectiveLutDispatch;
        osg::ref_ptr<osg::Texture2D> mSkyViewLutTexture;
        osg::ref_ptr<osg::DispatchCompute> mSkyViewLutDispatch;

        osg::ref_ptr<osg::TextureCubeMap> mSkyLightCubemapTexture;
        osg::ref_ptr<osg::DispatchCompute> mSkyLightCubemapDispatch;

        void calcAtmosphereUniformBuffer(AtmosphereParameters* parameters, AtmosphereUniformBuffer* buffer)
        {

        }
    };

    namespace refl
    {
        template <> inline Type* Reflection::createType<AtmosphereParameters>()
        {
            Struct* structure = new StructInstance<AtmosphereParameters>("AtmosphereParameters");
            return structure;
        }

        template <> inline Type* Reflection::createType<Atmosphere::CoordinateMode>()
        {
            Enum* enumerate = new EnumInstance<Atmosphere::CoordinateMode>("Atmosphere::CoordinateMode", {
                {"Planet Top At World Origin", Atmosphere::CoordinateMode::Planet_Top_At_World_Origin},
                {"Planet Center At World Origin", Atmosphere::CoordinateMode::Planet_Center_At_World_Origin},
                {"Planet Top At Translation", Atmosphere::CoordinateMode::Planet_Top_At_Translation},
                {"Planet Center At Translation", Atmosphere::CoordinateMode::Planet_Center_At_Translation},
            });
            return enumerate;
        }

        template <> inline Type* Reflection::createType<Atmosphere>()
        {
            Class* clazz = new ClassInstance<Atmosphere, Component>("Atmosphere");
            clazz->addProperty("AtmosphereParameters", &Atmosphere::mAtmosphereParameters);
            clazz->addProperty("CoordinateMode", &Atmosphere::mCoordinateMode);
            return clazz;
        }
    }
}
