#include <common/Scene.hpp>
#include <common/asset/AssetManager.hpp>
#include <common/asset/DataAssetIO.hpp>
#include <common/gameobject/component/MeshRenderer.hpp>
#include <common/physics/BoxCollider.hpp>
#include <common/physics/RigidBody.hpp>
#include <common/physics/SphereCollider.hpp>

#include <algorithm>
#include <glm/glm.hpp>
#include <iostream>
#include <string>

namespace
{
GameObject* findGameObjectByName(Scene& scene, const std::string& name)
{
    for (size_t index = 0; index < scene.getGameObjectCount(); ++index)
    {
        GameObject* gameObject = scene.getGameObject(index);
        if (gameObject != nullptr && gameObject->getName() == name)
            return gameObject;
    }

    return nullptr;
}

GameObject* ensureGameObject(Scene& scene, const std::string& name)
{
    GameObject* existing = findGameObjectByName(scene, name);
    return existing != nullptr ? existing : scene.addGameObject(name);
}

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

int readIntNode(asset::DataAssetDefinition& definition, const std::string& name, int fallback)
{
    asset::DataAssetNodeDefinition* node = findNodeByName(definition, name);
    return node != nullptr && node->kind == asset::DataAssetValueKind::Int ? node->intValue : fallback;
}

float readFloatNode(asset::DataAssetDefinition& definition, const std::string& name, float fallback)
{
    asset::DataAssetNodeDefinition* node = findNodeByName(definition, name);
    return node != nullptr && node->kind == asset::DataAssetValueKind::Float ? node->floatValue : fallback;
}

bool readBoolNode(asset::DataAssetDefinition& definition, const std::string& name, bool fallback)
{
    asset::DataAssetNodeDefinition* node = findNodeByName(definition, name);
    return node != nullptr && node->kind == asset::DataAssetValueKind::Bool ? node->boolValue : fallback;
}

glm::vec3 readVec3Node(asset::DataAssetDefinition& definition, const std::string& name, const glm::vec3& fallback)
{
    asset::DataAssetNodeDefinition* node = findNodeByName(definition, name);
    return node != nullptr && node->kind == asset::DataAssetValueKind::Vec3 ? node->vec3Value : fallback;
}

std::string readStringNode(asset::DataAssetDefinition& definition, const std::string& name, const std::string& fallback)
{
    asset::DataAssetNodeDefinition* node = findNodeByName(definition, name);
    return node != nullptr && node->kind == asset::DataAssetValueKind::String ? node->stringValue : fallback;
}

void writeStringNode(asset::DataAssetDefinition& definition, const std::string& name, const std::string& value)
{
    asset::DataAssetNodeDefinition* node = findNodeByName(definition, name);
    if (node != nullptr && node->kind == asset::DataAssetValueKind::String)
        node->stringValue = value;
}

void writeIntNode(asset::DataAssetDefinition& definition, const std::string& name, int value)
{
    asset::DataAssetNodeDefinition* node = findNodeByName(definition, name);
    if (node != nullptr && node->kind == asset::DataAssetValueKind::Int)
        node->intValue = value;
}

void writeVec3Node(asset::DataAssetDefinition& definition, const std::string& name, const glm::vec3& value)
{
    asset::DataAssetNodeDefinition* node = findNodeByName(definition, name);
    if (node != nullptr && node->kind == asset::DataAssetValueKind::Vec3)
        node->vec3Value = value;
}

void ensureMeshRenderer(Scene& scene, GameObject& gameObject, const std::string& meshPath, const std::string& materialPath)
{
    component::Mesh* mesh = scene.resolveMeshAsset(meshPath);
    dataStruct::Material* material = scene.resolveMaterialAsset(materialPath);
    if (mesh == nullptr || material == nullptr)
        return;

    component::MeshRenderer* renderer = gameObject.getComponent<component::MeshRenderer>();
    if (renderer == nullptr)
    {
        renderer = new component::MeshRenderer(mesh, material);
        gameObject.addComponent(renderer);
    }
    else
    {
        renderer->setMesh(mesh);
        renderer->setMaterial(material);
    }

    renderer->setMeshAssetPath(meshPath);
    renderer->setMaterialAssetPath(materialPath);
}

physics::RigidBody* ensureRigidBody(GameObject& gameObject)
{
    physics::RigidBody* rigidBody = gameObject.getComponent<physics::RigidBody>();
    if (rigidBody == nullptr)
    {
        rigidBody = new physics::RigidBody(&gameObject);
        gameObject.addComponent(rigidBody);
    }

    return rigidBody;
}

physics::BoxCollider* ensureBoxCollider(GameObject& gameObject, const glm::vec3& halfExtents)
{
    physics::BoxCollider* collider = gameObject.getComponent<physics::BoxCollider>();
    if (collider == nullptr)
    {
        collider = new physics::BoxCollider(glm::vec3(0.0f), halfExtents, glm::vec3(1.0f));
        gameObject.addComponent(collider);
    }
    return collider;
}

physics::SphereCollider* ensureSphereCollider(GameObject& gameObject, float radius)
{
    physics::SphereCollider* collider = gameObject.getComponent<physics::SphereCollider>();
    if (collider == nullptr)
    {
        collider = new physics::SphereCollider(glm::vec3(0.0f), radius);
        gameObject.addComponent(collider);
    }

    collider->m_localCenter = glm::vec3(0.0f);
    collider->m_radius = radius;
    return collider;
}
}

