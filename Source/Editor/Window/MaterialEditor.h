#include "Window.h"
#include "Widget.h"

namespace xxx::editor
{
    class MaterialEditor : public Window
    {
    public:
        MaterialEditor(Asset* materialAsset) : Window(materialAsset->getName().c_str()), mMaterialAsset(materialAsset) {}

        virtual bool draw()
        {
            if (!mVisibility)
                return true;

            if (ImGui::Begin(mTitle.c_str()))
            {
                Material* material = mMaterialAsset->getRootObject<Material>();

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
                const Material::Parameters& materialParameters = material->getParameters();
                for (auto materialParamIt = materialParameters.begin(); materialParamIt != materialParameters.end(); ++materialParamIt)
                {
                    const std::string& parameterName = materialParamIt->first;
                    bool materialParamEnable = materialParamIt->second.second;
                    const Shader::ParameterValue& parameterValue = materialParamIt->second.first;

                    std::string idString = "material_param_" + std::to_string(paramId);
                    ImGui::PushID(idString.c_str());
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
                        Shader::TextureAndUnit textureAndUnit = std::get<Shader::TextureAndUnit>(parameterValue);
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

                    ++paramId;
                }

                if (ImGui::Button("Save"))
                    mMaterialAsset->save();
            }
            ImGui::End();

            return true;
        }

        virtual bool isSingleton() const
        {
            return false;
        }

    protected:
        osg::ref_ptr<Asset> mMaterialAsset;

    };
}
