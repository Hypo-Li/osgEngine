#include "Pipeline.h"

namespace xxx
{
    Pipeline::Pass* Pipeline::addInputPass(const std::string& name, osg::Node::NodeMask cullMask, GLbitfield clearMask, bool fixedSize, osg::Vec2 sizeScale)
    {
        osg::Camera* camera = new osg::Camera;
        camera->setName(name);
        camera->setGraphicsContext(_graphicsContext);
        camera->setCullMask(cullMask);
        camera->setClearColor(osg::Vec4(0.0, 0.0, 0.0, 1.0));
        camera->setClearMask(clearMask);
        camera->setRenderOrder(osg::Camera::PRE_RENDER);
        camera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);
        camera->setImplicitBufferAttachmentMask(0, 0);
        if (fixedSize)
        {
            camera->setViewport(0, 0, sizeScale.x(), sizeScale.y());
            camera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
        }
        else
        {
            osg::Viewport* nativeViewport = _view->getCamera()->getViewport();
            camera->setViewport(nativeViewport->x(), nativeViewport->y(), nativeViewport->width() * sizeScale.x(), nativeViewport->height() * sizeScale.y());
        }
        _view->addSlave(camera, true);
        Pass* newPass = new Pass(camera, fixedSize, sizeScale);
        _passes.push_back(newPass);
        return newPass;
    }

    Pipeline::Pass* Pipeline::addWorkPass(const std::string& name, osg::Program* program, GLbitfield clearMask, bool fixedSize, osg::Vec2 sizeScale)
    {
        osg::Camera* camera = new osg::Camera;
        camera->setName(name);
        camera->setGraphicsContext(_graphicsContext);
        camera->setClearColor(osg::Vec4(0.0, 0.0, 0.0, 1.0));
        camera->setClearMask(clearMask);
        camera->setRenderOrder(osg::Camera::PRE_RENDER);
        camera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);
        camera->setImplicitBufferAttachmentMask(0, 0);
        camera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
        camera->setViewMatrix(osg::Matrixd::identity());
        camera->setProjectionMatrix(osg::Matrix::ortho2D(0.0, 1.0, 0.0, 1.0));
        camera->setCullingMode(osg::CullSettings::NO_CULLING);
        if (fixedSize)
        {
            camera->setViewport(0, 0, sizeScale.x(), sizeScale.y());
        }
        else
        {
            osg::Viewport* nativeViewport = _view->getCamera()->getViewport();
            camera->setViewport(nativeViewport->x(), nativeViewport->y(), nativeViewport->width() * sizeScale.x(), nativeViewport->height() * sizeScale.y());
        }
        osg::Geode* geode = new osg::Geode;
        geode->addDrawable(getScreenGeometry());
        for (uint32_t i = 0; i < program->getNumShaders(); i++)
            program->getShader(i)->setName(name);
        geode->getOrCreateStateSet()->setAttribute(program, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
        camera->addChild(geode);
        _view->addSlave(camera, false);
        Pass* newPass = new Pass(camera, fixedSize, sizeScale);
        _passes.push_back(newPass);
        return newPass;
    }

    Pipeline::Pass* Pipeline::addDisplayPass(const std::string& name, osg::Program* program)
    {
        osg::Camera* camera = new osg::Camera;
        camera->setName(name);
        camera->setGraphicsContext(_graphicsContext);
        camera->setClearColor(osg::Vec4(0.0, 0.0, 0.0, 1.0));
        camera->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        camera->setRenderOrder(osg::Camera::POST_RENDER);
        camera->setViewport(_view->getCamera()->getViewport());
        camera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
        camera->setViewMatrix(osg::Matrixd::identity());
        camera->setProjectionMatrix(osg::Matrix::ortho2D(0.0, 1.0, 0.0, 1.0));
        camera->setAllowEventFocus(false);
        osg::Geode* geode = new osg::Geode;
        geode->addDrawable(getScreenGeometry());
        for (uint32_t i = 0; i < program->getNumShaders(); i++)
            program->getShader(i)->setName(name);
        geode->getOrCreateStateSet()->setAttribute(program, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
        camera->addChild(geode);
        _view->addSlave(camera, false);
        Pass* newPass = new Pass(camera, false, osg::Vec2(1.0, 1.0));
        _passes.push_back(newPass);
        return newPass;
    }
}
