#pragma once

#include <common/Scene.hpp>

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <string>
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
    int version = 1;
    int nextGameObjectId = 1;
    bool physicsSimulationEnabled = false;
    int selectedGameObjectId = -1;
    Camera camera;
    std::vector<std::string> meshAssets;
    std::vector<std::string> shaderAssets;
    std::vector<std::string> materialAssets;
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
    if (!(input >> token) || token != "CAMERA_POSITION" || !readVec3(input, snapshot.camera.m_position))
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

    size_t gameObjectCount = 0;
    if (!(input >> token >> gameObjectCount) || token != "GAME_OBJECTS")
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
}

inline SceneSnapshot captureScene(const Scene& scene)
{
    SceneSnapshot snapshot;
    snapshot.nextGameObjectId = scene.getNextGameObjectId();
    snapshot.physicsSimulationEnabled = scene.isPhysicsSimulationEnabled();
    snapshot.selectedGameObjectId = scene.getSelectedGameObject() != nullptr ? scene.getSelectedGameObject()->getId() : -1;
    snapshot.camera = scene.getCamera();

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

        GameObjectSnapshot gameObjectSnapshot;
        gameObjectSnapshot.id = gameObject->getId();
        gameObjectSnapshot.name = gameObject->getName();
        if (const Transform* parent = gameObject->transform.getParent())
        {
            const GameObject* parentObject = parent->getGameObject();
            gameObjectSnapshot.parentId = parentObject != nullptr ? parentObject->getId() : -1;
        }
        gameObjectSnapshot.position = gameObject->transform.getPosition();
        gameObjectSnapshot.rotation = gameObject->transform.getRotation();
        gameObjectSnapshot.scale = gameObject->transform.getScale();

        if (const component::MeshRenderer* meshRenderer = detail::findComponent<component::MeshRenderer>(*gameObject))
        {
            gameObjectSnapshot.meshAssetPath = meshRenderer->getMeshAssetPath();
            gameObjectSnapshot.materialAssetPath = meshRenderer->getMaterialAssetPath();
        }
        else if (const component::Mesh* mesh = detail::findComponent<component::Mesh>(*gameObject))
        {
            gameObjectSnapshot.meshAssetPath = mesh->getAssetPath();
        }

        detail::appendUnique(snapshot.meshAssets, gameObjectSnapshot.meshAssetPath);
        detail::appendUnique(snapshot.materialAssets, gameObjectSnapshot.materialAssetPath);

        for (size_t componentIndex = 0; componentIndex < gameObject->getComponentCount(); ++componentIndex)
        {
            const component::Component* component = gameObject->getComponentAt(componentIndex);
            if (component == nullptr)
                continue;
            if (dynamic_cast<const component::Mesh*>(component) != nullptr)
                continue;

            const std::optional<component_meta::ComponentSnapshot> componentSnapshot = component_meta::trySerializeComponent(*component);
            if (componentSnapshot.has_value())
                gameObjectSnapshot.components.push_back(*componentSnapshot);
        }

        snapshot.gameObjects.push_back(gameObjectSnapshot);
    }

    return snapshot;
}

inline bool applySceneSnapshot(Scene& scene, const SceneSnapshot& snapshot)
{
    scene.setPhysicsSimulationEnabled(false);
    scene.setSelectedGameObject(nullptr);
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