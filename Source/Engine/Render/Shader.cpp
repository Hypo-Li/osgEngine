#include "Shader.h"
#include "Material.h"

namespace xxx
{
    Shader::Shader()
    {

    }

    void Shader::apply() const
    {
        AssetManager::get().foreachAsset<Material>([this](Asset* asset) {
            if (asset->getState() == Asset::State::Loaded)
            {
                Material* material = asset->getRootObjectSafety<Material>();
                if (material->getShader() == this)
                    material->syncWithShader();
            }
        });
    }
}
