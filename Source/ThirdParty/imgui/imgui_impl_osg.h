#pragma once
#include "imgui.h"
#ifndef IMGUI_DISABLE

namespace osg
{
    class Camera;
}

namespace osgViewer
{
	class ViewerBase;
}

namespace osgGA
{
	class GUIEventAdapter;
	class GUIActionAdapter;
}

IMGUI_IMPL_API bool     ImGui_ImplOsg_Init(osg::Camera* camera);
//IMGUI_IMPL_API void     ImGui_ImplOsg_Shutdown();
IMGUI_IMPL_API void     ImGui_ImplOsg_NewFrame();

IMGUI_IMPL_API bool     ImGui_ImplOsg_Handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa, bool wantCaptureEvents);
#endif // #ifndef IMGUI_DISABLE
