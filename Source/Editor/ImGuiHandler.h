#pragma once
#include "Widget.h"
#include <Engine/Render/Pipeline.h>
#include <Engine/Core/Context.h>
#include <Engine/Core/Component.h>
#include <Engine/Core/Asset.h>
#include <Engine/Core/AssetManager.h>
#include <Engine/Component/MeshRenderer.h>
#include <ThirdParty/imgui/imgui.h>
#include <ThirdParty/imgui/imgui_impl_opengl3.h>
#include <ThirdParty/imgui/imgui_impl_osg.h>
#include <ThirdParty/imgui/imgui_stdlib.h>
#include <ThirdParty/imgui/imgui_internal.h>

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
		ImGuiHandler(osgViewer::Viewer* viewer, Pipeline* pipeline) : mPipeline(pipeline), mSceneViewViewport(new osg::Viewport)
		{
            Pipeline::Pass* lastPass = mPipeline->getPasses().rbegin()->get();
            Pipeline::Pass* penultimatePass = (mPipeline->getPasses().rbegin() + 1)->get();
            mImGuiCamera = lastPass->getCamera();
            mSceneColorTexture = dynamic_cast<osg::Texture2D*>(penultimatePass->getBufferTexture(Pipeline::Pass::BufferType::COLOR_BUFFER0));

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
			ImGui_ImplOsg_Init(viewer, mImGuiCamera);
		}

		virtual bool handle(const osgGA::GUIEventAdapter& ea, osgGA::GUIActionAdapter& aa)
		{
			static bool initialized = false;
			if (!initialized)
			{
                mImGuiCamera->setPostDrawCallback(new ImGuiNewFrameCallback(this));
                initialized = true;
			}

            return ImGui_ImplOsg_Handle(ea, aa, !(mSceneViewWindowIsFocused && mSceneViewItemIsHovered));
		}

        osg::Viewport* getSceneViewViewport() const
        {
            return mSceneViewViewport;
        }

	private:
        osg::ref_ptr<osg::Camera> mImGuiCamera;
        osg::ref_ptr<osg::Texture2D> mSceneColorTexture;
        osg::ref_ptr<osg::Viewport> mSceneViewViewport;
        osg::ref_ptr<Pipeline> mPipeline;
        bool mSceneViewWindowIsFocused = false;
        bool mSceneViewItemIsHovered = false;
        int mSceneViewX, mSceneViewY;
        int mSceneViewWidth, mSceneViewHeight;
        bool mSceneViewSizeDirty = false;

		class ImGuiNewFrameCallback : public osg::Camera::DrawCallback
		{
			ImGuiHandler* mHandler;
		public:
			ImGuiNewFrameCallback(ImGuiHandler* handler) : mHandler(handler) {}

			void operator()(osg::RenderInfo& renderInfo) const override
			{
				ImGui_ImplOpenGL3_NewFrame();
				ImGui_ImplOsg_NewFrame();
				ImGui::NewFrame();
				mHandler->draw();
                mHandler->updateSceneViewSize();
				ImGui::Render();
				ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
			}
		};

        bool drawShaderGUI(Shader* shader)
        {
            Asset* shaderAsset = shader->getAsset();
            if (shaderAsset)
                ImGui::Text(shaderAsset->getPath().c_str());

            if (shaderAsset && ImGui::Button("Save"))
                shaderAsset->save();

            ImGui::InputTextMultiline("Source", &shader->getSource());

            return ImGui::Button("Apply");
        }

        void drawMaterialGUI(Material* material)
        {
            Asset* materialAsset = material->getAsset();
            if (materialAsset)
                ImGui::Text(materialAsset->getPath().c_str());

            if (materialAsset && ImGui::Button("Save"))
                materialAsset->save();

            const Material::Parameters& materialParameters = material->getParameters();

            ShadingModel shadingModel = material->getShadingModel();
            ShadingModel newShadingModel = EnumCombo("ShadingModel", shadingModel);
            if (shadingModel != newShadingModel)
                material->setShadingModel(newShadingModel);

            AlphaMode alphaMode = material->getAlphaMode();
            AlphaMode newAlphaMode = EnumCombo("AlphaMode", alphaMode);
            if (alphaMode != newAlphaMode)
                material->setAlphaMode(newAlphaMode);

            bool doubleSided = material->getDoubleSided();
            if (ImGui::Checkbox("Double Sided", &doubleSided))
                material->setDoubleSided(doubleSided);

            int paramId = 0;
            for (auto materialParamIt = materialParameters.begin(); materialParamIt != materialParameters.end(); ++materialParamIt)
            {
                const std::string& parameterName = materialParamIt->first;
                bool materialParamEnable = materialParamIt->second.second;
                const Shader::ParameterValue& parameterValue = materialParamIt->second.first;

                ImGui::PushID(paramId);
                if (ImGui::Checkbox("", &materialParamEnable))
                {
                    material->enableParameter(parameterName, materialParamEnable);
                }
                ImGui::PopID();
                ImGui::SameLine();

                if (!materialParamEnable)
                    ImGui::BeginDisabled();

                switch (parameterValue.index())
                {
                case size_t(Shader::ParameterIndex::Bool):
                {
                    bool boolValue = std::get<bool>(parameterValue);
                    if (ImGui::Checkbox(parameterName.c_str(), &boolValue))
                        material->setParameter(parameterName, boolValue);
                    break;
                }
                case size_t(Shader::ParameterIndex::Int):
                {
                    int intValue = std::get<int>(parameterValue);
                    if (ImGui::DragInt(parameterName.c_str(), &intValue))
                        material->setParameter(parameterName, intValue);
                    break;
                }
                case size_t(Shader::ParameterIndex::Float):
                {
                    float floatValue = std::get<float>(parameterValue);
                    if (ImGui::DragFloat(parameterName.c_str(), &floatValue))
                        material->setParameter(parameterName, floatValue);
                    break;
                }
                case size_t(Shader::ParameterIndex::Vec2f):
                {
                    osg::Vec2f vec2fValue = std::get<osg::Vec2f>(parameterValue);
                    if (ImGui::DragFloat2(parameterName.c_str(), &vec2fValue.x()))
                        material->setParameter(parameterName, vec2fValue);
                    break;
                }
                case size_t(Shader::ParameterIndex::Vec3f):
                {
                    osg::Vec3f vec3fValue = std::get<osg::Vec3f>(parameterValue);
                    if (ImGui::DragFloat3(parameterName.c_str(), &vec3fValue.x()))
                        material->setParameter(parameterName, vec3fValue);
                    break;
                }
                case size_t(Shader::ParameterIndex::Vec4f):
                {
                    osg::Vec4 vec4fValue = std::get<osg::Vec4f>(parameterValue);
                    if (ImGui::DragFloat4(parameterName.c_str(), &vec4fValue.x()))
                        material->setParameter(parameterName, vec4fValue);
                    break;
                }
                case size_t(Shader::ParameterIndex::Texture):
                {
                    const Shader::TextureAndUnit& textureAndUnit = std::get<Shader::TextureAndUnit>(parameterValue);
                    Asset* textureAsset = textureAndUnit.first->getAsset();
                    if (textureAsset)
                    {
                        Asset* selectedAsset = AssetCombo<Texture>(parameterName.c_str(), textureAsset);
                        if (selectedAsset)
                        {
                            if (!selectedAsset->isLoaded())
                                selectedAsset->load();
                            material->setParameter(parameterName, selectedAsset->getRootObject<Texture>());
                        }
                    }
                    break;
                }
                default:
                    break;
                }

                if (!materialParamEnable)
                    ImGui::EndDisabled();

                paramId++;
            }

            if (ImGui::TreeNode("Shader"))
            {
                if (drawShaderGUI(material->getShader()))
                {
                    material->syncWithShader();
                }
                ImGui::TreePop();
            }
        }

        void drawMeshRendererGUI(MeshRenderer* meshRenderer)
        {
            if (ImGui::CollapsingHeader("MeshRenderer"))
            {
                uint32_t submeshesCount = meshRenderer->getSubmeshesCount();
                for (uint32_t i = 0; i < submeshesCount; ++i)
                {
                    Material* material = meshRenderer->getMaterial(i);
                    if (ImGui::TreeNode(("Material" + std::to_string(i)).c_str()))
                    {
                        drawMaterialGUI(material);
                        ImGui::TreePop();
                    }
                }
            }
        }

        void drawSceneView()
        {
            ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0, 0.0));
            if (ImGui::Begin("Scene View"))
            {
                ImGuiIO& io = ImGui::GetIO();
                mSceneViewWindowIsFocused = ImGui::IsWindowFocused();

                if (mSceneColorTexture->getTextureObject(0))
                {
                    ImGui::Image((void*)mSceneColorTexture->getTextureObject(0)->id(), ImGui::GetContentRegionAvail(), ImVec2(0.0, 1.0), ImVec2(1.0, 0.0));
                    mSceneViewItemIsHovered = ImGui::IsItemHovered();

                    ImVec2 itemRectMin = ImGui::GetItemRectMin();
                    ImVec2 itemRectMax = ImGui::GetItemRectMax();
                    mSceneViewX = itemRectMin.x;
                    mSceneViewY = io.DisplaySize.y - itemRectMax.y;
                    int width = itemRectMax.x - itemRectMin.x;
                    int height = itemRectMax.y - itemRectMin.y;
                    if (width != mSceneColorTexture->getTextureWidth() || height != mSceneColorTexture->getTextureHeight())
                    {
                        mSceneViewWidth = width;
                        mSceneViewHeight = height;
                        mSceneViewSizeDirty = true;
                    }
                }
                else
                {
                    mSceneViewWidth = mSceneViewHeight = 0;
                }

                mSceneViewViewport->setViewport(mSceneViewX, mSceneViewY, mSceneViewWidth, mSceneViewHeight);
            }
            ImGui::End();
            ImGui::PopStyleVar();
        }

        void drawInspector()
        {
            if (ImGui::Begin("Inspector"))
            {
                Entity* activedEntity = Context::get().getActivedEntity();
                if (activedEntity)
                {
                    ImGui::Text(activedEntity->getName().c_str());
                    if (ImGui::CollapsingHeader("Transform"))
                    {
                        static osg::Vec3d entityPosition;
                        static osg::Vec3d entityRotation;
                        static osg::Vec3d entityScale;
                        entityPosition = activedEntity->getPosition();
                        entityRotation = activedEntity->getRotation();
                        entityScale = activedEntity->getScale();
                        if (ImGui::DragScalarN("Position", ImGuiDataType_Double, &entityPosition.x(), 3, 0.01))
                        {
                            activedEntity->setPosition(entityPosition);
                        }
                        if (ImGui::DragScalarN("Rotation", ImGuiDataType_Double, &entityRotation.x(), 3))
                        {
                            activedEntity->setRotation(entityRotation);
                        }
                        if (ImGui::DragScalarN("Scale", ImGuiDataType_Double, &entityScale.x(), 3, 0.01))
                        {
                            activedEntity->setScale(entityScale);
                        }
                    }
                    uint32_t componentsCount = activedEntity->getComponentsCount();
                    for (uint32_t i = 0; i < componentsCount; ++i)
                    {
                        Component* component = activedEntity->getComponent(i);
                        if (component->getType() == Component::Type::MeshRenderer)
                            drawMeshRendererGUI(dynamic_cast<MeshRenderer*>(component));
                    }
                }

                static std::vector<std::string> items = {
                    "Test",
                    "SomeThings",
                    "What",
                    "Why",
                    "Where",
                    "How"
                };
                static int currentItem = 0;
                ImGui::ComboWithFilter("ComboFilter", &currentItem, items, 6);
            }
            ImGui::End();
        }

        GLenum getTextureId(osg::Texture2D* texture, osg::GraphicsContext* gc)
        {
            if (!texture->getTextureObject(gc->getState()->getContextID()))
                texture->apply(*gc->getState());
            return texture->getTextureObject(gc->getState()->getContextID())->id();
        }

        void drawAssetBrowser()
        {
            if (ImGui::Begin("Asset Browser"))
            {
                static std::filesystem::path currentPath = Context::get().getEngineAssetPath();
                static osg::ref_ptr<osg::Texture2D> texture = new osg::Texture2D(osgDB::readImageFile(TEMP_DIR "awesomeface.png"));
                static GLenum textureId = getTextureId(texture, mPipeline->getOsgGraphicsContext());

                if (currentPath != Context::get().getEngineAssetPath())
                {
                    if (ImGui::Button("<-"))
                        currentPath = currentPath.parent_path();
                }

                static float padding = 16.0f;
                static float thumbnailSize = 64.0f;
                float cellSize = thumbnailSize + padding;

                float panelWidth = ImGui::GetContentRegionAvail().x;
                int columnCount = (int)(panelWidth / cellSize);
                if (columnCount < 1)
                    columnCount = 1;

                ImGuiIO& io = ImGui::GetIO();
                ImGui::Columns(columnCount, 0, false);

                for (auto& directoryEntry : std::filesystem::directory_iterator(currentPath))
                {
                    const auto& path = directoryEntry.path();
                    if (directoryEntry.is_regular_file() && path.extension().string() != ".xast")
                        continue;

                    std::string stemString = path.stem().string();

                    ImGui::PushID(stemString.c_str());
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));
                    ImGui::ImageButton(ImTextureID(textureId), {thumbnailSize, thumbnailSize}, {0, 1}, {1, 0});
                    ImGui::PopStyleColor();
                    if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
                    {
                        if (directoryEntry.is_directory())
                            currentPath /= path.filename();
                    }

                    ImGui::TextWrapped(stemString.c_str());

                    ImGui::NextColumn();

                    ImGui::PopID();
                }

                ImGui::Columns(1);

                ImGui::SliderFloat("Thumbnail Size", &thumbnailSize, 16, 512);
                ImGui::SliderFloat("Padding", &padding, 0, 32);
            }
            ImGui::End();
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

			ImGui::Begin("DockSpace", nullptr, window_flags);
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

            drawSceneView();
            drawInspector();
            drawAssetBrowser();
		}

        void updateSceneViewSize()
        {
            if (mSceneViewSizeDirty)
            {
                mPipeline->resize(mSceneViewWidth, mSceneViewHeight, false);
                mSceneViewSizeDirty = false;
            }
        }
	};
}
