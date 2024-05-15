#pragma once
#include <osgGA/GUIEventHandler>
#include <osgUtil/LineSegmentIntersector>
#include <iostream>

namespace xxx
{
    class TestEventHandler : public osgGA::GUIEventHandler
    {
        virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
        {
            if (ea.getHandled())
                return false;
            if (ea.getEventType() == osgGA::GUIEventAdapter::PUSH &&
                ea.getButton() == osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON)
            {
                osgViewer::Viewer* viewer = dynamic_cast<osgViewer::Viewer*>(aa.asView());
                osgUtil::LineSegmentIntersector::Intersections intersections;
                if (viewer->computeIntersections(ea.getX(), ea.getY(), intersections))
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

                /*float x = ea.getX();
                float y = ea.getY();
                osg::ref_ptr<osgUtil::LineSegmentIntersector> intersector = new osgUtil::LineSegmentIntersector(osgUtil::Intersector::WINDOW, x, y);
                osgUtil::IntersectionVisitor iv(intersector.get());
                aa.asView()->getCamera()->accept(iv);

                if (intersector->containsIntersections())
                {
                    const osgUtil::LineSegmentIntersector::Intersection& intersection = *(intersector->getIntersections().begin());
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
                }*/
            }
            return false;
        }
    };
}
