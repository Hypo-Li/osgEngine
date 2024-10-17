#include "Window.h"
#include "Widget.h"

namespace xxx::editor
{
    class ShaderEditor : public Window
    {
    public:
        ShaderEditor(Asset* shaderAsset) : Window(shaderAsset->getName().c_str()), mShaderAsset(shaderAsset) {}

        virtual bool draw()
        {
            if (!mVisibility)
                return true;

            if (ImGui::Begin(mTitle.c_str()))
            {
                Shader* shader = mShaderAsset->getRootObject<Shader>();

                int paramId = 0;
                const auto& shaderParameters = shader->getParameters();
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
                    case size_t(Shader::ParameterIndex::Bool):
                    {
                        bool boolValue = std::get<bool>(parameterValue);
                        if (ImGui::Checkbox(parameterName.c_str(), &boolValue))
                            shader->setParameter(parameterName, boolValue);
                        break;
                    }
                    case size_t(Shader::ParameterIndex::Int):
                    {
                        int intValue = std::get<int>(parameterValue);
                        if (ImGui::DragInt(parameterName.c_str(), &intValue))
                            shader->setParameter(parameterName, intValue);
                        break;
                    }
                    case size_t(Shader::ParameterIndex::Float):
                    {
                        float floatValue = std::get<float>(parameterValue);
                        if (ImGui::DragFloat(parameterName.c_str(), &floatValue))
                            shader->setParameter(parameterName, floatValue);
                        break;
                    }
                    case size_t(Shader::ParameterIndex::Vec2f):
                    {
                        osg::Vec2f vec2fValue = std::get<osg::Vec2f>(parameterValue);
                        if (ImGui::DragFloat2(parameterName.c_str(), &vec2fValue.x()))
                            shader->setParameter(parameterName, vec2fValue);
                        break;
                    }
                    case size_t(Shader::ParameterIndex::Vec3f):
                    {
                        osg::Vec3f vec3fValue = std::get<osg::Vec3f>(parameterValue);
                        if (ImGui::DragFloat3(parameterName.c_str(), &vec3fValue.x()))
                            shader->setParameter(parameterName, vec3fValue);
                        break;
                    }
                    case size_t(Shader::ParameterIndex::Vec4f):
                    {
                        osg::Vec4 vec4fValue = std::get<osg::Vec4f>(parameterValue);
                        if (ImGui::DragFloat4(parameterName.c_str(), &vec4fValue.x()))
                            shader->setParameter(parameterName, vec4fValue);
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
                                shader->setParameter(parameterName, selectedAsset->getRootObject<Texture>());
                            }
                        }
                        break;
                    }
                    default:
                        break;
                    }

                    ++paramId;

                    if (removeThisParameter)
                        shaderParamIt = shader->removeParameter(parameterName);
                    else
                        ++shaderParamIt;
                }

                ImGui::InputTextMultiline("Source", &shader->getSource());

                if (ImGui::Button("Apply"))
                {
                    AssetManager::get().foreachAsset<Material>([shader](Asset* asset) {
                        Material* material = asset->getRootObject<Material>();
                        if (material->getShader() == shader)
                            material->syncWithShader();
                    });
                }
                if (ImGui::Button("Save"))
                    mShaderAsset->save();
            }
            ImGui::End();

            return true;
        }

        virtual bool isSingleton() const
        {
            return false;
        }

    protected:
        osg::ref_ptr<Asset> mShaderAsset;

    };
}
