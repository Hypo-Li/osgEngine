#pragma once
#include <osgViewer/ViewerEventHandlers>
#include <ThirdParty/imgui/imgui.h>
#include <ThirdParty/imgui/imgui_impl_opengl3.h>
#include <ThirdParty/imgui/imgui_impl_osg.h>
#include <ThirdParty/imgui/imgui_stdlib.h>

namespace xxx
{
    class ImGuiInitOperation : public osg::Operation
    {
        const char* _glslVersion;
    public:
        ImGuiInitOperation(const char* glslVersion = nullptr) : osg::Operation("ImGuiInitOperation", false), _glslVersion(glslVersion) {}

        void operator()(osg::Object* object) override
        {
            osg::GraphicsContext* context = dynamic_cast<osg::GraphicsContext*>(object);
            if (!context)
                return;

            ImGui_ImplOpenGL3_Init(_glslVersion);
        }
    };

    class ImGuiHandler : public osgGA::GUIEventHandler
    {
    public:
        ImGuiHandler(osgViewer::Viewer* viewer, osg::Camera* camera) : _imguiCamera(camera)
        {
            IMGUI_CHECKVERSION();
            ImGui::CreateContext();
            ImGuiIO& io = ImGui::GetIO(); (void)io;
            io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
            io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
            io.ConfigWindowsMoveFromTitleBarOnly = true;
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
            //style.Colors[ImGuiCol_WindowBg].w = 1.0;
            //style.ScaleAllSizes(1.0);
            ImGui_ImplOsg_Init(camera);
        }

        virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa) override
        {
            static bool initialized = false;
            if (!initialized)
            {
                _imguiCamera->setPostDrawCallback(new ImGuiNewFrameCallback(this));
                initialized = true;
            }

            return ImGui_ImplOsg_Handle(ea, aa, true);
        }

    private:
        osg::ref_ptr<osg::Camera> _imguiCamera;

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

        virtual void draw();
    };
}
