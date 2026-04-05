#pragma once

#include <string>

class Scene;
class GameObject;

namespace asset
{
struct DataAssetDefinition;
}

namespace component
{
class ScriptComponent;
}

namespace gameplay
{
using SceneScriptEntry = void(*)(Scene& scene);
using ComponentScriptStartEntry = void(*)(Scene& scene, GameObject& gameObject, component::ScriptComponent& scriptComponent, asset::DataAssetDefinition* dataAssetDefinition);
using ComponentScriptUpdateEntry = void(*)(Scene& scene, GameObject& gameObject, component::ScriptComponent& scriptComponent, asset::DataAssetDefinition* dataAssetDefinition, float deltaTime);

void registerGeneratedSceneScripts();
void bootstrap();
void registerSceneScript(const std::string& assetPath, SceneScriptEntry entry);
void registerComponentScript(const std::string& assetPath, ComponentScriptStartEntry startEntry, ComponentScriptUpdateEntry updateEntry);
void runSceneScript(Scene& scene);
void runPendingComponentScriptStarts(Scene& scene);
void runComponentScriptUpdates(Scene& scene, float deltaTime);
}