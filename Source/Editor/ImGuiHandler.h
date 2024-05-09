#pragma once
#include <osgViewer/ViewerEventHandlers>
#include <ThirdParty/imgui/imgui.h>
#include <ThirdParty/imgui/imgui_impl_opengl3.h>
#include <ThirdParty/imgui/imgui_impl_osg.h>
namespace xxx
{
	class ImGuiHandler : public osgGA::GUIEventHandler
	{
	public:
		ImGuiHandler(osgViewer::Viewer* viewer)
		{
			IMGUI_CHECKVERSION();
			ImGui::CreateContext();
			ImGuiIO& io = ImGui::GetIO(); (void)io;
			io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
			io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
			//io.ConfigDockingAlwaysTabBar = true;

            std::ifstream cnsCharIFS(TEMP_DIR "cns_common_char.txt", std::ios::binary | std::ios::ate);
            assert(cnsCharIFS.is_open());
            size_t bufSize = cnsCharIFS.tellg();
            cnsCharIFS.seekg(std::ios::beg);
            unsigned char* charBuf = new unsigned char[bufSize];
            cnsCharIFS.read((char*)charBuf, bufSize);
            cnsCharIFS.close();

            static ImVector<ImWchar> myRange;
            ImFontGlyphRangesBuilder myGlyph;
            myGlyph.AddText((const char*)charBuf);
            myGlyph.BuildRanges(&myRange);
            delete[] charBuf;

            ImFontConfig config;
            config.MergeMode = true;
			io.Fonts->AddFontFromFileTTF(TEMP_DIR "ProggyClean.ttf", 13.0, NULL, io.Fonts->GetGlyphRangesDefault());
            io.Fonts->AddFontFromFileTTF(TEMP_DIR "NotoSansSC-Regular.ttf", 20.0, &config, myRange.Data);
            io.Fonts->Build();
			ImGui::StyleColorsClassic();
			ImGuiStyle& style = ImGui::GetStyle();
			style.WindowRounding = 6.0;
			style.Colors[ImGuiCol_WindowBg].w = 1.0;
            style.ScaleAllSizes(1.5);
			ImGui_ImplOsg_Init(viewer);
		}

		virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
		{
			static bool initialized = false;
			if (!initialized)
			{
				osg::View* view = aa.asView();
				if (view)
				{
					view->getCamera()->setPostDrawCallback(new ImGuiNewFrameCallback(this));
					initialized = true;
				}
			}

			return ImGui_ImplOsg_Handle(ea, aa);
		}

	private:
		class ImGuiNewFrameCallback : public osg::Camera::DrawCallback
		{
			ImGuiHandler* _handler;
		public:
			ImGuiNewFrameCallback(ImGuiHandler* handler) : _handler(handler) {}

			void operator()(osg::RenderInfo& renderInfo) const override
			{
				ImGui_ImplOpenGL3_NewFrame();
				ImGui_ImplOsg_NewFrame();
				ImGui::NewFrame();
				_handler->draw();
				ImGui::Render();
				ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
			}
		};

		virtual void draw()
		{
			static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

			const ImGuiViewport* viewport = ImGui::GetMainViewport();
			ImGui::SetNextWindowPos(viewport->WorkPos);
			ImGui::SetNextWindowSize(viewport->WorkSize);
			ImGui::SetNextWindowViewport(viewport->ID);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
			ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));

			ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
			window_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
			window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

			ImGui::Begin("DockSpace Demo", nullptr, window_flags);
			ImGui::PopStyleVar(3);
			ImGuiIO& io = ImGui::GetIO();
			if (io.ConfigFlags & ImGuiConfigFlags_DockingEnable)
			{
				ImGuiID dockspace_id = ImGui::GetID("MyDockSpace");
				ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), dockspace_flags);
			}
			if (ImGui::BeginMenuBar())
			{
				if (ImGui::BeginMenu("File"))
				{
					if (ImGui::MenuItem("New", "Ctrl+Z")) {}
					if (ImGui::MenuItem("Open..", "Ctrl+O")) {}
					if (ImGui::BeginMenu("Open Recent"))
					{
						if (ImGui::MenuItem("test.syzproj")) {}
						ImGui::EndMenu();
					}
					ImGui::Separator();
					if (ImGui::MenuItem("Save", "Ctrl+S")) {}
					if (ImGui::MenuItem("Save As..", "Shift+Ctrl+S")) {}
					ImGui::EndMenu();
				}
				ImGui::EndMenuBar();
			}
			ImGui::End();

			if (ImGui::Begin("Hierarchy"))
			{
				/*if (ImGui::TreeNodeEx())*/
				static char textbuf[1024] = {};
				ImGui::InputText("中文测试", textbuf, 1024);
			}
			ImGui::End();

            if (ImGui::Begin("Inspector"))
            {
                if (ImGui::TreeNode("Transform"))
                {
                    static float position[3] = { 0.0f, 0.0f, 0.0f };
                    static float rotation[3] = { 0.0f, 0.0f, 0.0f };
                    static float scale[3] = { 1.0, 1.0, 1.0 };
                    ImGui::DragFloat3("Position", position);
                    ImGui::DragFloat3("Rotation", rotation);
                    ImGui::DragFloat3("Scale", scale);
                    ImGui::TreePop();
                }
            }
            ImGui::End();
		}
	};
}
