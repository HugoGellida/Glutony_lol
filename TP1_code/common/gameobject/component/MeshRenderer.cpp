#include "MeshRenderer.hpp"

#include "../GameObject.hpp"
#include "../../Scene.hpp"

namespace
{
bool assignMeshAsset(component::MeshRenderer& renderer, const std::string& assetPath)
{
    renderer.setMeshAssetPath(assetPath);

    GameObject* owner = renderer.getOwner();
    Scene* scene = owner != nullptr ? owner->getScene() : nullptr;
    if (scene == nullptr)
        return !assetPath.empty();

    component::Mesh* mesh = scene->resolveMeshAsset(assetPath);
    if (mesh == nullptr)
        return false;

    owner->setSharedComponent(mesh);
    renderer.setMesh(mesh);
    return true;
}

bool assignMaterialAsset(component::MeshRenderer& renderer, const std::string& assetPath)
{
    renderer.setMaterialAssetPath(assetPath);

    GameObject* owner = renderer.getOwner();
    Scene* scene = owner != nullptr ? owner->getScene() : nullptr;
    if (scene == nullptr)
        return !assetPath.empty();

    dataStruct::Material* material = scene->resolveMaterialAsset(assetPath);
    if (material == nullptr)
        return false;

    renderer.setMaterial(material);
    scene->refreshMaterialAsset(assetPath);
    return true;
}
}

namespace component
{
const component_meta::ComponentDescriptor& MeshRenderer::componentDescriptor()
{
    static const component_meta::ComponentDescriptor descriptor = []() {
        component_meta::ComponentDescriptor value;
        value.typeKey = "render.mesh_renderer";
        value.displayName = "Mesh Renderer";
        value.version = 1;
        value.factory = [](GameObject* parent) -> component::Component* {
            (void)parent;
            return new MeshRenderer(nullptr, nullptr);
        };
        value.fields = {
            {
                "mesh_asset",
                "Mesh Asset",
                component_meta::FieldKind::Asset,
                [](const component::Component& component) -> component_meta::SerializedValue {
                    return static_cast<const MeshRenderer&>(component).getMeshAssetPath();
                },
                [](component::Component& component, const component_meta::SerializedValue& value) -> bool {
                    const std::string* parsed = std::get_if<std::string>(&value);
                    if (parsed == nullptr || parsed->empty())
                        return false;

                    return assignMeshAsset(static_cast<MeshRenderer&>(component), *parsed);
                },
                {},
                component_meta::AssetReferenceKind::Mesh
            },
            {
                "material_asset",
                "Material Asset",
                component_meta::FieldKind::Asset,
                [](const component::Component& component) -> component_meta::SerializedValue {
                    return static_cast<const MeshRenderer&>(component).getMaterialAssetPath();
                },
                [](component::Component& component, const component_meta::SerializedValue& value) -> bool {
                    const std::string* parsed = std::get_if<std::string>(&value);
                    if (parsed == nullptr || parsed->empty())
                        return false;

                    return assignMaterialAsset(static_cast<MeshRenderer&>(component), *parsed);
                },
                {},
                component_meta::AssetReferenceKind::Material
            }
        };
        return value;
    }();

    static const bool registered = []() {
        component_meta::registerComponentDescriptor(descriptor);
        return true;
    }();
    (void)registered;
    return descriptor;
}
}
