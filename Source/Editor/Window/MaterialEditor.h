#include "Window.h"
#include "Widget.h"

#include <osgViewer/View>

namespace xxx::editor
{
    class MaterialEditor : public Window
    {
    public:
        MaterialEditor(Asset* materialAsset) : Window(materialAsset->getName() + "##" + materialAsset->getPath()), mMaterialAsset(materialAsset)
        {
            mMaterial = mMaterialAsset->getRootObject<Material>();

            /*mSimpleView = new osgViewer::View;
            mSimpleView->setSceneData(mEngine->getView()->getSceneData());
            osg::Camera* camera = mSimpleView->getCamera();
            camera->setViewport(0, 0, 128, 128);
            camera->setProjectionMatrixAsPerspective(90.0, 1.0, 0.1, 400.0);

            mSimplePipeline = new Pipeline(mSimpleView, mEngine->getPipeline()->getGraphicsContext());
            Pipeline::Pass* gbufferPass = mSimplePipeline->addInputPass("GBuffer", 0x00000001, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            using BufferType = Pipeline::Pass::BufferType;
            gbufferPass->attach(BufferType::COLOR_BUFFER0, GL_RGBA16F);
            gbufferPass->attach(BufferType::DEPTH_BUFFER, GL_DEPTH_COMPONENT24);

            mSimpleView->addEventHandler(imguiHandler);

            mViewer->addView(mSimpleView);

            SceneView* sceneView2 = wm.createWindow<SceneView>(
                "Scene View 2",
                dynamic_cast<osg::Texture2D*>(gbufferPass->getBufferTexture(BufferType::COLOR_BUFFER0)),
                [this](int w, int h) {mSimplePipeline->resize(w, h, true, false); },
                [this]() { if (mViewer->getViewWithFocus() != mSimpleView) mViewer->setCameraWithFocus(mSimpleView->getCamera()); }
            );

            mSimpleView->setCameraManipulator(new osgGA::TrackballManipulator);*/

            /*mView = new osgViewer::View;
            mView->setSceneData();
            osg::Camera* camera = mView->getCamera();
            camera->setViewport(0, 0, 128, 128);
            camera->setProjectionMatrixAsPerspective(90.0, 1.0, 0.1, 400.0);

            mPipeline = createSceneRenderingPipeline(mView, true);
            mView->addEventHandler();*/

        }

