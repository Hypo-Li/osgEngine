#include "Engine/Core/Entity.h"
#include "Engine/Core/Component.h"
#include "Engine/Render/Shader.h"
#include "Engine/Render/Mesh.h"
#include "Engine/Core/Asset.h"
#include "Engine/Core/Prefab.h"
#include "Engine/Core/AssetManager.h"
#include "Engine/Render/Pipeline.h"
#include "Engine/Component/MeshRenderer.h"

#include "DebugCallback.h"

#include <osg/Vec2f>
#include <osg/Vec3f>
#include <osg/Vec4f>
#include <osgViewer/Viewer>
#include <osgGA/TrackballManipulator>

#include <iostream>

using namespace xxx;
using namespace xxx::refl;

int main()
{
    AssetManager& am = AssetManager::get();

    /*Asset* materialAsset = am.createAsset(nullptr, "Engine/Material/DefaultMaterial");
    materialAsset->load();
    Material* material = materialAsset->getRootObject<Material>();
    
    Mesh* mesh = new Mesh(TEMP_DIR "cube.obj");
    mesh->setDefaultMaterial(0, material);
    am.createAsset(mesh, "Engine/Mesh/Cube")->save();*/

    Texture2D* texture = new Texture2D(TEMP_DIR "white.png");
    am.createAsset(texture, "Engine/Texture/DefaultTexture")->save();

    /*Entity* entity = new Entity("TestEntity");
    MeshRenderer* meshRenderer = entity->addComponent<MeshRenderer>();
    meshRenderer->setMesh(mesh);*/

    return 0;
}
