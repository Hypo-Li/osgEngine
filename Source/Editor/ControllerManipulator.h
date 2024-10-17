#pragma once
#include <osgGA/FirstPersonManipulator>

namespace xxx::editor
{
    class ControllerManipulator : public osgGA::FirstPersonManipulator
    {
    public:
        ControllerManipulator(float velocity = 1.0f) :
            osgGA::FirstPersonManipulator(),
            mVelocity(velocity)
        {
            mKeyMap[Forward] = false;
            mKeyMap[Backward] = false;
            mKeyMap[Left] = false;
            mKeyMap[Right] = false;
        }

        virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& us) override
        {
            if (ea.getHandled())
                return true;

            switch (ea.getEventType())
            {
            case osgGA::GUIEventAdapter::FRAME:
                if (mKeyMap[Forward]) moveForward(mVelocity);
                if (mKeyMap[Backward]) moveForward(-mVelocity);
                if (mKeyMap[Left]) moveRight(-mVelocity);
                if (mKeyMap[Right]) moveRight(mVelocity);
                return true;
            case osgGA::GUIEventAdapter::KEYDOWN:
                if (ea.getKey() == osgGA::GUIEventAdapter::KEY_Up)
                {
                    mKeyMap[Forward] = true;
                    return true;
                }
                else if (ea.getKey() == osgGA::GUIEventAdapter::KEY_Down)
                {
                    mKeyMap[Backward] = true;
                    return true;
                }
                else if (ea.getKey() == osgGA::GUIEventAdapter::KEY_Left)
                {
                    mKeyMap[Left] = true;
                    return true;
                }
                else if (ea.getKey() == osgGA::GUIEventAdapter::KEY_Right)
                {
                    mKeyMap[Right] = true;
                    return true;
                }
            case osgGA::GUIEventAdapter::KEYUP:
                if (ea.getKey() == osgGA::GUIEventAdapter::KEY_Up)
                {
                    mKeyMap[Forward] = false;
                    return true;
                }
                else if (ea.getKey() == osgGA::GUIEventAdapter::KEY_Down)
                {
                    mKeyMap[Backward] = false;
                    return true;
                }
                else if (ea.getKey() == osgGA::GUIEventAdapter::KEY_Left)
                {
                    mKeyMap[Left] = false;
                    return true;
                }
                else if (ea.getKey() == osgGA::GUIEventAdapter::KEY_Right)
                {
                    mKeyMap[Right] = false;
                    return true;
                }
            default:
                return FirstPersonManipulator::handle(ea, us);
            }
        }

        void setVelocity(float velocity)
        {
            mVelocity = velocity;
        }

    private:
        float mVelocity = 1.0f;
        enum MovingDirectoin
        {
            Forward = 0,
            Backward,
            Left,
            Right,
        };
        bool mKeyMap[4];
    };
}
