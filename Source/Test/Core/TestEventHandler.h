#pragma once
#include <Editor/ImGuiHandler.h>
#include <osgGA/GUIEventHandler>
#include <osgUtil/LineSegmentIntersector>
#include <iostream>

namespace xxx
{
    class TestEventHandler : public osgGA::GUIEventHandler
    {
        osg::ref_ptr<osg::Camera> _camera;
        osg::ref_ptr<ImGuiHandler> _imguiHandler;
    public:
        TestEventHandler(osg::Camera* camera, ImGuiHandler* imguiHandler) : _camera(camera), _imguiHandler(imguiHandler) {}

        virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
        {
            if (ea.getHandled())
                return false;
            if (ea.getEventType() == osgGA::GUIEventAdapter::RELEASE &&
                ea.getButton() == osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON)
            {
                osgViewer::Viewer* viewer = dynamic_cast<osgViewer::Viewer*>(aa.asView());
                const float x = ea.getX(), y = ea.getY();
                osg::Matrixd vpvMatrix; //Viewport * Projection * View
                vpvMatrix.postMult(viewer->getCamera()->getViewMatrix());
                vpvMatrix.postMult(_camera->getProjectionMatrix());
                vpvMatrix.postMult(_imguiHandler->getSceneViewViewportMatrix());

                osg::Matrixd inverse = osg::Matrixd::inverse(vpvMatrix);
                osg::Vec3d start = osg::Vec3d(x, y, 0.0) * inverse;
                osg::Vec3d end = osg::Vec3(x, y, 1.0) * inverse;

                osgUtil::LineSegmentIntersector* intersector = new osgUtil::LineSegmentIntersector(osgUtil::Intersector::MODEL, start, end);

                osgUtil::IntersectionVisitor iv(intersector);
                viewer->getSceneData()->accept(iv);

                const osgUtil::LineSegmentIntersector::Intersections& intersections = intersector->getIntersections();
                if (!intersections.empty())
                {
                    const osgUtil::LineSegmentIntersector::Intersection& intersection = *(intersections.begin());
                    osg::NodePath nodePath = intersection.nodePath;
                    if (!nodePath.empty())
                    {
                        for (auto ritr = nodePath.rbegin(); ritr != nodePath.rend(); ritr++)
                        {
                            xxx::Entity* entity = castNodeToEntity(*ritr);
                            if (entity)
                            {
                                std::cout << entity->getName() << std::endl;
                                break;
                            }
                        }
                    }
                }
            }
            return false;
        }
    };
}
