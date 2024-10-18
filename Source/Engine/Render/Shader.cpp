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
            if (asset->isLoaded())
            {
                Material* material = asset->getRootObject<Material>();
                if (material->getShader() == this)
                    material->syncWithShader();
            }
        });
    }
}
