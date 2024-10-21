#pragma once
#include <osg/GraphicsContext>

namespace xxx
{
    void debugCallback(GLenum source, GLenum type, GLuint, GLenum severity,
    GLsizei, const GLchar* message, const void*)
    {
        std::string srcStr = "UNDEFINED";
        switch (source)

        {
        case GL_DEBUG_SOURCE_API:             srcStr = "API"; break;
        case GL_DEBUG_SOURCE_WINDOW_SYSTEM:   srcStr = "WINDOW_SYSTEM"; break;
        case GL_DEBUG_SOURCE_SHADER_COMPILER: srcStr = "SHADER_COMPILER"; break;
        case GL_DEBUG_SOURCE_THIRD_PARTY:     srcStr = "THIRD_PARTY"; break;
        case GL_DEBUG_SOURCE_APPLICATION:     srcStr = "APPLICATION"; break;
        case GL_DEBUG_SOURCE_OTHER:           srcStr = "OTHER"; break;
        }

        std::string typeStr = "UNDEFINED";

        osg::NotifySeverity osgSeverity = osg::DEBUG_INFO;
        switch (type)
        {

        case GL_DEBUG_TYPE_ERROR:
            //	__debugbreak();
            typeStr = "ERROR";
            osgSeverity = osg::FATAL;
            break;
        case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: typeStr = "DEPRECATED_BEHAVIOR"; break;
        case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR:  typeStr = "UNDEFINED_BEHAVIOR"; break;
        case GL_DEBUG_TYPE_PORTABILITY:         typeStr = "PORTABILITY"; break;
        case GL_DEBUG_TYPE_PERFORMANCE:         typeStr = "PERFORMANCE"; break;
        case GL_DEBUG_TYPE_OTHER:               typeStr = "OTHER"; break;
        }


        osg::notify(osgSeverity) << "OpenGL " << typeStr << " [" << srcStr << "]: " << message << std::endl;

    }

    void enableGLDebugExtension(int context_id)
    {
        //create the extensions
        PFNGLDEBUGMESSAGECONTROLPROC glDebugMessageControl = nullptr;
        PFNGLDEBUGMESSAGECALLBACKPROC glDebugMessageCallback = nullptr;
        if (osg::isGLExtensionSupported(context_id, "GL_KHR_debug"))
        {
            osg::setGLExtensionFuncPtr(glDebugMessageCallback, "glDebugMessageCallback");
            osg::setGLExtensionFuncPtr(glDebugMessageControl, "glDebugMessageControl");

        }
        else if (osg::isGLExtensionSupported(context_id, "GL_ARB_debug_output"))
        {
            osg::setGLExtensionFuncPtr(glDebugMessageCallback, "glDebugMessageCallbackARB");
            osg::setGLExtensionFuncPtr(glDebugMessageControl, "glDebugMessageControlARB");
        }
        else if (osg::isGLExtensionSupported(context_id, "GL_AMD_debug_output"))
        {
            osg::setGLExtensionFuncPtr(glDebugMessageCallback, "glDebugMessageCallbackAMD");
            osg::setGLExtensionFuncPtr(glDebugMessageControl, "glDebugMessageControlAMD");
        }

        if (glDebugMessageCallback != nullptr && glDebugMessageControl != nullptr)
        {
            glEnable(GL_DEBUG_OUTPUT);
            glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
            glDebugMessageControl(GL_DONT_CARE, GL_DONT_CARE, GL_DEBUG_SEVERITY_MEDIUM, 0, NULL, GL_TRUE);
            glDebugMessageCallback(debugCallback, nullptr);
        }
    }

    class EnableGLDebugOperation : public osg::GraphicsOperation
    {
    public:
        EnableGLDebugOperation()
            : osg::GraphicsOperation("EnableGLDebugOperation", false) {
        }
        virtual void operator ()(osg::GraphicsContext* gc) {
            OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_mutex);
            int context_id = gc->getState()->getContextID();
            enableGLDebugExtension(context_id);
        }
        OpenThreads::Mutex _mutex;
    };
}
