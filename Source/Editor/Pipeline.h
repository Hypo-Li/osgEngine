#pragma once
#include <osgViewer/Viewer>
#include <cassert>

namespace xxx
{
    class Pipeline : public osg::Referenced
    {
    public:
        Pipeline(osgViewer::Viewer* viewer)
        {
            _viewer = viewer;
            osg::Camera* nativeCamera = viewer->getCamera();
            _graphicsContext = nativeCamera->getGraphicsContext();
            nativeCamera->setGraphicsContext(nullptr);
            osg::Viewport* nativeViewport = nativeCamera->getViewport();
            int x = nativeViewport->x();
            int y = nativeViewport->y();
            int width = nativeViewport->width();
            int height = nativeViewport->height();
        }
        virtual ~Pipeline() = default;

        class Pass : public osg::Referenced
        {
            friend class Pipeline;
        public:
            Pass(osg::Camera* camera, double sizeScale = 1.0) : _camera(camera), _sizeScale(sizeScale) {}

        private:
            osg::ref_ptr<osg::Camera> _camera;
            double _sizeScale;
        };

        Pass* addInputPass(const char* name, osg::Node::NodeMask cullMask, GLbitfield clearMask = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
        {
            osg::Camera* camera = new osg::Camera;
            camera->setName(name);
            camera->setGraphicsContext(_graphicsContext);
            camera->setCullMask(cullMask);
            camera->setClearColor(osg::Vec4(0.0, 0.0, 0.0, 1.0));
            camera->setClearMask(clearMask);
            camera->setRenderOrder(osg::Camera::PRE_RENDER);
            camera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);
            osg::Viewport* nativeViewport = _viewer->getCamera()->getViewport();
            int x = nativeViewport->x();
            int y = nativeViewport->y();
            int width = nativeViewport->width();
            int height = nativeViewport->height();
            camera->setViewport(x, y, width, height);
            camera->setImplicitBufferAttachmentMask(0, 0);
            _viewer->addSlave(camera, true);
            Pass* newPass = new Pass(camera);
            _passes.push_back(newPass);
            return newPass;
        }

        Pass* addWorkPass(const char* name, osg::Program* program, double sizeScale = 1.0, GLbitfield clearMask = GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT)
        {
            osg::Camera* camera = new osg::Camera;
            camera->setName(name);
            camera->setGraphicsContext(_graphicsContext);
            camera->setClearColor(osg::Vec4(0.0, 0.0, 0.0, 1.0));
            camera->setClearMask(clearMask);
            camera->setRenderOrder(osg::Camera::PRE_RENDER);
            camera->setRenderTargetImplementation(osg::Camera::FRAME_BUFFER_OBJECT);
            osg::Viewport* nativeViewport = _viewer->getCamera()->getViewport();
            int x = nativeViewport->x();
            int y = nativeViewport->y();
            int width = nativeViewport->width();
            int height = nativeViewport->height();
            camera->setViewport(x, y, width * sizeScale, height * sizeScale);
            camera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
            camera->setViewMatrix(osg::Matrixd::identity());
            camera->setProjectionMatrix(osg::Matrix::ortho2D(0.0, 1.0, 0.0, 1.0));
            camera->setImplicitBufferAttachmentMask(0, 0);
            osg::Geode* geode = new osg::Geode;
            geode->addDrawable(getScreenGeometry());
            for (uint32_t i = 0; i < program->getNumShaders(); i++)
                program->getShader(i)->setName(name);
            geode->getOrCreateStateSet()->setAttribute(program, osg::StateAttribute::ON | osg::StateAttribute::OVERRIDE);
            camera->addChild(geode);
            _viewer->addSlave(camera, false);
            Pass* newPass = new Pass(camera, sizeScale);
            _passes.push_back(newPass);
            return newPass;
        }

        Pass* addFinalPass(const char* name, osg::Program* program)
        {
            osg::Camera* camera = new osg::Camera;
            camera->setName(name);
            camera->setGraphicsContext(_graphicsContext);
            camera->setClearColor(osg::Vec4(0.0, 0.0, 0.0, 1.0));
            camera->setClearMask(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            camera->setRenderOrder(osg::Camera::POST_RENDER);
            camera->setViewport(_viewer->getCamera()->getViewport());
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
            _viewer->addSlave(camera, false);
            Pass* newPass = new Pass(camera);
            _passes.push_back(newPass);
            return newPass;
        }

        // Set all cameras's viewport except for final camera
        void setViewport(int x, int y, int width, int height)
        {
            for (Pass* pass : _passes)
            {
                if (pass->_camera->getRenderTargetImplementation() == osg::Camera::FRAME_BUFFER_OBJECT)
                {
                    double sizeScale = pass->_sizeScale;
                    pass->_camera->setViewport(x * sizeScale, y * sizeScale, width * sizeScale, height * sizeScale);
                }
            }
            double newAspect = double(width) / double(height);
            double fovy, aspect, zNear, zFar;
            _viewer->getCamera()->setProjectionMatrixAsPerspective(fovy, newAspect, zNear, zFar);
        }

    private:
        osg::ref_ptr<osgViewer::Viewer> _viewer;
        osg::ref_ptr<osg::GraphicsContext> _graphicsContext;
        std::vector<osg::ref_ptr<Pass>> _passes;

        osg::Geometry* Pipeline::getScreenGeometry()
        {
            static osg::ref_ptr<osg::Geometry> geometry = nullptr;
            if (geometry) return geometry;
            geometry = new osg::Geometry;
            geometry->setUseDisplayList(false);
            geometry->setUseVertexBufferObjects(true);
            osg::Vec3Array* positions = new osg::Vec3Array();
            positions->push_back({ -1.f, 3.f, 0.f });
            positions->push_back({ -1.f, -1.f, 0.f });
            positions->push_back({ 3.f, -1.f, 0.f });
            geometry->setVertexAttribArray(0, positions);
            geometry->setVertexAttribBinding(0, osg::Geometry::BIND_PER_VERTEX);
            geometry->addPrimitiveSet(new osg::DrawArrays(GL_TRIANGLES, 0, 3));
            geometry->setCullingActive(false);
            return geometry;
        }
    };
}
