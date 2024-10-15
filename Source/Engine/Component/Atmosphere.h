#pragma once
#include "Light.h"

#include <osg/DispatchCompute>
#include <osg/Texture2D>
#include <osg/Texture3D>
#include <osg/TextureCubemap>
#include <osg/BindImageTexture>
#include <osgDB/ReadFile>

namespace xxx
{
    struct AtmosphereParameters
    {

    };

    class Atmosphere : public Component
    {
        REFLECT_CLASS(Atmosphere)
    public:
        Atmosphere()
        {
            // Transmittance Lut
            {
                mTransmittanceLutTexture = new osg::Texture2D;
                mTransmittanceLutTexture->setTextureSize(256, 64);
                mTransmittanceLutTexture->setInternalFormat(GL_RGBA16F_ARB);
                mTransmittanceLutTexture->setSourceFormat(GL_RGBA);
                mTransmittanceLutTexture->setFilter(osg::Texture::MIN_FILTER, osg::Texture::LINEAR);
                osg::ref_ptr<osg::BindImageTexture> transmittanceLutImage = new osg::BindImageTexture(0, mTransmittanceLutTexture, osg::BindImageTexture::WRITE_ONLY, GL_RGBA16F_ARB);
                osg::ref_ptr<osg::Program> transmittanceLutProgram = new osg::Program;
                transmittanceLutProgram->addShader(osgDB::readShaderFile(osg::Shader::COMPUTE, ""));
                mTransmittanceLutDispatch = new osg::DispatchCompute(8, 2, 1);
                mTransmittanceLutDispatch->getOrCreateStateSet()->setAttribute(transmittanceLutProgram, osg::StateAttribute::ON);
                mTransmittanceLutDispatch->getOrCreateStateSet()->setAttribute(transmittanceLutImage, osg::StateAttribute::ON);
                //mTransmittanceLutDispatch->getOrCreateStateSet()->setAttributeAndModes(); UBB
                mTransmittanceLutDispatch->setCullingActive(false);
                mTransmittanceLutDispatch->setNodeMask(0x00000001);
                mOsgComponentGroup->addChild(mTransmittanceLutDispatch);
            }
            
            // MultiScattering Lut
            {
                mMultiScatteringLutTexture = new osg::Texture2D;
            }
        }

    protected:
        AtmosphereParameters mAtmosphereParameters;

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
    };
}
