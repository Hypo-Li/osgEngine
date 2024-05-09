#include "imgui.h"
#ifndef IMGUI_DISABLE
#include "imgui_impl_osg.h"
// OSG
#include <osgViewer/Viewer>
#include <osgGA/GUIEventHandler>

struct ImGui_ImplOsg_Data
{
	osgViewer::Viewer* Viewer;
	osgViewer::GraphicsWindow* Window;
	double Time;
	osgViewer::GraphicsWindow::MouseCursor MouseCursors[ImGuiMouseCursor_COUNT];
	ImGuiMouseCursor CurrentCursor;
};

ImGui_ImplOsg_Data* ImGui_ImplOsg_GetBackendData()
{
	return ImGui::GetCurrentContext() ? (ImGui_ImplOsg_Data*)ImGui::GetIO().BackendPlatformUserData : nullptr;
}

bool ImGui_ImplOsg_Init(osgViewer::Viewer* viewer)
{
	ImGuiIO& io = ImGui::GetIO();
	IM_ASSERT(io.BackendPlatformUserData == nullptr && "Already initialized a platform backend!");

	ImGui_ImplOsg_Data* bd = IM_NEW(ImGui_ImplOsg_Data)();
	io.BackendPlatformUserData = (void*)bd;
	io.BackendPlatformName = "imgui_impl_osg";
	io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
	io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;
	
	bd->Viewer = viewer;
	bd->Window = dynamic_cast<osgViewer::GraphicsWindow*>(bd->Viewer->getCamera()->getGraphicsContext());
	IM_ASSERT(bd->Window != nullptr);
	bd->Time = 0.0;

	using MouseCursor = osgViewer::GraphicsWindow::MouseCursor;
	bd->MouseCursors[ImGuiMouseCursor_Arrow] = MouseCursor::LeftArrowCursor;
	bd->MouseCursors[ImGuiMouseCursor_TextInput] = MouseCursor::TextCursor;
	bd->MouseCursors[ImGuiMouseCursor_ResizeAll] = MouseCursor::LeftArrowCursor;
	bd->MouseCursors[ImGuiMouseCursor_ResizeNS] = MouseCursor::UpDownCursor;
	bd->MouseCursors[ImGuiMouseCursor_ResizeEW] = MouseCursor::LeftRightCursor;
	bd->MouseCursors[ImGuiMouseCursor_ResizeNESW] = MouseCursor::LeftArrowCursor;
	bd->MouseCursors[ImGuiMouseCursor_ResizeNWSE] = MouseCursor::LeftArrowCursor;
	bd->MouseCursors[ImGuiMouseCursor_Hand] = MouseCursor::HandCursor;
	bd->MouseCursors[ImGuiMouseCursor_NotAllowed] = MouseCursor::LeftArrowCursor;

	bd->CurrentCursor = ImGuiMouseCursor_Arrow;
	return true;
}

static void ImGui_ImplOsg_UpdateMouseCursor()
{
	ImGuiIO& io = ImGui::GetIO();
	ImGui_ImplOsg_Data* bd = ImGui_ImplOsg_GetBackendData();
	if ((io.ConfigFlags & ImGuiConfigFlags_NoMouseCursorChange))
		return;

	ImGuiMouseCursor imgui_cursor = ImGui::GetMouseCursor();
	if (bd->CurrentCursor == imgui_cursor)
		return;

	if (imgui_cursor != ImGuiMouseCursor_None)
	{
		bd->CurrentCursor = imgui_cursor;
		bd->Window->setCursor(bd->MouseCursors[imgui_cursor]);
	}
}

void ImGui_ImplOsg_NewFrame()
{
	ImGuiIO& io = ImGui::GetIO();
	ImGui_ImplOsg_Data* bd = ImGui_ImplOsg_GetBackendData();

	osg::Viewport* viewport = bd->Viewer->getCamera()->getViewport();
	io.DisplaySize = ImVec2(viewport->width(), viewport->height());

	double current_time = bd->Viewer->getFrameStamp()->getSimulationTime();
	if (current_time <= bd->Time)
		current_time = bd->Time + 0.0000001;
	io.DeltaTime = bd->Time > 0.0 ? (float)(current_time - bd->Time) : (float)(1.0f / 60.0f);
	bd->Time = current_time;

	ImGui_ImplOsg_UpdateMouseCursor();
}

static ImGuiKey ImGui_ImplOsg_KeyToImGuiKey(int key)
{
	using KEY = osgGA::GUIEventAdapter::KeySymbol;
	switch (key)
	{
	case KEY::KEY_Tab: return ImGuiKey_Tab;
	case KEY::KEY_Left: return ImGuiKey_LeftArrow;
	case KEY::KEY_Right: return ImGuiKey_RightArrow;
	case KEY::KEY_Up: return ImGuiKey_UpArrow;
	case KEY::KEY_Down: return ImGuiKey_DownArrow;
	case KEY::KEY_Page_Up: return ImGuiKey_PageUp;
	case KEY::KEY_Page_Down: return ImGuiKey_PageDown;
	case KEY::KEY_Control_L: return ImGuiMod_Ctrl;
	case KEY::KEY_C: return ImGuiKey_C;
	case KEY::KEY_V: return ImGuiKey_V;
	case KEY::KEY_A: return ImGuiKey_A;
	case KEY::KEY_Z: return ImGuiKey_Z;
	case 1: return ImGuiKey_A;
	default: return ImGuiKey_None;
	}
}

