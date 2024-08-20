#pragma once
#include <osgGA/FirstPersonManipulator>

class ControllerManipulator : public osgGA::FirstPersonManipulator
{
public:
    ControllerManipulator(float velocity = 1.0f) :
        osgGA::FirstPersonManipulator(),
        _velocity(velocity)
    {
        _keyMap[Forward] = false;
        _keyMap[Backward] = false;
        _keyMap[Left] = false;
        _keyMap[Right] = false;
    }

    virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& us) override
    {
        switch (ea.getEventType())
        {
        case osgGA::GUIEventAdapter::FRAME:
            if (_keyMap[Forward]) moveForward(_velocity);
            if (_keyMap[Backward]) moveForward(-_velocity);
            if (_keyMap[Left]) moveRight(-_velocity);
            if (_keyMap[Right]) moveRight(_velocity);
            return true;
        case osgGA::GUIEventAdapter::KEYDOWN:
            if (ea.getKey() == osgGA::GUIEventAdapter::KEY_Up)
            {
                _keyMap[Forward] = true;
                return true;
            }
            else if (ea.getKey() == osgGA::GUIEventAdapter::KEY_Down)
            {
                _keyMap[Backward] = true;
                return true;
            }
            else if (ea.getKey() == osgGA::GUIEventAdapter::KEY_Left)
            {
                _keyMap[Left] = true;
                return true;
            }
            else if (ea.getKey() == osgGA::GUIEventAdapter::KEY_Right)
            {
                _keyMap[Right] = true;
                return true;
            }
        case osgGA::GUIEventAdapter::KEYUP:
            if (ea.getKey() == osgGA::GUIEventAdapter::KEY_Up)
            {
                _keyMap[Forward] = false;
                return true;
            }
            else if (ea.getKey() == osgGA::GUIEventAdapter::KEY_Down)
            {
                _keyMap[Backward] = false;
                return true;
            }
            else if (ea.getKey() == osgGA::GUIEventAdapter::KEY_Left)
            {
                _keyMap[Left] = false;
                return true;
            }
            else if (ea.getKey() == osgGA::GUIEventAdapter::KEY_Right)
            {
                _keyMap[Right] = false;
                return true;
            }
        default:
            return FirstPersonManipulator::handle(ea, us);
        }
    }

    void setVelocity(float velocity)
    {
        _velocity = velocity;
    }

private:
    float _velocity;
    enum MovingDirectoin
    {
        Forward = 0,
        Backward,
        Left,
        Right,
    };
    bool _keyMap[4];
};