        virtual bool draw() override
        {
            if (ImGui::Begin(mTitle.c_str(), &mVisibility))
            {
                ShadingModel shadingModel = mMaterial->getShadingModel();
                if (EnumerationCombo("ShadingModel", &shadingModel))
                    mMaterial->setShadingModel(shadingModel);

                AlphaMode alphaMode = mMaterial->getAlphaMode();
                if (EnumerationCombo("AlphaMode", &alphaMode))
                    mMaterial->setAlphaMode(alphaMode);

                bool doubleSided = mMaterial->getDoubleSided();
                if (ImGui::Checkbox("Double Sided", &doubleSided))
                    mMaterial->setDoubleSided(doubleSided);

                Asset* shaderAsset = mMaterial->getShader()->getAsset();
                if (AssetCombo<Shader>("Shader", &shaderAsset))
                {
                    mMaterial->setShader(shaderAsset->getRootObject<Shader>());
                }

                int paramId = 0;
                const Material::Parameters& materialParameters = mMaterial->getParameters();
                for (auto materialParamIt = materialParameters.begin(); materialParamIt != materialParameters.end(); ++materialParamIt)
                {
                    const std::string& parameterName = materialParamIt->first;
                    bool materialParamEnable = materialParamIt->second.second;
                    const Shader::ParameterValue& parameterValue = materialParamIt->second.first;

                    std::string idString = "material_param_" + std::to_string(paramId);
                    ImGui::PushID(idString.c_str());
                    if (ImGui::Checkbox("", &materialParamEnable))
                    {
                        mMaterial->enableParameter(parameterName, materialParamEnable);
                    }
                    ImGui::PopID();
                    ImGui::SameLine();

                    if (!materialParamEnable)
                        ImGui::BeginDisabled();

                    switch (parameterValue.index())
                    {
                    case size_t(Shader::ParameterType::Bool):
                    {
                        bool boolValue = std::get<bool>(parameterValue);
                        if (ImGui::Checkbox(parameterName.c_str(), &boolValue))
                            mMaterial->setParameter(parameterName, boolValue);
                        break;
                    }
                    case size_t(Shader::ParameterType::Int):
                    {
                        int intValue = std::get<int>(parameterValue);
                        if (ImGui::DragInt(parameterName.c_str(), &intValue))
                            mMaterial->setParameter(parameterName, intValue);
                        break;
                    }
                    case size_t(Shader::ParameterType::Float):
                    {
                        float floatValue = std::get<float>(parameterValue);
                        if (ImGui::DragFloat(parameterName.c_str(), &floatValue, 0.01f))
                            mMaterial->setParameter(parameterName, floatValue);
                        break;
                    }
                    case size_t(Shader::ParameterType::Vec2f):
                    {
                        osg::Vec2f vec2fValue = std::get<osg::Vec2f>(parameterValue);
                        if (ImGui::DragFloat2(parameterName.c_str(), vec2fValue.ptr()))
                            mMaterial->setParameter(parameterName, vec2fValue);
                        break;
                    }
                    case size_t(Shader::ParameterType::Vec3f):
                    {
                        osg::Vec3f vec3fValue = std::get<osg::Vec3f>(parameterValue);
                        if (ImGui::ColorEdit3(parameterName.c_str(), vec3fValue.ptr(), ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR))
                            mMaterial->setParameter(parameterName, vec3fValue);
                        break;
                    }
                    case size_t(Shader::ParameterType::Vec4f):
                    {
                        osg::Vec4 vec4fValue = std::get<osg::Vec4f>(parameterValue);
                        if (ImGui::ColorEdit4(parameterName.c_str(), vec4fValue.ptr(), ImGuiColorEditFlags_Float | ImGuiColorEditFlags_HDR))
                            mMaterial->setParameter(parameterName, vec4fValue);
                        break;
                    }
                    case size_t(Shader::ParameterType::Texture2D):
                    {
                        Asset* textureAsset = std::get<Shader::Texture2DUnitPair>(parameterValue).first->getAsset();
                        if (AssetCombo<Texture2D>(parameterName.c_str(), &textureAsset))
                        {
                            mMaterial->setParameter(parameterName, textureAsset->getRootObject<Texture2D>());
                        }
                        break;
                    }
                    case size_t(Shader::ParameterType::Texture2DArray):
                    {
                        Asset* textureAsset = std::get<Shader::Texture2DArrayUnitPair>(parameterValue).first->getAsset();
                        if (AssetCombo<Texture2DArray>(parameterName.c_str(), &textureAsset))
                        {
                            mMaterial->setParameter(parameterName, textureAsset->getRootObject<Texture2DArray>());
                        }
                        break;
                    }
                    case size_t(Shader::ParameterType::Texture3D):
                    {
                        Asset* textureAsset = std::get<Shader::Texture3DUnitPair>(parameterValue).first->getAsset();
                        if (AssetCombo<Texture3D>(parameterName.c_str(), &textureAsset))
                        {
                            mMaterial->setParameter(parameterName, textureAsset->getRootObject<Texture3D>());
                        }
                        break;
                    }
                    case size_t(Shader::ParameterType::TextureCubemap):
                    {
                        Asset* textureAsset = std::get<Shader::TextureCubemapUnitPair>(parameterValue).first->getAsset();
                        if (AssetCombo<TextureCubemap>(parameterName.c_str(), &textureAsset))
                        {
                            mMaterial->setParameter(parameterName, textureAsset->getRootObject<TextureCubemap>());
                        }
                        break;
                    }
                    default:
                        break;
                    }

                    if (!materialParamEnable)
                        ImGui::EndDisabled();

                    ++paramId;
                }

                if (ImGui::Button("Save"))
                    mMaterialAsset->save();
            }
            ImGui::End();

            return mVisibility;
        }

        virtual bool isSingleton() const
        {
            return false;
        }

    protected:
        osg::ref_ptr<Asset> mMaterialAsset;
        osg::ref_ptr<Material> mMaterial;
        //osg::ref_ptr<osgViewer::View> mView;
        //osg::ref_ptr<Pipeline> mPipeline;
    };
}
