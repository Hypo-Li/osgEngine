#pragma once
#include <Engine/Core/Context.h>

#include <osgGA/GUIEventHandler>
#include <osgUtil/LineSegmentIntersector>
#include <osgViewer/Viewer>

namespace xxx::editor
{
    class PickEventHandler : public osgGA::GUIEventHandler
    {
    public:
        PickEventHandler(osg::Camera* camera, osg::Viewport* displayViewport) :
            mCamera(camera),
            mDisplayViewport(displayViewport)
        {

        }

        virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa) override
        {
            if (ea.getHandled())
                return false;
            static bool wantPick = false;
            if (ea.getEventType() & osgGA::GUIEventAdapter::PUSH &&
                ea.getButton() == osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON)
            {
                wantPick = true;
            }
            if (ea.getEventType() & osgGA::GUIEventAdapter::DRAG)
            {
                wantPick = false;
            }
            if (ea.getEventType() & osgGA::GUIEventAdapter::RELEASE &&
                ea.getButton() == osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON && wantPick)
            {
                wantPick = false;
                osgViewer::View* view = dynamic_cast<osgViewer::View*>(aa.asView());
                const float x = ea.getX(), y = ea.getY();
                osg::Matrixd vpvMatrix; //Viewport * Projection * View
                vpvMatrix.postMult(mCamera->getViewMatrix());
                vpvMatrix.postMult(mCamera->getProjectionMatrix());
                vpvMatrix.postMult(mDisplayViewport->computeWindowMatrix());

                osg::Matrixd inverse = osg::Matrixd::inverse(vpvMatrix);
                osg::Vec3d start = osg::Vec3d(x, y, 0.0) * inverse;
                osg::Vec3d end = osg::Vec3(x, y, 1.0) * inverse;

                osgUtil::LineSegmentIntersector* intersector = new osgUtil::LineSegmentIntersector(osgUtil::Intersector::MODEL, start, end);

                osgUtil::IntersectionVisitor iv(intersector);
                view->getSceneData()->accept(iv);

                const osgUtil::LineSegmentIntersector::Intersections& intersections = intersector->getIntersections();
                if (!intersections.empty())
                {
                    const osgUtil::LineSegmentIntersector::Intersection& intersection = *(intersections.begin());
                    osg::NodePath nodePath = intersection.nodePath;
                    if (!nodePath.empty())
                    {
                        for (auto ritr = nodePath.rbegin(); ritr != nodePath.rend(); ritr++)
                        {
                            EntityNode* entityNode = dynamic_cast<EntityNode*>(*ritr);
                            if (entityNode)
                            {
                                Context::get().setActivedEntity(entityNode->getEntity());
                                return true;
                            }
                        }
                    }
                }
                Context::get().setActivedEntity(nullptr);
            }
            return false;
        }

    protected:
        osg::ref_ptr<osg::Camera> mCamera;
        osg::ref_ptr<osg::Viewport> mDisplayViewport;

    };
}