static int ConvertFromOSGKey(int key)
{
	using KEY = osgGA::GUIEventAdapter::KeySymbol;

	switch (key)
	{
	case KEY::KEY_Tab:
		return ImGuiKey_Tab;
	case KEY::KEY_Left:
		return ImGuiKey_LeftArrow;
	case KEY::KEY_Right:
		return ImGuiKey_RightArrow;
	case KEY::KEY_Up:
		return ImGuiKey_UpArrow;
	case KEY::KEY_Down:
		return ImGuiKey_DownArrow;
	case KEY::KEY_Page_Up:
		return ImGuiKey_PageUp;
	case KEY::KEY_Page_Down:
		return ImGuiKey_PageDown;
	case KEY::KEY_Home:
		return ImGuiKey_Home;
	case KEY::KEY_End:
		return ImGuiKey_End;
	case KEY::KEY_Delete:
		return ImGuiKey_Delete;
	case KEY::KEY_BackSpace:
		return ImGuiKey_Backspace;
	case KEY::KEY_Return:
		return ImGuiKey_Enter;
	case KEY::KEY_Escape:
		return ImGuiKey_Escape;
	default: // Not found
		return -1;
	}
}

bool ImGui_ImplOsg_Handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
{
	ImGuiIO& io = ImGui::GetIO();
	const bool wantCaptureMouse = io.WantCaptureMouse;
	const bool wantCaptureKeyboard = io.WantCaptureKeyboard;

	switch (ea.getEventType())
	{
	case osgGA::GUIEventAdapter::KEYDOWN:
	case osgGA::GUIEventAdapter::KEYUP:
	{
		const bool isKeyDown = ea.getEventType() == osgGA::GUIEventAdapter::KEYDOWN;
		/*int key = ea.getKey();

		io.AddKeyEvent(ImGuiMod_Ctrl, ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_CTRL);
		io.AddKeyEvent(ImGuiMod_Shift, ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_SHIFT);
		io.AddKeyEvent(ImGuiMod_Alt, ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_ALT);
		io.AddKeyEvent(ImGuiMod_Super, ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_SUPER);
		
		if (isKeyDown && key > 0 && key < 0xFF)
		{
			io.AddInputCharacter((unsigned short)key);
		}*/

		io.AddKeyEvent(ImGuiMod_Ctrl, ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_CTRL);
		io.AddKeyEvent(ImGuiMod_Shift, ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_SHIFT);
		io.AddKeyEvent(ImGuiMod_Alt, ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_ALT);
		io.AddKeyEvent(ImGuiMod_Super, ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_SUPER);

		int osgKey = ea.getKey();
		if (isKeyDown && osgKey > 0 && osgKey < 0xFF)
		{
			io.AddInputCharacter((unsigned short)osgKey);
		}

		ImGuiKey imguiKey = ImGui_ImplOsg_KeyToImGuiKey(osgKey);
		if (imguiKey != ImGuiKey_None)
			io.AddKeyEvent(imguiKey, isKeyDown);
		return wantCaptureKeyboard;
	}
	case (osgGA::GUIEventAdapter::RELEASE):
	case (osgGA::GUIEventAdapter::PUSH):
	{
		//io.MousePos = ImVec2(ea.getX(), io.DisplaySize.y - ea.getY());
		io.AddMousePosEvent(ea.getX(), io.DisplaySize.y - ea.getY());
		bool isButtonDown = ea.getEventType() == osgGA::GUIEventAdapter::PUSH;
		if (ea.getButton() == osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON)
			io.AddMouseButtonEvent(ImGuiMouseButton_Left, isButtonDown);
		if (ea.getButton() == osgGA::GUIEventAdapter::RIGHT_MOUSE_BUTTON)
			io.AddMouseButtonEvent(ImGuiMouseButton_Right, isButtonDown);
		if (ea.getButton() == osgGA::GUIEventAdapter::MIDDLE_MOUSE_BUTTON)
			io.AddMouseButtonEvent(ImGuiMouseButton_Middle, isButtonDown);
		return wantCaptureMouse;
	}
	case (osgGA::GUIEventAdapter::DRAG):
	case (osgGA::GUIEventAdapter::MOVE):
	{
		//io.MousePos = ImVec2(ea.getX(), io.DisplaySize.y - ea.getY());
		io.AddMousePosEvent(ea.getX(), io.DisplaySize.y - ea.getY());
		return wantCaptureMouse;
	}
	case (osgGA::GUIEventAdapter::SCROLL):
	{
		io.AddMouseWheelEvent(0.0, ea.getScrollingMotion() == osgGA::GUIEventAdapter::SCROLL_UP ? 1.0 : -1.0);
		return wantCaptureMouse;
	}
	default: return false;
	}

	return false;
}

#endif // #ifndef IMGUI_DISABLE
