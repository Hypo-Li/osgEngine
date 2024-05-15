#include "imgui.h"
#ifndef IMGUI_DISABLE
#include "imgui_impl_osg.h"
// OSG
#include <osgViewer/Viewer>
#include <osgGA/GUIEventHandler>
#include <iostream>

struct ImGui_ImplOsg_Data
{
	osgViewer::Viewer* Viewer;
    osg::Camera* Camera;
	osgViewer::GraphicsWindow* Window;
	double Time;
	osgViewer::GraphicsWindow::MouseCursor MouseCursors[ImGuiMouseCursor_COUNT];
	ImGuiMouseCursor CurrentCursor;
};

ImGui_ImplOsg_Data* ImGui_ImplOsg_GetBackendData()
{
	return ImGui::GetCurrentContext() ? (ImGui_ImplOsg_Data*)ImGui::GetIO().BackendPlatformUserData : nullptr;
}

bool ImGui_ImplOsg_Init(osgViewer::Viewer* viewer, osg::Camera* camera)
{
	ImGuiIO& io = ImGui::GetIO();
	IM_ASSERT(io.BackendPlatformUserData == nullptr && "Already initialized a platform backend!");

	ImGui_ImplOsg_Data* bd = IM_NEW(ImGui_ImplOsg_Data)();
	io.BackendPlatformUserData = (void*)bd;
	io.BackendPlatformName = "imgui_impl_osg";
	io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors;
	io.BackendFlags |= ImGuiBackendFlags_HasSetMousePos;
	
	bd->Viewer = viewer;
    bd->Camera = camera;
	bd->Window = dynamic_cast<osgViewer::GraphicsWindow*>(camera->getGraphicsContext());
	IM_ASSERT(bd->Window != nullptr);
	bd->Time = 0.0;

	using MouseCursor = osgViewer::GraphicsWindow::MouseCursor;
	bd->MouseCursors[ImGuiMouseCursor_Arrow] = MouseCursor::LeftArrowCursor;
	bd->MouseCursors[ImGuiMouseCursor_TextInput] = MouseCursor::TextCursor;
	bd->MouseCursors[ImGuiMouseCursor_ResizeAll] = MouseCursor::LeftArrowCursor;
	bd->MouseCursors[ImGuiMouseCursor_ResizeNS] = MouseCursor::UpDownCursor;
	bd->MouseCursors[ImGuiMouseCursor_ResizeEW] = MouseCursor::LeftRightCursor;
	bd->MouseCursors[ImGuiMouseCursor_ResizeNESW] = MouseCursor::TopRightCorner;
	bd->MouseCursors[ImGuiMouseCursor_ResizeNWSE] = MouseCursor::TopLeftCorner;
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

	osg::Viewport* viewport = bd->Camera->getViewport();
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
    case KEY::KEY_Home: return ImGuiKey_Home;
    case KEY::KEY_End: return ImGuiKey_End;
    case KEY::KEY_Insert: return ImGuiKey_Insert;
    case KEY::KEY_Delete: return ImGuiKey_Delete;
    case KEY::KEY_BackSpace: return ImGuiKey_Backspace;
    case KEY::KEY_Space: return ImGuiKey_Space;
    case KEY::KEY_Return: return ImGuiKey_Enter;
    case KEY::KEY_Escape: return ImGuiKey_Escape;
    case KEY::KEY_Quote: return ImGuiKey_Apostrophe;            // '
    case KEY::KEY_Comma: return ImGuiKey_Comma;                 // ,
    case KEY::KEY_Minus: return ImGuiKey_Minus;                 // -
    case KEY::KEY_Period: return ImGuiKey_Period;               // .
    case KEY::KEY_Slash: return ImGuiKey_Slash;                 // /
    case KEY::KEY_Semicolon: return ImGuiKey_Semicolon;         // ;
    case KEY::KEY_Equals: return ImGuiKey_Equal;                // =
    case KEY::KEY_Leftbracket: return ImGuiKey_LeftBracket;     // [
    case KEY::KEY_Backslash: return ImGuiKey_Backslash;         // \ 
    case KEY::KEY_Rightbracket: return ImGuiKey_RightBracket;   // ]
    case KEY::KEY_Backquote: return ImGuiKey_GraveAccent;       // `
    case KEY::KEY_Caps_Lock: return ImGuiKey_CapsLock;
    case KEY::KEY_Scroll_Lock: return ImGuiKey_ScrollLock;
    case KEY::KEY_Num_Lock: return ImGuiKey_NumLock;
    case KEY::KEY_Print: return ImGuiKey_PrintScreen;
    case KEY::KEY_Pause: return ImGuiKey_Pause;
    case KEY::KEY_KP_0: return ImGuiKey_Keypad0;
    case KEY::KEY_KP_1: return ImGuiKey_Keypad1;
    case KEY::KEY_KP_2: return ImGuiKey_Keypad2;
    case KEY::KEY_KP_3: return ImGuiKey_Keypad3;
    case KEY::KEY_KP_4: return ImGuiKey_Keypad4;
    case KEY::KEY_KP_5: return ImGuiKey_Keypad5;
    case KEY::KEY_KP_6: return ImGuiKey_Keypad6;
    case KEY::KEY_KP_7: return ImGuiKey_Keypad7;
    case KEY::KEY_KP_8: return ImGuiKey_Keypad8;
    case KEY::KEY_KP_9: return ImGuiKey_Keypad9;
    case KEY::KEY_KP_Decimal: return ImGuiKey_KeypadDecimal;
    case KEY::KEY_KP_Divide: return ImGuiKey_KeypadDivide;
    case KEY::KEY_KP_Multiply: return ImGuiKey_KeypadMultiply;
    case KEY::KEY_KP_Subtract: return ImGuiKey_KeypadSubtract;
    case KEY::KEY_KP_Add: return ImGuiKey_KeypadAdd;
    case KEY::KEY_KP_Enter: return ImGuiKey_KeypadEnter;
    case KEY::KEY_KP_Equal: return ImGuiKey_KeypadEqual;
    case KEY::KEY_Shift_L: return ImGuiKey_LeftShift;
    case KEY::KEY_Control_L: return ImGuiKey_LeftCtrl;
    case KEY::KEY_Alt_L: return ImGuiKey_LeftAlt;
    case KEY::KEY_Super_L: return ImGuiKey_LeftSuper;
    case KEY::KEY_Shift_R: return ImGuiKey_RightShift;
    case KEY::KEY_Control_R: return ImGuiKey_RightCtrl;
    case KEY::KEY_Alt_R: return ImGuiKey_RightAlt;
    case KEY::KEY_Super_R: return ImGuiKey_RightSuper;
    case KEY::KEY_Menu: return ImGuiKey_Menu;
    case KEY::KEY_0: return ImGuiKey_0;
    case KEY::KEY_1: return ImGuiKey_1;
    case KEY::KEY_2: return ImGuiKey_2;
    case KEY::KEY_3: return ImGuiKey_3;
    case KEY::KEY_4: return ImGuiKey_4;
    case KEY::KEY_5: return ImGuiKey_5;
    case KEY::KEY_6: return ImGuiKey_6;
    case KEY::KEY_7: return ImGuiKey_7;
    case KEY::KEY_8: return ImGuiKey_8;
    case KEY::KEY_9: return ImGuiKey_9;
    case KEY::KEY_A: return ImGuiKey_A;
    case KEY::KEY_B: return ImGuiKey_B;
    case KEY::KEY_C: return ImGuiKey_C;
    case KEY::KEY_D: return ImGuiKey_D;
    case KEY::KEY_E: return ImGuiKey_E;
    case KEY::KEY_F: return ImGuiKey_F;
    case KEY::KEY_G: return ImGuiKey_G;
    case KEY::KEY_H: return ImGuiKey_H;
    case KEY::KEY_I: return ImGuiKey_I;
    case KEY::KEY_J: return ImGuiKey_J;
    case KEY::KEY_K: return ImGuiKey_K;
    case KEY::KEY_L: return ImGuiKey_L;
    case KEY::KEY_M: return ImGuiKey_M;
    case KEY::KEY_N: return ImGuiKey_N;
    case KEY::KEY_O: return ImGuiKey_O;
    case KEY::KEY_P: return ImGuiKey_P;
    case KEY::KEY_Q: return ImGuiKey_Q;
    case KEY::KEY_R: return ImGuiKey_R;
    case KEY::KEY_S: return ImGuiKey_S;
    case KEY::KEY_T: return ImGuiKey_T;
    case KEY::KEY_U: return ImGuiKey_U;
    case KEY::KEY_V: return ImGuiKey_V;
    case KEY::KEY_W: return ImGuiKey_W;
    case KEY::KEY_X: return ImGuiKey_X;
    case KEY::KEY_Y: return ImGuiKey_Y;
    case KEY::KEY_Z: return ImGuiKey_Z;

    case 'A': return ImGuiKey_A;
    case 'B': return ImGuiKey_B;
    case 'C': return ImGuiKey_C;
    case 'D': return ImGuiKey_D;
    case 'E': return ImGuiKey_E;
    case 'F': return ImGuiKey_F;
    case 'G': return ImGuiKey_G;
    case 'H': return ImGuiKey_H;
    case 'I': return ImGuiKey_I;
    case 'J': return ImGuiKey_J;
    case 'K': return ImGuiKey_K;
    case 'L': return ImGuiKey_L;
    case 'M': return ImGuiKey_M;
    case 'N': return ImGuiKey_N;
    case 'O': return ImGuiKey_O;
    case 'P': return ImGuiKey_P;
    case 'Q': return ImGuiKey_Q;
    case 'R': return ImGuiKey_R;
    case 'S': return ImGuiKey_S;
    case 'T': return ImGuiKey_T;
    case 'U': return ImGuiKey_U;
    case 'V': return ImGuiKey_V;
    case 'W': return ImGuiKey_W;
    case 'X': return ImGuiKey_X;
    case 'Y': return ImGuiKey_Y;
    case 'Z': return ImGuiKey_Z;

    case 1:  return ImGuiKey_A;
    case 2:  return ImGuiKey_B;
    case 3:  return ImGuiKey_C;
    case 4:  return ImGuiKey_D;
    case 5:  return ImGuiKey_E;
    case 6:  return ImGuiKey_F;
    case 7:  return ImGuiKey_G;
    case 8:  return ImGuiKey_H;
    case 9:  return ImGuiKey_I;
    case 10: return ImGuiKey_J;
    case 11: return ImGuiKey_K;
    case 12: return ImGuiKey_L;
    case 13: return ImGuiKey_M;
    case 14: return ImGuiKey_N;
    case 15: return ImGuiKey_O;
    case 16: return ImGuiKey_P;
    case 17: return ImGuiKey_Q;
    case 18: return ImGuiKey_R;
    case 19: return ImGuiKey_S;
    case 20: return ImGuiKey_T;
    case 21: return ImGuiKey_U;
    case 22: return ImGuiKey_V;
    case 23: return ImGuiKey_W;
    case 24: return ImGuiKey_X;
    case 25: return ImGuiKey_Y;
    case 26: return ImGuiKey_Z;

    case KEY::KEY_F1: return ImGuiKey_F1;
    case KEY::KEY_F2: return ImGuiKey_F2;
    case KEY::KEY_F3: return ImGuiKey_F3;
    case KEY::KEY_F4: return ImGuiKey_F4;
    case KEY::KEY_F5: return ImGuiKey_F5;
    case KEY::KEY_F6: return ImGuiKey_F6;
    case KEY::KEY_F7: return ImGuiKey_F7;
    case KEY::KEY_F8: return ImGuiKey_F8;
    case KEY::KEY_F9: return ImGuiKey_F9;
    case KEY::KEY_F10: return ImGuiKey_F10;
    case KEY::KEY_F11: return ImGuiKey_F11;
    case KEY::KEY_F12: return ImGuiKey_F12;
    case KEY::KEY_F13: return ImGuiKey_F13;
    case KEY::KEY_F14: return ImGuiKey_F14;
    case KEY::KEY_F15: return ImGuiKey_F15;
    case KEY::KEY_F16: return ImGuiKey_F16;
    case KEY::KEY_F17: return ImGuiKey_F17;
    case KEY::KEY_F18: return ImGuiKey_F18;
    case KEY::KEY_F19: return ImGuiKey_F19;
    case KEY::KEY_F20: return ImGuiKey_F20;
    case KEY::KEY_F21: return ImGuiKey_F21;
    case KEY::KEY_F22: return ImGuiKey_F22;
    case KEY::KEY_F23: return ImGuiKey_F23;
    case KEY::KEY_F24: return ImGuiKey_F24;

    default: return ImGuiKey_None;
    }
}

