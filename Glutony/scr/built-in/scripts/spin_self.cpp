#include <common/Scene.hpp>
#include <common/asset/DataAssetIO.hpp>
#include <common/gameobject/component/ScriptComponent.hpp>

#include <glm/glm.hpp>

#include <string>

namespace
{
asset::DataAssetNodeDefinition* findNodeByName(asset::DataAssetNodeDefinition& node, const std::string& name)
{
    if (node.name == name)
        return &node;

    for (asset::DataAssetNodeDefinition& child : node.children)
    {
        asset::DataAssetNodeDefinition* match = findNodeByName(child, name);
        if (match != nullptr)
            return match;
    }

    return nullptr;
}

asset::DataAssetNodeDefinition* findNodeByName(asset::DataAssetDefinition& definition, const std::string& name)
{
    for (asset::DataAssetNodeDefinition& child : definition.children)
    {
        asset::DataAssetNodeDefinition* match = findNodeByName(child, name);
        if (match != nullptr)
            return match;
    }

    return nullptr;
}

float readFloatNode(asset::DataAssetDefinition* definition, const std::string& name, float fallback)
{
    if (definition == nullptr)
        return fallback;

    asset::DataAssetNodeDefinition* node = findNodeByName(*definition, name);
    return node != nullptr && node->kind == asset::DataAssetValueKind::Float ? node->floatValue : fallback;
}

void writeFloatNode(asset::DataAssetDefinition* definition, const std::string& name, float value)
{
    if (definition == nullptr)
        return;

    asset::DataAssetNodeDefinition* node = findNodeByName(*definition, name);
    if (node != nullptr && node->kind == asset::DataAssetValueKind::Float)
        node->floatValue = value;
}

void writeStringNode(asset::DataAssetDefinition* definition, const std::string& name, const std::string& value)
{
    if (definition == nullptr)
        return;

    asset::DataAssetNodeDefinition* node = findNodeByName(*definition, name);
    if (node != nullptr && node->kind == asset::DataAssetValueKind::String)
        node->stringValue = value;
}
}

void spinSelfStart(Scene& scene, GameObject& gameObject, component::ScriptComponent& scriptComponent, asset::DataAssetDefinition* dataAssetDefinition)
{
    (void)scene;
    (void)scriptComponent;

    writeStringNode(dataAssetDefinition, "object_name", gameObject.getName());
    writeFloatNode(dataAssetDefinition, "current_local_yaw", gameObject.transform.getRotation().y);
}

void spinSelfUpdate(Scene& scene, GameObject& gameObject, component::ScriptComponent& scriptComponent, asset::DataAssetDefinition* dataAssetDefinition, float deltaTime)
{
    (void)scene;
    (void)scriptComponent;

    const float speedDegreesPerSecond = readFloatNode(dataAssetDefinition, "speed_deg_per_sec", 90.0f);
    glm::vec3 rotation = gameObject.transform.getRotation();
    rotation.y += speedDegreesPerSecond * deltaTime;
    gameObject.transform.setRotation(rotation);

    writeFloatNode(dataAssetDefinition, "current_local_yaw", rotation.y);
}