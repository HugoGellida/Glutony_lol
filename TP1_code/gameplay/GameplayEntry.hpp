#pragma once

#include <string>

#include <common/render/RenderUniformFactoryPlugin.hpp>

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
void registerGeneratedRenderUniformFactories();
void bootstrap();
void registerSceneScript(const std::string& assetPath, SceneScriptEntry entry);
void registerComponentScript(const std::string& assetPath, ComponentScriptStartEntry startEntry, ComponentScriptUpdateEntry updateEntry);
void registerRenderUniformFactory(const std::string& assetPath, render::UniformFactoryEntry entry, render::UniformFactoryIterationCountEntry iterationCountEntry);
void runSceneScript(Scene& scene);
void runPendingComponentScriptStarts(Scene& scene);
void runComponentScriptUpdates(Scene& scene, float deltaTime);
bool queryRenderUniformFactoryIterationCount(const std::string& assetPath, const render::UniformFactoryExecutionContext& context, int& iterationCountOut);
bool runRenderUniformFactory(const std::string& assetPath, const render::UniformFactoryExecutionContext& context, const render::UniformFactoryWriter& writer);
}