static unsigned char osgKeyToChar(int key)
{
	using KEY = osgGA::GUIEventAdapter::KeySymbol;
	switch (key)
	{
	case KEY::KEY_KP_Space: return ' ';
	case KEY::KEY_KP_Equal: return '=';
	case KEY::KEY_KP_Multiply: return '*';
	case KEY::KEY_KP_Add: return '+';
	case KEY::KEY_KP_Subtract: return '-';
	case KEY::KEY_KP_Decimal: return '.';
	case KEY::KEY_KP_Divide: return '/';
	case KEY::KEY_KP_0: return '0';
	case KEY::KEY_KP_1: return '1';
	case KEY::KEY_KP_2: return '2';
	case KEY::KEY_KP_3: return '3';
	case KEY::KEY_KP_4: return '4';
	case KEY::KEY_KP_5: return '5';
	case KEY::KEY_KP_6: return '6';
	case KEY::KEY_KP_7: return '7';
	case KEY::KEY_KP_8: return '8';
	case KEY::KEY_KP_9: return '9';
	default: return (key >= 20 && key <= 0x7F) ? key : 0; // printable character range
	}
}

bool ImGui_ImplOsg_Handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa, bool wantCaptureEvents)
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

		int osgKey = ea.getKey();

		/*std::cout << osgKey << " ";
		if (ea.getModKeyMask() != 0)
		{
			std::cout << "with ";
			if (ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_CTRL)
				std::cout << "ctrl ";
			if (ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_SHIFT)
				std::cout << "shift ";
			if (ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_ALT)
				std::cout << "alt ";
			if (ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_SUPER)
				std::cout << "super ";
		}
		std::cout << (isKeyDown ? "press" : "release") << std::endl;*/

		io.AddKeyEvent(ImGuiMod_Ctrl, ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_CTRL);
		io.AddKeyEvent(ImGuiMod_Shift, ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_SHIFT);
		io.AddKeyEvent(ImGuiMod_Alt, ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_ALT);
		io.AddKeyEvent(ImGuiMod_Super, ea.getModKeyMask() & osgGA::GUIEventAdapter::MODKEY_SUPER);

		unsigned char character = osgKeyToChar(osgKey);
		if (isKeyDown && character != 0)
			io.AddInputCharacter(character);

		ImGuiKey imguiKey = ImGui_ImplOsg_KeyToImGuiKey(osgKey);
		if (imguiKey != ImGuiKey_None)
			io.AddKeyEvent(imguiKey, isKeyDown);
		return wantCaptureKeyboard && wantCaptureEvents;
	}
    case (osgGA::GUIEventAdapter::DOUBLECLICK):
	case (osgGA::GUIEventAdapter::RELEASE):
	case (osgGA::GUIEventAdapter::PUSH):
	{
		//io.MousePos = ImVec2(ea.getX(), io.DisplaySize.y - ea.getY());
		io.AddMousePosEvent(ea.getX(), io.DisplaySize.y - ea.getY());
		bool isButtonDown = ea.getEventType() == osgGA::GUIEventAdapter::PUSH || ea.getEventType() == osgGA::GUIEventAdapter::DOUBLECLICK;
		if (ea.getButton() == osgGA::GUIEventAdapter::LEFT_MOUSE_BUTTON)
			io.AddMouseButtonEvent(ImGuiMouseButton_Left, isButtonDown);
		if (ea.getButton() == osgGA::GUIEventAdapter::RIGHT_MOUSE_BUTTON)
			io.AddMouseButtonEvent(ImGuiMouseButton_Right, isButtonDown);
		if (ea.getButton() == osgGA::GUIEventAdapter::MIDDLE_MOUSE_BUTTON)
			io.AddMouseButtonEvent(ImGuiMouseButton_Middle, isButtonDown);
		return wantCaptureMouse && wantCaptureEvents;
	}
	case (osgGA::GUIEventAdapter::DRAG):
	case (osgGA::GUIEventAdapter::MOVE):
	{
		io.AddMousePosEvent(ea.getX(), io.DisplaySize.y - ea.getY());
		return wantCaptureMouse && wantCaptureEvents;
	}
	case (osgGA::GUIEventAdapter::SCROLL):
	{
		io.AddMouseWheelEvent(0.0, ea.getScrollingMotion() == osgGA::GUIEventAdapter::SCROLL_UP ? 1.0 : -1.0);
		return wantCaptureMouse && wantCaptureEvents;
	}
	default: return false;
	}

	return false;
}

#endif // #ifndef IMGUI_DISABLE
