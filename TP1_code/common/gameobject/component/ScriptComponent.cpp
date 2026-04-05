#include "ScriptComponent.hpp"

#include "Mesh.hpp"

#include "../../asset/AssetManager.hpp"

namespace component
{
void ScriptComponent::setScriptAssetPath(const std::string& assetPath)
{
    const std::string normalizedPath = asset::AssetManager::normalizeRelativePath(assetPath);
    if (m_scriptAssetPath == normalizedPath)
        return;

    m_scriptAssetPath = normalizedPath;
    resetRuntimeState();
}

void ScriptComponent::setDataAssetPath(const std::string& assetPath)
{
    m_dataAssetPath = asset::AssetManager::normalizeRelativePath(assetPath);
}

asset::DataAssetDefinition* ScriptComponent::loadDataAssetDefinition() const
{
    if (m_dataAssetPath.empty())
        return nullptr;

    return asset::AssetManager::instance().loadDataAssetDefinition(m_dataAssetPath);
}

const component_meta::ComponentDescriptor& ScriptComponent::componentDescriptor()
{
    static const component_meta::ComponentDescriptor descriptor = []() {
        component_meta::ComponentDescriptor value;
        value.typeKey = "gameplay.script";
        value.displayName = "Script";
        value.version = 1;
        value.factory = [](GameObject* parent) -> component::Component* {
            (void)parent;
            return new ScriptComponent();
        };
        value.fields = {
            {
                "scriptAsset",
                "Script",
                component_meta::FieldKind::Asset,
                [](const component::Component& component) -> component_meta::SerializedValue {
                    return static_cast<const ScriptComponent&>(component).getScriptAssetPath();
                },
                [](component::Component& component, const component_meta::SerializedValue& value) -> bool {
                    const std::string* parsed = std::get_if<std::string>(&value);
                    if (parsed == nullptr)
                        return false;

                    ScriptComponent& scriptComponent = static_cast<ScriptComponent&>(component);
                    if (!parsed->empty() && !asset::AssetManager::hasExtension(*parsed, ".component_script"))
                        return false;

                    scriptComponent.setScriptAssetPath(*parsed);
                    return true;
                },
                {},
                component_meta::AssetReferenceKind::ComponentScript
            },
            {
                "dataAsset",
                "DataAsset",
                component_meta::FieldKind::Asset,
                [](const component::Component& component) -> component_meta::SerializedValue {
                    return static_cast<const ScriptComponent&>(component).getDataAssetPath();
                },
                [](component::Component& component, const component_meta::SerializedValue& value) -> bool {
                    const std::string* parsed = std::get_if<std::string>(&value);
                    if (parsed == nullptr)
                        return false;

                    ScriptComponent& scriptComponent = static_cast<ScriptComponent&>(component);
                    if (!parsed->empty() && !asset::AssetManager::hasExtension(*parsed, ".data"))
                        return false;

                    scriptComponent.setDataAssetPath(*parsed);
                    return true;
                },
                {},
                component_meta::AssetReferenceKind::Data
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