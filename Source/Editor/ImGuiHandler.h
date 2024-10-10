#pragma once
#include <Engine/Render/Pipeline.h>
#include <Engine/Core/Context.h>
#include <Engine/Core/Component.h>
#include <Engine/Component/MeshRenderer.h>
#include <ThirdParty/imgui/imgui.h>
#include <ThirdParty/imgui/imgui_impl_opengl3.h>
#include <ThirdParty/imgui/imgui_impl_osg.h>
#include <ThirdParty/imgui/imgui_stdlib.h>

#include <osgViewer/ViewerEventHandlers>

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
		ImGuiHandler(osgViewer::Viewer* viewer, osg::Camera* camera, osg::Texture2D* sceneColorTexture, Pipeline* pipeline) : _imguiCamera(camera), _sceneColorTexture(sceneColorTexture), _pipeline(pipeline)
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
			ImGui_ImplOsg_Init(viewer, camera);
		}

		virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
		{
			static bool initialized = false;
			if (!initialized)
			{
                _imguiCamera->setPostDrawCallback(new ImGuiNewFrameCallback(this));
                initialized = true;
			}

            return ImGui_ImplOsg_Handle(ea, aa, !(_sceneViewWindowIsFocused && _sceneViewItemIsHovered));
		}

        osg::Matrixd getSceneViewViewportMatrix()
        {
            double halfWidth = _sceneViewWidth / 2.0, halfHeight = _sceneViewHeight / 2.0;
            return osg::Matrixd(
                halfWidth, 0.0, 0.0, 0.0,
                0.0, halfHeight, 0.0, 0.0,
                0.0, 0.0, 0.5, 0.0,
                _sceneViewX + halfWidth, _sceneViewY + halfHeight, 0.5, 1.0
            );
        }

	private:
        osg::ref_ptr<osg::Camera> _imguiCamera;
        osg::ref_ptr<osg::Texture2D> _sceneColorTexture;
        osg::ref_ptr<Pipeline> _pipeline;
        bool _sceneViewWindowIsFocused = false;
        bool _sceneViewItemIsHovered = false;
        int _sceneViewX, _sceneViewY;
        int _sceneViewWidth, _sceneViewHeight;
        bool _sceneViewSizeDirty = false;

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
                _handler->updateSceneViewSize();
				ImGui::Render();
				ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
			}
		};

        void drawMeshRendererInspector(CMeshRenderer* meshRenderer)
        {
            static std::string paths[] = {
                "Texture/AwesomeFace",
                "Texture/Container",
                "Texture/T_Ceramic_Tile_M",
                "Texture/T_Ceramic_Tile_N",
                "Texture/T_Perlin_Noise_M",
                "Texture/T_Rock_Marble_Polished_D",
            };
            if (ImGui::TreeNode("MeshRenderer"))
            {
                uint32_t submeshesCount = meshRenderer->getSubmeshesCount();
                for (uint32_t i = 0; i < submeshesCount; ++i)
                {
                    if (ImGui::TreeNode(("Material" + std::to_string(i)).c_str()))
                    {
                        AMaterial* material = meshRenderer->getMaterial(i);
                        ImGui::Text(material->getPath().c_str());

                        AMaterialTemplate* materialTemplate;
                        if (material->getType() == Asset::Type::MaterialTemplate)
                            materialTemplate = reinterpret_cast<AMaterialTemplate*>(material);
                        else if (material->getType() == Asset::Type::MaterialInstance)
                            materialTemplate = dynamic_cast<AMaterialInstance*>(material)->getMaterialTemplate();

                        auto itr = materialTemplate->_parameters.begin();
                        uint32_t buttonId = 0;
                        while (itr != materialTemplate->_parameters.end())
                        {
                            switch (itr->second.index())
                            {
                            case size_t(AMaterialTemplate::ParameterIndex::Bool):
                            {
                                bool boolValue = std::get<bool>(itr->second);
                                if (ImGui::Checkbox(itr->first.c_str(), &boolValue))
                                    materialTemplate->setParameter(itr->first, boolValue);
                                break;
                            }
                            case size_t(AMaterialTemplate::ParameterIndex::Int):
                            {
                                int intValue = std::get<int>(itr->second);
                                if (ImGui::DragInt(itr->first.c_str(), &intValue))
                                    materialTemplate->setParameter(itr->first, intValue);
                                break;
                            }
                            case size_t(AMaterialTemplate::ParameterIndex::Float):
                            {
                                float floatValue = std::get<float>(itr->second);
                                if (ImGui::DragFloat(itr->first.c_str(), &floatValue))
                                    materialTemplate->setParameter(itr->first, floatValue);
                                break;
                            }
                            case size_t(AMaterialTemplate::ParameterIndex::Float2):
                            {
                                osg::Vec2 float2Value = std::get<osg::Vec2>(itr->second);
                                if (ImGui::DragFloat2(itr->first.c_str(), &float2Value.x()))
                                    materialTemplate->setParameter(itr->first, float2Value);
                                break;
                            }
                            case size_t(AMaterialTemplate::ParameterIndex::Float3):
                            {
                                osg::Vec3 float3Value = std::get<osg::Vec3>(itr->second);
                                if (ImGui::ColorEdit3(itr->first.c_str(), &float3Value.x()))
                                    materialTemplate->setParameter(itr->first, float3Value);
                                break;
                            }
                            case size_t(AMaterialTemplate::ParameterIndex::Float4):
                            {
                                osg::Vec4 float4Value = std::get<osg::Vec4>(itr->second);
                                if (ImGui::ColorEdit4(itr->first.c_str(), &float4Value.x()))
                                    materialTemplate->setParameter(itr->first, float4Value);
                                break;
                            }
                            case size_t(AMaterialTemplate::ParameterIndex::Texture):
                            {
                                using TextureAndUnit = AMaterialTemplate::TextureAndUnit;
                                TextureAndUnit& textureAndUnit = std::get<TextureAndUnit>(itr->second);

                                if (ImGui::BeginCombo(itr->first.c_str(), textureAndUnit.first->getPath().c_str()))
                                {
                                    for (int n = 0; n < IM_ARRAYSIZE(paths); ++n)
                                    {
                                        const bool is_selected = (textureAndUnit.first->getPath() == paths[n]);
                                        if (ImGui::Selectable(paths[n].c_str(), is_selected))
                                            materialTemplate->setParameter(itr->first, xxx::AssetManager::loadAsset<xxx::ATexture>(paths[n]));

                                        // Set the initial focus when opening the combo (scrolling + keyboard navigation focus)
                                        if (is_selected)
                                            ImGui::SetItemDefaultFocus();
                                    }
                                    ImGui::EndCombo();
                                }

                                //ImGui::Text(textureAndUnit.first->getPath().c_str());
                                break;
                            }
                            default:
                                break;
                            }

                            ImGui::SameLine(ImGui::GetWindowContentRegionMax().x - 30);
                            auto removedItr = itr++;
                            ImGui::PushID(buttonId);
                            if (ImGui::Button("×"))
                                materialTemplate->removeParameter(removedItr->first);
                            ImGui::PopID();
                            buttonId++;
                        }

                        if (ImGui::InputTextMultiline("Source", &materialTemplate->getSource()))
                            materialTemplate->_shaderDirty = true;

                        if (ImGui::Button("Apply"))
                        {
                            materialTemplate->apply();
                        }

                        ImGui::TreePop();
                    }
                }
                ImGui::TreePop();
            }
        }

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

			//if (ImGui::Begin("Hierarchy"))
			//{
			//	/*if (ImGui::TreeNodeEx())*/
			//	static char textbuf[1024] = {};
			//	ImGui::InputText("中文测试", textbuf, 1024);
			//}
			//ImGui::End();

            if (ImGui::Begin("Inspector"))
            {
                Entity* activedEntity = Context::get().getActivedEntity();
                if (activedEntity)
                {
                    ImGui::Text(activedEntity->getEntityName().c_str());
                    if (ImGui::TreeNode("Transform"))
                    {
                        /*static float position[3] = { 0.0f, 0.0f, 0.0f };
                        static float rotation[3] = { 0.0f, 0.0f, 0.0f };
                        static float scale[3] = { 1.0, 1.0, 1.0 };*/
                        /*ImGui::DragFloat3("Position", position);
                        ImGui::DragFloat3("Rotation", rotation);
                        ImGui::DragFloat3("Scale", scale);*/
                        static osg::Vec3d entityPosition;
                        static osg::Vec3d entityRotation;
                        static osg::Vec3d entityScale;
                        entityPosition = activedEntity->getPosition();
                        entityRotation = activedEntity->getRotation();
                        entityScale = activedEntity->getScale();
                        if (ImGui::DragScalarN("Position", ImGuiDataType_Double, &entityPosition.x(), 3, 0.05))
                        {
                            activedEntity->setPosition(entityPosition);
                            activedEntity->getMatrix();
                        }
                        if (ImGui::DragScalarN("Rotation", ImGuiDataType_Double, &entityRotation.x(), 3))
                        {
                            activedEntity->setRotation(entityRotation);
                            activedEntity->getMatrix();
                        }
                        if (ImGui::DragScalarN("Scale", ImGuiDataType_Double, &entityScale.x(), 3))
                        {
                            activedEntity->setScale(entityScale);
                            activedEntity->getMatrix();
                        }
                        ImGui::TreePop();
                    }
                    uint32_t componentsCount = activedEntity->getComponentsCount();
                    for (uint32_t i = 0; i < componentsCount; ++i)
                    {
                        Component* component = activedEntity->getComponent(i);
                        if (component->getType() == Component::Type::MeshRenderer)
                            drawMeshRendererInspector(dynamic_cast<CMeshRenderer*>(component));
                    }
                }
                
            }
            ImGui::End();

            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0, 0.0));
            if (ImGui::Begin("SceneView"))
            {
                _sceneViewWindowIsFocused = ImGui::IsWindowFocused();

                if (_sceneColorTexture->getTextureObject(0))
                {
                    ImVec2 regionMin = ImGui::GetWindowContentRegionMin();
                    ImVec2 regionMax = ImGui::GetWindowContentRegionMax();
                    ImGui::Image((void*)_sceneColorTexture->getTextureObject(0)->id(), ImVec2(regionMax.x - regionMin.x, regionMax.y - regionMin.y), ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
                    _sceneViewItemIsHovered = ImGui::IsItemHovered();
                    ImVec2 itemRectMin = ImGui::GetItemRectMin();
                    ImVec2 itemRectMax = ImGui::GetItemRectMax();

                    _sceneViewX = itemRectMin.x;
                    _sceneViewY = io.DisplaySize.y - itemRectMax.y;
                    int width = itemRectMax.x - itemRectMin.x;
                    int height = itemRectMax.y - itemRectMin.y;
                    if (width != _sceneViewWidth || height != _sceneViewHeight)
                    {
                        _sceneViewWidth = width;
                        _sceneViewHeight = height;
                        _sceneViewSizeDirty = true;
                    }
                }
                else
                {
                    _sceneViewWidth = _sceneViewHeight = 0;
                }
            }
            ImGui::End();
            ImGui::PopStyleVar();

		}

        void updateSceneViewSize()
        {
            if (_sceneViewSizeDirty)
            {
                _pipeline->resize(_sceneViewWidth, _sceneViewHeight, false);
                _sceneViewSizeDirty = false;
            }
        }
	};
}
