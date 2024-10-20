#include "Window.h"
#include "Widget.h"

namespace xxx::editor
{
    class ShaderEditor : public Window
    {
    public:
        ShaderEditor(Asset* shaderAsset) : Window(shaderAsset->getName() + "##" + shaderAsset->getPath()), mShaderAsset(shaderAsset)
        {
            if (!mShaderAsset->isLoaded())
                mShaderAsset->load();
            mShader = mShaderAsset->getRootObject<Shader>();
        }

        virtual bool draw() override
        {
            if (ImGui::Begin(mTitle.c_str(), &mVisibility))
            {
                // Parameters
                if (ImGui::CollapsingHeader("Parameters"))
                {
                    int paramId = 0;
                    const auto& shaderParameters = mShader->getParameters();
                    for (auto shaderParamIt = shaderParameters.begin(); shaderParamIt != shaderParameters.end();)
                    {
                        const std::string& parameterName = shaderParamIt->first;
                        const Shader::ParameterValue& parameterValue = shaderParamIt->second;

                        std::string idString = "shader_param_" + std::to_string(paramId);
                        ImGui::PushID(idString.c_str());
                        bool removeThisParameter = false;
                        if (ImGui::Button("x"))
                        {
                            removeThisParameter = true;
                        }
                        ImGui::PopID();
                        ImGui::SameLine();

                        switch (parameterValue.index())
                        {
                        case size_t(Shader::ParameterType::Bool):
                        {
                            bool boolValue = std::get<bool>(parameterValue);
                            if (ImGui::Checkbox(parameterName.c_str(), &boolValue))
                                mShader->setParameter(parameterName, boolValue);
                            break;
                        }
                        case size_t(Shader::ParameterType::Int):
                        {
                            int intValue = std::get<int>(parameterValue);
                            if (ImGui::DragInt(parameterName.c_str(), &intValue))
                                mShader->setParameter(parameterName, intValue);
                            break;
                        }
                        case size_t(Shader::ParameterType::Float):
                        {
                            float floatValue = std::get<float>(parameterValue);
                            if (ImGui::DragFloat(parameterName.c_str(), &floatValue))
                                mShader->setParameter(parameterName, floatValue);
                            break;
                        }
                        case size_t(Shader::ParameterType::Vec2f):
                        {
                            osg::Vec2f vec2fValue = std::get<osg::Vec2f>(parameterValue);
                            if (ImGui::DragFloat2(parameterName.c_str(), &vec2fValue.x()))
                                mShader->setParameter(parameterName, vec2fValue);
                            break;
                        }
                        case size_t(Shader::ParameterType::Vec3f):
                        {
                            osg::Vec3f vec3fValue = std::get<osg::Vec3f>(parameterValue);
                            if (ImGui::DragFloat3(parameterName.c_str(), &vec3fValue.x()))
                                mShader->setParameter(parameterName, vec3fValue);
                            break;
                        }
                        case size_t(Shader::ParameterType::Vec4f):
                        {
                            osg::Vec4 vec4fValue = std::get<osg::Vec4f>(parameterValue);
                            if (ImGui::DragFloat4(parameterName.c_str(), &vec4fValue.x()))
                                mShader->setParameter(parameterName, vec4fValue);
                            break;
                        }
                        case size_t(Shader::ParameterType::Texture):
                        {
                            const Shader::TextureAndUnit& textureAndUnit = std::get<Shader::TextureAndUnit>(parameterValue);
                            Asset* textureAsset = textureAndUnit.first->getAsset();
                            if (textureAsset)
                            {
                                if (AssetCombo<Texture>(parameterName.c_str(), &textureAsset))
                                {
                                    if (!textureAsset->isLoaded())
                                        textureAsset->load();
                                    mShader->setParameter(parameterName, textureAsset->getRootObject<Texture>());
                                }
                            }
                            break;
                        }
                        default:
                            break;
                        }

                        ++paramId;

                        if (removeThisParameter)
                            shaderParamIt = mShader->removeParameter(parameterName);
                        else
                            ++shaderParamIt;
                    }

                    // Add new parameter
                    static std::string parameterName;
                    static Shader::ParameterType parameterType;

                    if (ImGui::Button("+"))
                    {
                        addParameter(parameterName, parameterType);
                    }

                    float windowWidth = ImGui::GetContentRegionAvail().x;
                    float componentWidth = (windowWidth - ImGui::GetStyle().ItemSpacing.x - ImGui::GetItemRectSize().x) / 2;

                    ImGui::SameLine();
                    ImGui::PushItemWidth(componentWidth);
                    EnumCombo<Shader::ParameterType>("##Type", &parameterType); ImGui::SameLine();
                    ImGui::InputText("##Name", &parameterName);
                    ImGui::PopItemWidth();
                }

                if (ImGui::CollapsingHeader("Source"))
                {
                    ImGui::InputTextMultiline("##Source", &mShader->getSource(), ImVec2(ImGui::GetContentRegionAvail().x, 0), ImGuiInputTextFlags_AllowTabInput);
                }
                

                if (ImGui::Button("Apply"))
                    mShader->apply();

                ImGui::SameLine();

                if (ImGui::Button("Save"))
                    mShaderAsset->save();
            }
            ImGui::End();

            return mVisibility;
        }

        virtual bool isSingleton() const
        {
            return false;
        }

    protected:
        osg::ref_ptr<Asset> mShaderAsset;
        osg::ref_ptr<Shader> mShader;

        void addParameter(const std::string& name, Shader::ParameterType type)
        {
            switch (type)
            {
            case xxx::Shader::ParameterType::Bool:
                mShader->addParameter(name, false);
                break;
            case xxx::Shader::ParameterType::Int:
                mShader->addParameter(name, 0);
                break;
            case xxx::Shader::ParameterType::Float:
                mShader->addParameter(name, 0.0f);
                break;
            case xxx::Shader::ParameterType::Vec2f:
                mShader->addParameter(name, osg::Vec2f(0, 0));
                break;
            case xxx::Shader::ParameterType::Vec3f:
                mShader->addParameter(name, osg::Vec3f(0, 0, 0));
                break;
            case xxx::Shader::ParameterType::Vec4f:
                mShader->addParameter(name, osg::Vec4f(0, 0, 0, 0));
                break;
            case xxx::Shader::ParameterType::Texture:
            {
                Asset* defaultTextureAsset = AssetManager::get().getAsset("Engine/Texture/DefaultTexture");
                if (!defaultTextureAsset->isLoaded())
                    defaultTextureAsset->load();
                mShader->addParameter(name, defaultTextureAsset->getRootObject<Texture>());
                break;
            }
            default:
                break;
            }
        }
    };
}
