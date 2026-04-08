#pragma once

#include <common/Scene.hpp>

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace scene_serialization
{
struct GameObjectSnapshot
{
    int id = -1;
    std::string name;
    int parentId = -1;
    glm::vec3 position{0.0f, 0.0f, 0.0f};
    glm::vec3 rotation{0.0f, 0.0f, 0.0f};
    glm::vec3 scale{1.0f, 1.0f, 1.0f};
    std::string meshAssetPath;
    std::string materialAssetPath;
    std::vector<component_meta::ComponentSnapshot> components;
};

struct SceneSnapshot
{
    int version = 3;
    int nextGameObjectId = 1;
    bool physicsSimulationEnabled = false;
    int selectedGameObjectId = -1;
    std::string sceneScriptAssetPath;
    std::string dataAssetPath;
    render::DirectionalLightSettings directionalLight;
    Camera camera;
    std::vector<std::string> meshAssets;
    std::vector<std::string> shaderAssets;
    std::vector<std::string> materialAssets;
    std::vector<render::SceneRenderTargetSettings> renderTargets;
    std::vector<GameObjectSnapshot> gameObjects;
};

namespace detail
{
template <typename T>
const T* findComponent(const GameObject& gameObject)
{
    for (size_t componentIndex = 0; componentIndex < gameObject.getComponentCount(); ++componentIndex)
    {
        const component::Component* component = gameObject.getComponentAt(componentIndex);
        if (const T* casted = dynamic_cast<const T*>(component))
            return casted;
    }

    return nullptr;
}

inline void appendUnique(std::vector<std::string>& values, const std::string& value)
{
    if (value.empty())
        return;

    if (std::find(values.begin(), values.end(), value) == values.end())
        values.push_back(value);
}

inline void writeVec3(std::ostream& output, const glm::vec3& value)
{
    output << value.x << ' ' << value.y << ' ' << value.z;
}

inline bool readVec3(std::istream& input, glm::vec3& value)
{
    return static_cast<bool>(input >> value.x >> value.y >> value.z);
}

inline void writeSerializedValue(std::ostream& output, const component_meta::SerializedValue& value)
{
    std::visit([&output](const auto& storedValue) {
        using StoredType = std::decay_t<decltype(storedValue)>;

        if constexpr (std::is_same_v<StoredType, bool>)
        {
            output << "BOOL " << (storedValue ? 1 : 0);
        }
        else if constexpr (std::is_same_v<StoredType, int>)
        {
            output << "INT " << storedValue;
        }
        else if constexpr (std::is_same_v<StoredType, float>)
        {
            output << "FLOAT " << storedValue;
        }
        else if constexpr (std::is_same_v<StoredType, glm::vec3>)
        {
            output << "VEC3 ";
            writeVec3(output, storedValue);
        }
        else
        {
            output << "STRING " << std::quoted(storedValue);
        }
    }, value);
}

inline bool readSerializedValue(std::istream& input, component_meta::SerializedValue& value)
{
    std::string typeToken;
    if (!(input >> typeToken))
        return false;

    if (typeToken == "BOOL")
    {
        int numericValue = 0;
        if (!(input >> numericValue))
            return false;
        value = (numericValue != 0);
        return true;
    }

    if (typeToken == "INT")
    {
        int numericValue = 0;
        if (!(input >> numericValue))
            return false;
        value = numericValue;
        return true;
    }

    if (typeToken == "FLOAT")
    {
        float numericValue = 0.0f;
        if (!(input >> numericValue))
            return false;
        value = numericValue;
        return true;
    }

    if (typeToken == "VEC3")
    {
        glm::vec3 vecValue(0.0f, 0.0f, 0.0f);
        if (!readVec3(input, vecValue))
            return false;
        value = vecValue;
        return true;
    }

    if (typeToken == "STRING")
    {
        std::string stringValue;
        if (!(input >> std::quoted(stringValue)))
            return false;
        value = stringValue;
        return true;
    }

    return false;
}

inline bool saveSnapshotToStream(std::ostream& output, const SceneSnapshot& snapshot)
{
    output << "SCENE_SNAPSHOT " << snapshot.version << '\n';
    output << "NEXT_GAMEOBJECT_ID " << snapshot.nextGameObjectId << '\n';
    output << "PHYSICS " << (snapshot.physicsSimulationEnabled ? 1 : 0) << '\n';
    output << "SELECTED_ID " << snapshot.selectedGameObjectId << '\n';
    output << "SCENE_SCRIPT_ASSET " << std::quoted(snapshot.sceneScriptAssetPath) << '\n';
    output << "DATA_ASSET " << std::quoted(snapshot.dataAssetPath) << '\n';
    output << "DIRECTIONAL_LIGHT_ENABLED " << (snapshot.directionalLight.enabled ? 1 : 0) << '\n';
    output << "DIRECTIONAL_LIGHT_DIRECTION ";
    writeVec3(output, snapshot.directionalLight.direction);
    output << '\n';
    output << "DIRECTIONAL_LIGHT_COLOR ";
    writeVec3(output, snapshot.directionalLight.color);
    output << '\n';
    output << "DIRECTIONAL_LIGHT_INTENSITY " << snapshot.directionalLight.intensity << '\n';
    output << "CAMERA_POSITION ";
    writeVec3(output, snapshot.camera.m_position);
    output << '\n';
    output << "CAMERA_ORIENTATION ";
    writeVec3(output, snapshot.camera.m_orientation);
    output << '\n';
    output << "CAMERA_FOV " << snapshot.camera.m_fov << '\n';
    output << "CAMERA_ASPECT " << snapshot.camera.m_aspectRatio << '\n';
    output << "CAMERA_NEAR " << snapshot.camera.m_nearPlane << '\n';
    output << "CAMERA_FAR " << snapshot.camera.m_farPlane << '\n';

    auto writeAssetSection = [&output](const char* sectionName, const std::vector<std::string>& values) {
        output << sectionName << ' ' << values.size() << '\n';
        for (const std::string& value : values)
            output << "ASSET " << std::quoted(value) << '\n';
    };

    writeAssetSection("MESH_ASSETS", snapshot.meshAssets);
    writeAssetSection("SHADER_ASSETS", snapshot.shaderAssets);
    writeAssetSection("MATERIAL_ASSETS", snapshot.materialAssets);
    output << "RENDER_TARGETS " << snapshot.renderTargets.size() << '\n';
    for (const render::SceneRenderTargetSettings& renderTarget : snapshot.renderTargets)
    {
        output << "RENDER_TARGET "
               << std::quoted(renderTarget.name) << ' '
               << renderTarget.width << ' '
               << renderTarget.height << ' '
               << render::renderTargetFormatName(renderTarget.format) << '\n';
    }

    output << "GAME_OBJECTS " << snapshot.gameObjects.size() << '\n';
    for (const GameObjectSnapshot& gameObject : snapshot.gameObjects)
    {
        output << "BEGIN_GAME_OBJECT\n";
        output << "ID " << gameObject.id << '\n';
        output << "NAME " << std::quoted(gameObject.name) << '\n';
        output << "PARENT_ID " << gameObject.parentId << '\n';
        output << "POSITION ";
        writeVec3(output, gameObject.position);
        output << '\n';
        output << "ROTATION ";
        writeVec3(output, gameObject.rotation);
        output << '\n';
        output << "SCALE ";
        writeVec3(output, gameObject.scale);
        output << '\n';
        output << "MESH_ASSET " << std::quoted(gameObject.meshAssetPath) << '\n';
        output << "MATERIAL_ASSET " << std::quoted(gameObject.materialAssetPath) << '\n';
        output << "COMPONENTS " << gameObject.components.size() << '\n';

        for (const component_meta::ComponentSnapshot& component : gameObject.components)
        {
            output << "BEGIN_COMPONENT\n";
            output << "TYPE " << std::quoted(component.typeKey) << '\n';
            output << "VERSION " << component.version << '\n';
            output << "FIELDS " << component.fields.size() << '\n';
            for (const component_meta::SerializedField& field : component.fields)
            {
                output << "FIELD " << std::quoted(field.key) << ' ';
                writeSerializedValue(output, field.value);
                output << '\n';
            }
            output << "END_COMPONENT\n";
        }

        output << "END_GAME_OBJECT\n";
    }

    output << "END_SCENE\n";
    return static_cast<bool>(output);
}

inline bool saveGameObjectSnapshotToStream(std::ostream& output, const GameObjectSnapshot& gameObject)
{
    output << "BEGIN_GAME_OBJECT\n";
    output << "ID " << gameObject.id << '\n';
    output << "NAME " << std::quoted(gameObject.name) << '\n';
    output << "PARENT_ID " << gameObject.parentId << '\n';
    output << "POSITION ";
    writeVec3(output, gameObject.position);
    output << '\n';
    output << "ROTATION ";
    writeVec3(output, gameObject.rotation);
    output << '\n';
    output << "SCALE ";
    writeVec3(output, gameObject.scale);
    output << '\n';
    output << "MESH_ASSET " << std::quoted(gameObject.meshAssetPath) << '\n';
    output << "MATERIAL_ASSET " << std::quoted(gameObject.materialAssetPath) << '\n';
    output << "COMPONENTS " << gameObject.components.size() << '\n';

    for (const component_meta::ComponentSnapshot& component : gameObject.components)
    {
        output << "BEGIN_COMPONENT\n";
        output << "TYPE " << std::quoted(component.typeKey) << '\n';
        output << "VERSION " << component.version << '\n';
        output << "FIELDS " << component.fields.size() << '\n';
        for (const component_meta::SerializedField& field : component.fields)
        {
            output << "FIELD " << std::quoted(field.key) << ' ';
            writeSerializedValue(output, field.value);
            output << '\n';
        }
        output << "END_COMPONENT\n";
    }

    output << "END_GAME_OBJECT\n";
    return static_cast<bool>(output);
}

inline bool loadSnapshotFromStream(std::istream& input, SceneSnapshot& snapshot)
{
    std::string token;
    if (!(input >> token) || token != "SCENE_SNAPSHOT")
        return false;
    if (!(input >> snapshot.version))
        return false;

    auto readAssetSection = [&input](const std::string& expectedToken, std::vector<std::string>& values) -> bool {
        std::string token;
        size_t count = 0;
        if (!(input >> token >> count) || token != expectedToken)
            return false;

        values.clear();
        values.reserve(count);
        for (size_t index = 0; index < count; ++index)
        {
            std::string assetToken;
            std::string value;
            if (!(input >> assetToken >> std::quoted(value)) || assetToken != "ASSET")
                return false;
            values.push_back(value);
        }

        return true;
    };

    int physicsEnabled = 0;
    if (!(input >> token >> snapshot.nextGameObjectId) || token != "NEXT_GAMEOBJECT_ID")
        return false;
    if (!(input >> token >> physicsEnabled) || token != "PHYSICS")
        return false;
    snapshot.physicsSimulationEnabled = (physicsEnabled != 0);
    if (!(input >> token >> snapshot.selectedGameObjectId) || token != "SELECTED_ID")
        return false;

    snapshot.sceneScriptAssetPath.clear();
    snapshot.dataAssetPath.clear();
    snapshot.directionalLight = render::DirectionalLightSettings();

    if (!(input >> token))
        return false;

    if (token == "SCENE_SCRIPT_ASSET")
    {
        if (!(input >> std::quoted(snapshot.sceneScriptAssetPath)))
            return false;
        if (!(input >> token))
            return false;
    }

    if (token == "DATA_ASSET")
    {
        if (!(input >> std::quoted(snapshot.dataAssetPath)))
            return false;
        if (!(input >> token))
            return false;
    }

    if (token == "DIRECTIONAL_LIGHT_ENABLED")
    {
        int enabled = 0;
        if (!(input >> enabled))
            return false;
        snapshot.directionalLight.enabled = (enabled != 0);

        if (!(input >> token) || token != "DIRECTIONAL_LIGHT_DIRECTION" || !readVec3(input, snapshot.directionalLight.direction))
            return false;
        if (!(input >> token) || token != "DIRECTIONAL_LIGHT_COLOR" || !readVec3(input, snapshot.directionalLight.color))
            return false;
        if (!(input >> token) || token != "DIRECTIONAL_LIGHT_INTENSITY" || !(input >> snapshot.directionalLight.intensity))
            return false;
        if (!(input >> token))
            return false;
    }

    if (token != "CAMERA_POSITION" || !readVec3(input, snapshot.camera.m_position))
        return false;
    if (!(input >> token) || token != "CAMERA_ORIENTATION" || !readVec3(input, snapshot.camera.m_orientation))
        return false;
    if (!(input >> token >> snapshot.camera.m_fov) || token != "CAMERA_FOV")
        return false;
    if (!(input >> token >> snapshot.camera.m_aspectRatio) || token != "CAMERA_ASPECT")
        return false;
    if (!(input >> token >> snapshot.camera.m_nearPlane) || token != "CAMERA_NEAR")
        return false;
    if (!(input >> token >> snapshot.camera.m_farPlane) || token != "CAMERA_FAR")
        return false;

    if (!readAssetSection("MESH_ASSETS", snapshot.meshAssets))
        return false;
    if (!readAssetSection("SHADER_ASSETS", snapshot.shaderAssets))
        return false;
    if (!readAssetSection("MATERIAL_ASSETS", snapshot.materialAssets))
        return false;

    snapshot.renderTargets.clear();
    if (!(input >> token))
        return false;

    if (token == "RENDER_TARGETS")
    {
        size_t renderTargetCount = 0;
        if (!(input >> renderTargetCount))
            return false;

        snapshot.renderTargets.reserve(renderTargetCount);
        for (size_t renderTargetIndex = 0; renderTargetIndex < renderTargetCount; ++renderTargetIndex)
        {
            std::string entryToken;
            std::string formatToken;
            render::SceneRenderTargetSettings renderTarget;
            if (!(input >> entryToken >> std::quoted(renderTarget.name) >> renderTarget.width >> renderTarget.height >> formatToken) || entryToken != "RENDER_TARGET")
                return false;
            if (!render::parseRenderTargetFormat(formatToken, renderTarget.format))
                return false;
            snapshot.renderTargets.push_back(renderTarget);
        }

        if (!(input >> token))
            return false;
    }

    size_t gameObjectCount = 0;
    if (token != "GAME_OBJECTS" || !(input >> gameObjectCount))
        return false;

    snapshot.gameObjects.clear();
    snapshot.gameObjects.reserve(gameObjectCount);

    for (size_t gameObjectIndex = 0; gameObjectIndex < gameObjectCount; ++gameObjectIndex)
    {
        GameObjectSnapshot gameObject;
        if (!(input >> token) || token != "BEGIN_GAME_OBJECT")
            return false;
        if (!(input >> token >> gameObject.id) || token != "ID")
            return false;
        if (!(input >> token >> std::quoted(gameObject.name)) || token != "NAME")
            return false;
        if (!(input >> token >> gameObject.parentId) || token != "PARENT_ID")
            return false;
        if (!(input >> token) || token != "POSITION" || !readVec3(input, gameObject.position))
            return false;
        if (!(input >> token) || token != "ROTATION" || !readVec3(input, gameObject.rotation))
            return false;
        if (!(input >> token) || token != "SCALE" || !readVec3(input, gameObject.scale))
            return false;
        if (!(input >> token >> std::quoted(gameObject.meshAssetPath)) || token != "MESH_ASSET")
            return false;
        if (!(input >> token >> std::quoted(gameObject.materialAssetPath)) || token != "MATERIAL_ASSET")
            return false;

        size_t componentCount = 0;
        if (!(input >> token >> componentCount) || token != "COMPONENTS")
            return false;
        gameObject.components.reserve(componentCount);

        for (size_t componentIndex = 0; componentIndex < componentCount; ++componentIndex)
        {
            component_meta::ComponentSnapshot componentSnapshot;
            if (!(input >> token) || token != "BEGIN_COMPONENT")
                return false;
            if (!(input >> token >> std::quoted(componentSnapshot.typeKey)) || token != "TYPE")
                return false;
            if (!(input >> token >> componentSnapshot.version) || token != "VERSION")
                return false;

            size_t fieldCount = 0;
            if (!(input >> token >> fieldCount) || token != "FIELDS")
                return false;
            componentSnapshot.fields.reserve(fieldCount);

            for (size_t fieldIndex = 0; fieldIndex < fieldCount; ++fieldIndex)
            {
                component_meta::SerializedField field;
                if (!(input >> token >> std::quoted(field.key)) || token != "FIELD")
                    return false;
                if (!readSerializedValue(input, field.value))
                    return false;
                componentSnapshot.fields.push_back(field);
            }

            if (!(input >> token) || token != "END_COMPONENT")
                return false;
            gameObject.components.push_back(componentSnapshot);
        }

        if (!(input >> token) || token != "END_GAME_OBJECT")
            return false;
        snapshot.gameObjects.push_back(gameObject);
    }

    return (input >> token) && token == "END_SCENE";
}

inline bool loadGameObjectSnapshotFromStream(std::istream& input, GameObjectSnapshot& gameObject)
{
    std::string token;
    if (!(input >> token) || token != "BEGIN_GAME_OBJECT")
        return false;
    if (!(input >> token >> gameObject.id) || token != "ID")
        return false;
    if (!(input >> token >> std::quoted(gameObject.name)) || token != "NAME")
        return false;
    if (!(input >> token >> gameObject.parentId) || token != "PARENT_ID")
        return false;
    if (!(input >> token) || token != "POSITION" || !readVec3(input, gameObject.position))
        return false;
    if (!(input >> token) || token != "ROTATION" || !readVec3(input, gameObject.rotation))
        return false;
    if (!(input >> token) || token != "SCALE" || !readVec3(input, gameObject.scale))
        return false;
    if (!(input >> token >> std::quoted(gameObject.meshAssetPath)) || token != "MESH_ASSET")
        return false;
    if (!(input >> token >> std::quoted(gameObject.materialAssetPath)) || token != "MATERIAL_ASSET")
        return false;

    size_t componentCount = 0;
    if (!(input >> token >> componentCount) || token != "COMPONENTS")
        return false;
    gameObject.components.clear();
    gameObject.components.reserve(componentCount);

    for (size_t componentIndex = 0; componentIndex < componentCount; ++componentIndex)
    {
        component_meta::ComponentSnapshot componentSnapshot;
        if (!(input >> token) || token != "BEGIN_COMPONENT")
            return false;
        if (!(input >> token >> std::quoted(componentSnapshot.typeKey)) || token != "TYPE")
            return false;
        if (!(input >> token >> componentSnapshot.version) || token != "VERSION")
            return false;

        size_t fieldCount = 0;
        if (!(input >> token >> fieldCount) || token != "FIELDS")
            return false;
        componentSnapshot.fields.reserve(fieldCount);

        for (size_t fieldIndex = 0; fieldIndex < fieldCount; ++fieldIndex)
        {
            component_meta::SerializedField field;
            if (!(input >> token >> std::quoted(field.key)) || token != "FIELD")
                return false;
            if (!readSerializedValue(input, field.value))
                return false;
            componentSnapshot.fields.push_back(field);
        }

        if (!(input >> token) || token != "END_COMPONENT")
            return false;
        gameObject.components.push_back(componentSnapshot);
    }

    return (input >> token) && token == "END_GAME_OBJECT";
}
}

inline GameObjectSnapshot captureGameObject(const GameObject& gameObject)
{
    GameObjectSnapshot snapshot;
    snapshot.id = gameObject.getId();
    snapshot.name = gameObject.getName();
    if (const Transform* parent = gameObject.transform.getParent())
    {
        const GameObject* parentObject = parent->getGameObject();
        snapshot.parentId = parentObject != nullptr ? parentObject->getId() : -1;
    }
    snapshot.position = gameObject.transform.getPosition();
    snapshot.rotation = gameObject.transform.getRotation();
    snapshot.scale = gameObject.transform.getScale();

    if (const component::MeshRenderer* meshRenderer = detail::findComponent<component::MeshRenderer>(gameObject))
    {
        snapshot.meshAssetPath = meshRenderer->getMeshAssetPath();
        snapshot.materialAssetPath = meshRenderer->getMaterialAssetPath();
    }
    else if (const component::Mesh* mesh = detail::findComponent<component::Mesh>(gameObject))
    {
        snapshot.meshAssetPath = mesh->getAssetPath();
    }

    for (size_t componentIndex = 0; componentIndex < gameObject.getComponentCount(); ++componentIndex)
    {
        const component::Component* component = gameObject.getComponentAt(componentIndex);
        if (component == nullptr)
            continue;
        if (dynamic_cast<const component::Mesh*>(component) != nullptr)
            continue;

        const std::optional<component_meta::ComponentSnapshot> componentSnapshot = component_meta::trySerializeComponent(*component);
        if (componentSnapshot.has_value())
            snapshot.components.push_back(*componentSnapshot);
    }

    return snapshot;
}

inline bool applyGameObjectSnapshot(Scene& scene, const GameObjectSnapshot& snapshot)
{
    GameObject* gameObject = scene.getGameObjectById(snapshot.id);
    if (gameObject == nullptr)
        return false;

    gameObject->setName(snapshot.name);
    gameObject->transform.setPosition(snapshot.position);
    gameObject->transform.setRotation(snapshot.rotation);
    gameObject->transform.setScale(snapshot.scale);

    const bool wantsMeshComponent = !snapshot.meshAssetPath.empty();
    const bool wantsMeshRenderer = wantsMeshComponent && !snapshot.materialAssetPath.empty();

    for (size_t componentIndex = gameObject->getComponentCount(); componentIndex > 0; --componentIndex)
    {
        component::Component* component = gameObject->getComponentAt(componentIndex - 1);
        if (component == nullptr)
            continue;

        if (dynamic_cast<component::Mesh*>(component) != nullptr)
        {
            if (!wantsMeshComponent)
                gameObject->removeComponentAt(componentIndex - 1);
            continue;
        }

        const component_meta::ComponentDescriptor* descriptor = component->getComponentDescriptor();
        if (descriptor == nullptr)
            continue;

        if (descriptor->typeKey == component::MeshRenderer::componentDescriptor().typeKey)
        {
            if (!wantsMeshRenderer)
                gameObject->removeComponentAt(componentIndex - 1);
            continue;
        }

        bool foundInSnapshot = false;
        for (const component_meta::ComponentSnapshot& componentSnapshot : snapshot.components)
        {
            if (componentSnapshot.typeKey == descriptor->typeKey)
            {
                foundInSnapshot = true;
                break;
            }
        }

        if (!foundInSnapshot)
            gameObject->removeComponentAt(componentIndex - 1);
    }

    if (wantsMeshComponent)
    {
        component::Mesh* mesh = scene.resolveMeshAsset(snapshot.meshAssetPath);
        if (mesh != nullptr)
            gameObject->setSharedComponent(mesh);
    }

    if (wantsMeshRenderer)
    {
        component::Mesh* mesh = scene.resolveMeshAsset(snapshot.meshAssetPath);
        dataStruct::Material* material = scene.resolveMaterialAsset(snapshot.materialAssetPath);
        if (mesh != nullptr && material != nullptr)
        {
            component::MeshRenderer* meshRenderer = gameObject->getComponent<component::MeshRenderer>();
            if (meshRenderer == nullptr)
            {
                meshRenderer = new component::MeshRenderer(mesh, material);
                gameObject->addComponent(meshRenderer);
            }
            else
            {
                meshRenderer->setMesh(mesh);
                meshRenderer->setMaterial(material);
            }

            meshRenderer->setMeshAssetPath(snapshot.meshAssetPath);
            meshRenderer->setMaterialAssetPath(snapshot.materialAssetPath);
        }
    }

    for (const component_meta::ComponentSnapshot& componentSnapshot : snapshot.components)
    {
        component::Component* matchingComponent = nullptr;
        for (size_t componentIndex = 0; componentIndex < gameObject->getComponentCount(); ++componentIndex)
        {
            component::Component* component = gameObject->getComponentAt(componentIndex);
            if (component == nullptr)
                continue;

            const component_meta::ComponentDescriptor* descriptor = component->getComponentDescriptor();
            if (descriptor != nullptr && descriptor->typeKey == componentSnapshot.typeKey)
            {
                matchingComponent = component;
                break;
            }
        }

        if (matchingComponent != nullptr)
        {
            if (!component_meta::applyComponentSnapshot(*matchingComponent, componentSnapshot))
                return false;
            continue;
        }

        std::unique_ptr<component::Component> createdComponent = component_meta::createComponentFromSnapshot(componentSnapshot, gameObject);
        if (!createdComponent)
            return false;
        gameObject->addComponent(createdComponent.release());
    }

    if (physics::RigidBody* rigidBody = gameObject->getComponent<physics::RigidBody>())
        rigidBody->RefreshSerializedState();

    return true;
}

inline bool mergeSceneSnapshot(Scene& scene, const SceneSnapshot& snapshot)
{
    scene.setSceneScriptAssetPath(snapshot.sceneScriptAssetPath);
    scene.setDataAssetPath(snapshot.dataAssetPath);
    scene.setDirectionalLightSettings(snapshot.directionalLight);
    scene.setSavedRenderTargetSettings(snapshot.renderTargets);

    for (const std::string& path : snapshot.shaderAssets)
        scene.resolveShaderAsset(path);
    for (const std::string& path : snapshot.meshAssets)
        scene.resolveMeshAsset(path);
    for (const std::string& path : snapshot.materialAssets)
        scene.resolveMaterialAsset(path);

    std::unordered_set<int> snapshotIds;
    snapshotIds.reserve(snapshot.gameObjects.size());
    for (const GameObjectSnapshot& gameObjectSnapshot : snapshot.gameObjects)
        snapshotIds.insert(gameObjectSnapshot.id);

    std::vector<int> idsToRemove;
    idsToRemove.reserve(scene.getGameObjectCount());
    for (size_t index = 0; index < scene.getGameObjectCount(); ++index)
    {
        const GameObject* gameObject = scene.getGameObject(index);
        if (gameObject != nullptr && snapshotIds.find(gameObject->getId()) == snapshotIds.end())
            idsToRemove.push_back(gameObject->getId());
    }

    for (int id : idsToRemove)
        scene.removeGameObject(id);

    for (const GameObjectSnapshot& gameObjectSnapshot : snapshot.gameObjects)
    {
        if (scene.getGameObjectById(gameObjectSnapshot.id) == nullptr)
            scene.addGameObject(gameObjectSnapshot.name, gameObjectSnapshot.id);
    }

    for (const GameObjectSnapshot& gameObjectSnapshot : snapshot.gameObjects)
    {
        if (!applyGameObjectSnapshot(scene, gameObjectSnapshot))
            return false;
    }

    for (const GameObjectSnapshot& gameObjectSnapshot : snapshot.gameObjects)
    {
        GameObject* gameObject = scene.getGameObjectById(gameObjectSnapshot.id);
        if (gameObject == nullptr)
            return false;

        GameObject* currentParent = nullptr;
        if (const Transform* parentTransform = gameObject->transform.getParent())
            currentParent = parentTransform->getGameObject();

        if (gameObjectSnapshot.parentId < 0)
        {
            if (currentParent != nullptr)
            {
                currentParent->transform.detachChild(&gameObject->transform);
                gameObject->transform.removeParent();
            }
            continue;
        }

        GameObject* desiredParent = scene.getGameObjectById(gameObjectSnapshot.parentId);
        if (desiredParent == nullptr)
            return false;

        if (currentParent == desiredParent)
            continue;

        if (currentParent != nullptr)
        {
            currentParent->transform.detachChild(&gameObject->transform);
            gameObject->transform.removeParent();
        }

        gameObject->setParent(desiredParent);
    }

    scene.getCamera() = snapshot.camera;
    scene.setSelectedGameObjectById(snapshot.selectedGameObjectId);
    scene.setNextGameObjectId(snapshot.nextGameObjectId);
    scene.setPhysicsSimulationEnabled(snapshot.physicsSimulationEnabled);
    return true;
}

inline SceneSnapshot captureScene(const Scene& scene)
{
    SceneSnapshot snapshot;
    snapshot.nextGameObjectId = scene.getNextGameObjectId();
    snapshot.physicsSimulationEnabled = scene.isPhysicsSimulationEnabled();
    snapshot.selectedGameObjectId = scene.getSelectedGameObject() != nullptr ? scene.getSelectedGameObject()->getId() : -1;
    snapshot.sceneScriptAssetPath = scene.getSceneScriptAssetPath();
    snapshot.dataAssetPath = scene.getDataAssetPath();
    snapshot.directionalLight = scene.getDirectionalLightSettings();
    snapshot.camera = scene.getCamera();
    snapshot.renderTargets = scene.getSavedRenderTargetSettings();

    const asset::SceneAssetRegistry& registry = scene.getSceneAssetRegistry();
    snapshot.meshAssets = registry.getAssets(asset::AssetType::Mesh);
    snapshot.shaderAssets = registry.getAssets(asset::AssetType::Shader);
    snapshot.materialAssets = registry.getAssets(asset::AssetType::Material);

    snapshot.gameObjects.reserve(scene.getGameObjectCount());
    for (size_t index = 0; index < scene.getGameObjectCount(); ++index)
    {
        const GameObject* gameObject = scene.getGameObject(index);
        if (gameObject == nullptr)
            continue;

        GameObjectSnapshot gameObjectSnapshot = captureGameObject(*gameObject);

        detail::appendUnique(snapshot.meshAssets, gameObjectSnapshot.meshAssetPath);
        detail::appendUnique(snapshot.materialAssets, gameObjectSnapshot.materialAssetPath);

        snapshot.gameObjects.push_back(gameObjectSnapshot);
    }

    return snapshot;
}

inline bool applySceneSnapshot(Scene& scene, const SceneSnapshot& snapshot)
{
    scene.setPhysicsSimulationEnabled(false);
    scene.setSelectedGameObject(nullptr);
    scene.setSceneScriptAssetPath(snapshot.sceneScriptAssetPath);
    scene.setDataAssetPath(snapshot.dataAssetPath);
    scene.setDirectionalLightSettings(snapshot.directionalLight);
    scene.setSavedRenderTargetSettings(snapshot.renderTargets);
    scene.clearGameObjects();
    scene.clearSceneAssetRegistry();

    for (const std::string& path : snapshot.shaderAssets)
        scene.resolveShaderAsset(path);
    for (const std::string& path : snapshot.meshAssets)
        scene.resolveMeshAsset(path);
    for (const std::string& path : snapshot.materialAssets)
        scene.resolveMaterialAsset(path);

    for (const GameObjectSnapshot& gameObjectSnapshot : snapshot.gameObjects)
    {
        GameObject* gameObject = scene.addGameObject(gameObjectSnapshot.name, gameObjectSnapshot.id);
        gameObject->transform.setPosition(gameObjectSnapshot.position);
        gameObject->transform.setRotation(gameObjectSnapshot.rotation);
        gameObject->transform.setScale(gameObjectSnapshot.scale);

        if (!gameObjectSnapshot.meshAssetPath.empty())
        {
            component::Mesh* mesh = scene.resolveMeshAsset(gameObjectSnapshot.meshAssetPath);
            if (mesh != nullptr)
                gameObject->addComponent(mesh);
        }

        for (const component_meta::ComponentSnapshot& componentSnapshot : gameObjectSnapshot.components)
        {
            std::unique_ptr<component::Component> component = component_meta::createComponentFromSnapshot(componentSnapshot, gameObject);
            if (component)
                gameObject->addComponent(component.release());
        }

        if (gameObject->getComponent<component::MeshRenderer>() == nullptr &&
            !gameObjectSnapshot.meshAssetPath.empty() &&
            !gameObjectSnapshot.materialAssetPath.empty())
        {
            component::Mesh* mesh = scene.resolveMeshAsset(gameObjectSnapshot.meshAssetPath);
            dataStruct::Material* material = scene.resolveMaterialAsset(gameObjectSnapshot.materialAssetPath);
            if (mesh != nullptr && material != nullptr)
                gameObject->addComponent(new component::MeshRenderer(mesh, material));
        }
    }

    for (const GameObjectSnapshot& gameObjectSnapshot : snapshot.gameObjects)
    {
        if (gameObjectSnapshot.parentId < 0)
            continue;

        GameObject* gameObject = scene.getGameObjectById(gameObjectSnapshot.id);
        GameObject* parent = scene.getGameObjectById(gameObjectSnapshot.parentId);
        if (gameObject != nullptr && parent != nullptr)
            gameObject->setParent(parent);
    }

    for (size_t index = 0; index < scene.getGameObjectCount(); ++index)
    {
        GameObject* gameObject = scene.getGameObject(index);
        if (gameObject == nullptr)
            continue;

        physics::RigidBody* rigidBody = gameObject->getComponent<physics::RigidBody>();
        if (rigidBody != nullptr)
            rigidBody->RefreshSerializedState();
    }

    scene.getCamera() = snapshot.camera;
    scene.setSelectedGameObjectById(snapshot.selectedGameObjectId);
    scene.setNextGameObjectId(snapshot.nextGameObjectId);
    scene.setPhysicsSimulationEnabled(snapshot.physicsSimulationEnabled);
    return true;
}

inline bool saveSceneToFile(const Scene& scene, const std::string& filePath)
{
    const SceneSnapshot snapshot = captureScene(scene);
    std::ofstream output(filePath.c_str(), std::ios::out | std::ios::trunc);
    if (!output.is_open())
        return false;

    return detail::saveSnapshotToStream(output, snapshot);
}

inline bool loadSceneFromFile(Scene& scene, const std::string& filePath)
{
    std::ifstream input(filePath.c_str(), std::ios::in);
    if (!input.is_open())
        return false;

    SceneSnapshot snapshot;
    if (!detail::loadSnapshotFromStream(input, snapshot))
        return false;

    return applySceneSnapshot(scene, snapshot);
}
}