void sceneMain(Scene& scene)
{
    const std::string normalizedDataAssetPath = asset::AssetManager::normalizeRelativePath(scene.getDataAssetPath());
    asset::DataAssetDefinition* dataDefinition = normalizedDataAssetPath.empty()
        ? nullptr
        : asset::AssetManager::instance().loadDataAssetDefinition(normalizedDataAssetPath);

    const int requestedSpawnCount = dataDefinition != nullptr ? readIntNode(*dataDefinition, "spawn_count", 4) : 4;
    const int spawnCount = std::clamp(requestedSpawnCount, 2, 8);
    const float spacing = dataDefinition != nullptr ? readFloatNode(*dataDefinition, "spawn_spacing", 2.25f) : 2.25f;
    const bool enablePhysics = dataDefinition != nullptr ? readBoolNode(*dataDefinition, "enable_physics", true) : true;
    const glm::vec3 basePosition = dataDefinition != nullptr ? readVec3Node(*dataDefinition, "base_position", glm::vec3(0.0f, 1.25f, 0.0f)) : glm::vec3(0.0f, 1.25f, 0.0f);
    const std::string label = dataDefinition != nullptr ? readStringNode(*dataDefinition, "label", "preview stress test") : "preview stress test";

    GameObject* root = ensureGameObject(scene, "Stress Root");
    root->transform.setPosition(basePosition);
    root->transform.setRotation(glm::vec3(0.0f, 25.0f, 0.0f));
    root->transform.setScale(glm::vec3(1.0f));

    GameObject* pivot = ensureGameObject(scene, "Stress Pivot");
    pivot->setParent(root);
    pivot->transform.setPosition(glm::vec3(spacing, 0.0f, 0.0f));
    pivot->transform.setRotation(glm::vec3(0.0f, -35.0f, 0.0f));

    GameObject* cube = ensureGameObject(scene, "Stress Cube");
    cube->setParent(root);
    cube->transform.setPosition(glm::vec3(0.0f, 0.0f, spacing));
    cube->transform.setScale(glm::vec3(1.1f, 0.8f, 1.3f));
    ensureMeshRenderer(scene, *cube, "built-in/mesh/cube_n.obj", "built-in/materials/lit_default.mat");

    physics::RigidBody* cubeRigidBody = ensureRigidBody(*cube);
    cubeRigidBody->useGravity = enablePhysics;
    cubeRigidBody->mass = 3.0f;
    cubeRigidBody->linearDamping = 0.12f;
    cubeRigidBody->angularDamping = 0.08f;
    cubeRigidBody->RefreshSerializedState();
    ensureBoxCollider(*cube, glm::vec3(0.55f, 0.40f, 0.65f));

    GameObject* sphere = ensureGameObject(scene, "Stress Sphere");
    sphere->setParent(pivot);
    sphere->transform.setPosition(glm::vec3(0.0f, spacing * 0.6f, 0.0f));
    sphere->transform.setScale(glm::vec3(0.8f));
    ensureMeshRenderer(scene, *sphere, "built-in/mesh/unit_sphere_n.off", "built-in/materials/lit_default.mat");

    physics::RigidBody* sphereRigidBody = ensureRigidBody(*sphere);
    sphereRigidBody->useGravity = enablePhysics;
    sphereRigidBody->mass = 1.5f;
    sphereRigidBody->linearDamping = 0.04f;
    sphereRigidBody->angularDamping = 0.02f;
    sphereRigidBody->RefreshSerializedState();
    ensureSphereCollider(*sphere, 0.45f);

    for (int index = 0; index < spawnCount; ++index)
    {
        const std::string name = "Stress Satellite " + std::to_string(index);
        GameObject* satellite = ensureGameObject(scene, name);
        satellite->setParent((index % 2) == 0 ? root : pivot);
        satellite->transform.setPosition(glm::vec3(
            spacing * 0.65f * static_cast<float>(index - spawnCount / 2),
            0.35f * static_cast<float>(index),
            -spacing * 0.5f));
        satellite->transform.setRotation(glm::vec3(0.0f, 20.0f * static_cast<float>(index), 0.0f));
        satellite->transform.setScale(glm::vec3(0.45f + 0.08f * static_cast<float>(index)));
        ensureMeshRenderer(
            scene,
            *satellite,
            (index % 2) == 0 ? "built-in/mesh/cube_n.obj" : "built-in/mesh/unit_sphere_n.off",
            "built-in/materials/lit_default.mat");
    }

    if (dataDefinition != nullptr)
    {
        writeStringNode(*dataDefinition, "last_run", "sceneMain completed");
        writeIntNode(*dataDefinition, "spawned_objects", spawnCount + 4);
        writeStringNode(*dataDefinition, "root_name", root->getName() + " | " + label);
        writeVec3Node(*dataDefinition, "root_position", root->transform.getPosition());
    }

    std::cout << "[scene script] Stress sync scene initialized: label='" << label
              << "', spawn_count=" << spawnCount
              << ", data_asset='" << normalizedDataAssetPath << "'" << std::endl;
}