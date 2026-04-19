// Include standard headers
#include <stdio.h>
#include <stdlib.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <sstream>
#include <string>
#include <system_error>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include <GL/glew.h>
#include <GLFW/glfw3.h>

#include <common/app/RuntimePaths.hpp>
#include <common/app/RuntimePreviewSession.hpp>
#include <common/app/RuntimeWindow.hpp>
#include <common/Scene.hpp>
#include <common/asset/DataAssetIO.hpp>
#include <common/asset/MaterialAssetIO.hpp>
#include <common/gameobject/component/Mesh.hpp>
#include <common/gameobject/component/ScriptComponent.hpp>
#include <common/physics/RigidBody.hpp>
#include <common/scene/SceneSerialization.hpp>
#include <common/utils/Raycast.hpp>

#include <gameplay/GameplayEntry.hpp>

namespace
{
GLFWwindow* g_window = nullptr;
Scene* g_scene = nullptr;
int g_windowFramebufferWidth = 1024;
int g_windowFramebufferHeight = 768;
bool g_sceneClickPending = false;
double g_sceneClickX = 0.0;
double g_sceneClickY = 0.0;
bool g_publishPreviewFrames = false;
uint64_t g_previewFrameSequence = 0;
std::vector<unsigned char> g_previewPixels;
uint64_t g_resizeRequestSequence = 0;
uint64_t g_inputSequence = 0;
uint64_t g_clickSequence = 0;
uint64_t g_selectionSequence = 0;
uint64_t g_pauseSequence = 0;
uint64_t g_objectPatchSequence = 0;
uint64_t g_objectStateSequence = 0;
uint64_t g_sceneStateSequence = 0;
uint64_t g_sceneSyncSequence = 0;
uint64_t g_stateSequence = 0;
uint64_t g_dataAssetSequence = 0;
uint64_t g_dataAssetStateSequence = 0;
uint64_t g_materialSequence = 0;
uint64_t g_materialStateSequence = 0;
bool g_remoteInputCapture = false;
bool g_previewPaused = false;
bool g_sceneSimulationEnabled = false;
bool g_sceneScriptStartPending = false;
int g_lastPublishedSelectedGameObjectId = -2;
int g_lastPublishedFps = -1;
int g_lastPublishedCaptureEnabled = -1;
std::unordered_map<std::string, std::string> g_lastPublishedDataAssetPayloads;
std::string g_lastPublishedObjectStatePayload;
std::string g_lastPublishedScenePayload;

void applySceneSimulationState()
{
    if (g_scene == nullptr)
        return;

    g_scene->setPhysicsSimulationEnabled(g_sceneSimulationEnabled && !g_previewPaused);
}

void enableRuntimeSceneSimulation()
{
    g_sceneSimulationEnabled = true;
    applySceneSimulationState();
}

std::vector<std::string> collectReferencedDataAssetPaths(const Scene& scene)
{
    std::vector<std::string> paths;
    std::unordered_set<std::string> seenPaths;

    const auto appendPath = [&](const std::string& rawPath) {
        const std::string normalizedPath = asset::AssetManager::normalizeRelativePath(rawPath);
        if (normalizedPath.empty() || !seenPaths.insert(normalizedPath).second)
            return;

        paths.push_back(normalizedPath);
    };

    appendPath(scene.getDataAssetPath());

    for (size_t gameObjectIndex = 0; gameObjectIndex < scene.getGameObjectCount(); ++gameObjectIndex)
    {
        const GameObject* gameObject = scene.getGameObject(gameObjectIndex);
        if (gameObject == nullptr)
            continue;

        for (size_t componentIndex = 0; componentIndex < gameObject->getComponentCount(); ++componentIndex)
        {
            const auto* scriptComponent = dynamic_cast<const component::ScriptComponent*>(gameObject->getComponentAt(componentIndex));
            if (scriptComponent == nullptr)
                continue;

            appendPath(scriptComponent->getDataAssetPath());
        }
    }

    std::sort(paths.begin(), paths.end());
    return paths;
}

void cachePublishedDataAssetPayload(const std::string& normalizedPath, const asset::DataAssetDefinition* definition)
{
    if (normalizedPath.empty() || definition == nullptr)
    {
        g_lastPublishedDataAssetPayloads.erase(normalizedPath);
        return;
    }

    std::ostringstream payloadStream;
    if (!asset::DataAssetIO::writeDefinition(payloadStream, *definition))
    {
        g_lastPublishedDataAssetPayloads.erase(normalizedPath);
        return;
    }

    g_lastPublishedDataAssetPayloads[normalizedPath] = payloadStream.str();
}

GameObject* pickGameObjectAt(Scene& scene, double clickX, double clickY, int viewportWidth, int viewportHeight)
{
    Raycast::Ray ray = Raycast::getRayFromClick(
        0,
        0,
        viewportWidth,
        viewportHeight,
        clickX,
        clickY,
        scene.getCamera());

    float nearestDistance = MAXFLOAT;
    GameObject* selectedGameObject = nullptr;

    for (size_t index = 0; index < scene.getGameObjectCount(); ++index)
    {
        GameObject* gameObject = scene.getGameObject(index);
        if (gameObject == nullptr)
            continue;

        float hitDistance = MAXFLOAT;
        component::Mesh* pickMesh = gameObject->getComponent<component::Mesh>();
        if (pickMesh == nullptr)
        {
            if (component::MeshRenderer* meshRenderer = gameObject->getComponent<component::MeshRenderer>())
                pickMesh = meshRenderer->getMesh();
        }

        if (pickMesh != nullptr)
        {
            Raycast::raycastTransformedAABB(
                gameObject->transform,
                pickMesh->getAABB(),
                ray,
                &hitDistance);
        }
        else if (gameObject->getComponent<physics::RigidBody>() != nullptr)
        {
            Raycast::raycastAABB(gameObject->getComponent<physics::RigidBody>()->getAABB(), ray, &hitDistance);
        }

        if (hitDistance < nearestDistance && hitDistance > 0.0f)
        {
            selectedGameObject = gameObject;
            nearestDistance = hitDistance;
        }
    }

    return selectedGameObject;
}

bool writeStateFile(uint64_t sequence, int selectedGameObjectId, bool captureEnabled, int fps)
{
    std::error_code errorCode;
    std::filesystem::create_directories(runtime_preview::sessionDirectory(), errorCode);
    if (errorCode)
        return false;

    std::ofstream output(runtime_preview::stateMetadataTempPath(), std::ios::trunc);
    if (!output)
        return false;

    output << sequence << ' ' << selectedGameObjectId << ' ' << (captureEnabled ? 1 : 0) << ' ' << fps << '\n';
    output.close();

    std::filesystem::rename(runtime_preview::stateMetadataTempPath(), runtime_preview::stateMetadataPath(), errorCode);
    if (errorCode)
    {
        std::filesystem::remove(runtime_preview::stateMetadataTempPath(), errorCode);
        return false;
    }

    return true;
}

bool writeObjectStateFile(uint64_t sequence, const scene_serialization::GameObjectSnapshot& snapshot)
{
    std::error_code errorCode;
    std::filesystem::create_directories(runtime_preview::sessionDirectory(), errorCode);
    if (errorCode)
        return false;

    std::ofstream output(runtime_preview::objectStateTempPath(), std::ios::trunc);
    if (!output)
        return false;

    output << sequence << '\n';
    if (!scene_serialization::detail::saveGameObjectSnapshotToStream(output, snapshot))
        return false;
    output.close();

    std::filesystem::rename(runtime_preview::objectStateTempPath(), runtime_preview::objectStatePath(), errorCode);
    if (errorCode)
    {
        std::filesystem::remove(runtime_preview::objectStateTempPath(), errorCode);
        return false;
    }

    return true;
}

bool writeMaterialStateFile(uint64_t sequence, const std::string& materialAssetPath, const asset::MaterialAssetDefinition& definition)
{
    std::error_code errorCode;
    std::filesystem::create_directories(runtime_preview::sessionDirectory(), errorCode);
    if (errorCode)
        return false;

    std::ofstream output(runtime_preview::materialStateTempPath(), std::ios::trunc);
    if (!output)
        return false;

    output << sequence << '\n' << materialAssetPath << '\n';
    if (!asset::MaterialAssetIO::writeDefinition(output, definition))
        return false;
    output.close();

    std::filesystem::rename(runtime_preview::materialStateTempPath(), runtime_preview::materialStatePath(), errorCode);
    if (errorCode)
    {
        std::filesystem::remove(runtime_preview::materialStateTempPath(), errorCode);
        return false;
    }

    return true;
}

bool writeDataAssetStateFile(uint64_t sequence, const std::string& dataAssetPath, const asset::DataAssetDefinition& definition)
{
    std::error_code errorCode;
    std::filesystem::create_directories(runtime_preview::sessionDirectory(), errorCode);
    if (errorCode)
        return false;

    std::ofstream output(runtime_preview::dataAssetStateTempPath(), std::ios::trunc);
    if (!output)
        return false;

    output << sequence << '\n' << dataAssetPath << '\n';
    if (!asset::DataAssetIO::writeDefinition(output, definition))
        return false;
    output.close();

    std::filesystem::rename(runtime_preview::dataAssetStateTempPath(), runtime_preview::dataAssetStatePath(), errorCode);
    if (errorCode)
    {
        std::filesystem::remove(runtime_preview::dataAssetStateTempPath(), errorCode);
        return false;
    }

    return true;
}

bool writeSceneStateFile(uint64_t sequence, const Scene& scene)
{
    std::error_code errorCode;
    std::filesystem::create_directories(runtime_preview::sessionDirectory(), errorCode);
    if (errorCode)
        return false;

    if (!scene_serialization::saveSceneToFile(scene, runtime_preview::sceneStateTempPath().string()))
        return false;

    std::filesystem::rename(runtime_preview::sceneStateTempPath(), runtime_preview::sceneStatePath(), errorCode);
    if (errorCode)
    {
        std::filesystem::remove(runtime_preview::sceneStateTempPath(), errorCode);
        return false;
    }

    std::ofstream metadataOutput(runtime_preview::sceneStateMetadataTempPath(), std::ios::trunc);
    if (!metadataOutput)
        return false;

    metadataOutput << sequence << '\n';
    metadataOutput.close();

    std::filesystem::rename(runtime_preview::sceneStateMetadataTempPath(), runtime_preview::sceneStateMetadataPath(), errorCode);
    if (errorCode)
    {
        std::filesystem::remove(runtime_preview::sceneStateMetadataTempPath(), errorCode);
        return false;
    }

    return true;
}

void publishRuntimeState(int currentFps)
{
    if (!g_publishPreviewFrames || g_scene == nullptr)
        return;

    const GameObject* selectedGameObject = g_scene->getSelectedGameObject();
    const int selectedGameObjectId = selectedGameObject != nullptr ? selectedGameObject->getId() : -1;
    const int captureEnabled = g_remoteInputCapture ? 1 : 0;
    if (selectedGameObjectId == g_lastPublishedSelectedGameObjectId
        && currentFps == g_lastPublishedFps
        && captureEnabled == g_lastPublishedCaptureEnabled)
        return;

    if (writeStateFile(g_stateSequence + 1, selectedGameObjectId, g_remoteInputCapture, currentFps))
    {
        ++g_stateSequence;
        g_lastPublishedSelectedGameObjectId = selectedGameObjectId;
        g_lastPublishedFps = currentFps;
        g_lastPublishedCaptureEnabled = captureEnabled;
    }
}

void publishSelectedObjectState()
{
    if (!g_publishPreviewFrames || g_scene == nullptr)
        return;

    const GameObject* selectedGameObject = g_scene->getSelectedGameObject();
    if (selectedGameObject == nullptr)
    {
        g_lastPublishedObjectStatePayload.clear();
        return;
    }

    const scene_serialization::GameObjectSnapshot snapshot = scene_serialization::captureGameObject(*selectedGameObject);
    std::ostringstream payloadStream;
    if (!scene_serialization::detail::saveGameObjectSnapshotToStream(payloadStream, snapshot))
        return;

    const std::string payload = payloadStream.str();
    if (payload == g_lastPublishedObjectStatePayload)
        return;

    if (writeObjectStateFile(g_objectStateSequence + 1, snapshot))
    {
        ++g_objectStateSequence;
        g_lastPublishedObjectStatePayload = payload;
    }
}

void publishDirtyMaterialState()
{
    if (!g_publishPreviewFrames || g_scene == nullptr)
        return;

    std::string materialAssetPath;
    asset::MaterialAssetDefinition definition;
    if (!asset::AssetManager::instance().popDirtyMaterialState(materialAssetPath, definition) || materialAssetPath.empty())
        return;

    if (writeMaterialStateFile(g_materialStateSequence + 1, materialAssetPath, definition))
        ++g_materialStateSequence;
}

void publishDirtyDataAssetState()
{
    if (!g_publishPreviewFrames || g_scene == nullptr)
        return;

    const std::vector<std::string> dataAssetPaths = collectReferencedDataAssetPaths(*g_scene);
    if (dataAssetPaths.empty())
    {
        g_lastPublishedDataAssetPayloads.clear();
        return;
    }

    std::unordered_set<std::string> referencedPathSet(dataAssetPaths.begin(), dataAssetPaths.end());
    for (auto it = g_lastPublishedDataAssetPayloads.begin(); it != g_lastPublishedDataAssetPayloads.end();)
    {
        if (referencedPathSet.count(it->first) == 0)
            it = g_lastPublishedDataAssetPayloads.erase(it);
        else
            ++it;
    }

    for (const std::string& dataAssetPath : dataAssetPaths)
    {
        asset::DataAssetDefinition* definition = asset::AssetManager::instance().loadDataAssetDefinition(dataAssetPath);
        if (definition == nullptr)
            continue;

        std::ostringstream payloadStream;
        if (!asset::DataAssetIO::writeDefinition(payloadStream, *definition))
            continue;

        const std::string payload = payloadStream.str();
        const auto cachedPayload = g_lastPublishedDataAssetPayloads.find(dataAssetPath);
        if (cachedPayload != g_lastPublishedDataAssetPayloads.end() && cachedPayload->second == payload)
            continue;

        if (writeDataAssetStateFile(g_dataAssetStateSequence + 1, dataAssetPath, *definition))
        {
            ++g_dataAssetStateSequence;
            g_lastPublishedDataAssetPayloads[dataAssetPath] = payload;
            return;
        }
    }
}

void publishSceneState()
{
    if (!g_publishPreviewFrames || g_scene == nullptr)
        return;

    const scene_serialization::SceneSnapshot snapshot = scene_serialization::captureScene(*g_scene);
    std::ostringstream payloadStream;
    if (!scene_serialization::detail::saveSnapshotToStream(payloadStream, snapshot))
        return;

    const std::string payload = payloadStream.str();
    if (payload == g_lastPublishedScenePayload)
        return;

    if (writeSceneStateFile(g_sceneStateSequence + 1, *g_scene))
    {
        ++g_sceneStateSequence;
        g_lastPublishedScenePayload = payload;
    }
}

struct RuntimeOptions
{
    std::string scenePath;
    int previewWidth = 1024;
    int previewHeight = 768;
    bool hiddenPreviewWindow = false;
};

bool parseRuntimeOptions(int argc, char** argv, RuntimeOptions& options)
{
    for (int index = 1; index < argc; ++index)
    {
        const std::string argument = argv[index];
        if (argument == "--scene" && index + 1 < argc)
        {
            options.scenePath = argv[++index];
        }
        else if (argument == "--preview-width" && index + 1 < argc)
        {
            options.previewWidth = std::max(std::atoi(argv[++index]), 1);
        }
        else if (argument == "--preview-height" && index + 1 < argc)
        {
            options.previewHeight = std::max(std::atoi(argv[++index]), 1);
        }
        else if (argument == "--hidden-preview")
        {
            options.hiddenPreviewWindow = true;
            g_publishPreviewFrames = true;
        }
        else if (!argument.empty() && argument[0] != '-' && options.scenePath.empty())
        {
            options.scenePath = argument;
        }
        else
        {
            std::cerr << "\033[33m[player] Ignoring unknown argument: " << argument << "\033[0m" << std::endl;
        }
    }

    return true;
}

void publishPreviewFrame(int width, int height)
{
    if (!g_publishPreviewFrames || width <= 0 || height <= 0)
        return;

    const size_t pixelCount = static_cast<size_t>(width) * static_cast<size_t>(height) * 4u;
    g_previewPixels.resize(pixelCount);

    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadBuffer(GL_BACK);
    glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, g_previewPixels.data());

    std::error_code errorCode;
    std::filesystem::create_directories(runtime_preview::sessionDirectory(), errorCode);
    if (errorCode)
        return;

    {
        std::ofstream dataOutput(runtime_preview::frameDataTempPath(), std::ios::binary | std::ios::trunc);
        if (!dataOutput)
            return;
        dataOutput.write(reinterpret_cast<const char*>(g_previewPixels.data()), static_cast<std::streamsize>(g_previewPixels.size()));
    }

    {
        std::ofstream metaOutput(runtime_preview::frameMetadataTempPath(), std::ios::trunc);
        if (!metaOutput)
            return;
        metaOutput << (g_previewFrameSequence + 1) << ' ' << width << ' ' << height << '\n';
    }

    std::filesystem::rename(runtime_preview::frameDataTempPath(), runtime_preview::frameDataPath(), errorCode);
    if (errorCode)
    {
        std::filesystem::remove(runtime_preview::frameDataTempPath(), errorCode);
        return;
    }

    std::filesystem::rename(runtime_preview::frameMetadataTempPath(), runtime_preview::frameMetadataPath(), errorCode);
    if (errorCode)
    {
        std::filesystem::remove(runtime_preview::frameMetadataTempPath(), errorCode);
        return;
    }

    ++g_previewFrameSequence;
}

void pollPreviewResizeRequests()
{
    if (!g_publishPreviewFrames || g_window == nullptr)
        return;

    std::ifstream resizeInput(runtime_preview::resizeMetadataPath());
    if (!resizeInput)
        return;

    uint64_t nextSequence = 0;
    int nextWidth = 0;
    int nextHeight = 0;
    if (!(resizeInput >> nextSequence >> nextWidth >> nextHeight))
        return;

    if (nextSequence <= g_resizeRequestSequence || nextWidth <= 0 || nextHeight <= 0)
        return;

    glfwSetWindowSize(g_window, nextWidth, nextHeight);
    glfwGetFramebufferSize(g_window, &g_windowFramebufferWidth, &g_windowFramebufferHeight);
    if (g_scene != nullptr && g_windowFramebufferHeight > 0)
        g_scene->updateCamSettings((float)g_windowFramebufferWidth / (float)g_windowFramebufferHeight);

    g_resizeRequestSequence = nextSequence;
    std::cout << "\033[36m[player] Preview resized to " << g_windowFramebufferWidth << "x" << g_windowFramebufferHeight << "\033[0m" << std::endl;
}

void pollPreviewInputState()
{
    if (!g_publishPreviewFrames || g_scene == nullptr)
        return;

    std::ifstream inputStream(runtime_preview::inputMetadataPath());
    if (!inputStream)
        return;

    uint64_t nextSequence = 0;
    int captureEnabled = 0;
    double mouseDeltaX = 0.0;
    double mouseDeltaY = 0.0;
    size_t keyCount = 0;
    if (!(inputStream >> nextSequence >> captureEnabled >> mouseDeltaX >> mouseDeltaY >> keyCount))
        return;

    if (nextSequence <= g_inputSequence)
        return;

    std::map<int, bool> pressedKeys;
    for (size_t keyIndex = 0; keyIndex < keyCount; ++keyIndex)
    {
        int keyCode = 0;
        int pressed = 0;
        if (!(inputStream >> keyCode >> pressed))
            break;
        pressedKeys[keyCode] = (pressed != 0);
    }

    g_scene->getInputProcessor().setInjectedInputState(pressedKeys, mouseDeltaX, mouseDeltaY);

    const bool captureRequested = (captureEnabled != 0);
    if (captureRequested != g_remoteInputCapture)
    {
        g_remoteInputCapture = captureRequested;
        g_scene->setFpsControlEnabled(g_remoteInputCapture, g_window, true, g_windowFramebufferWidth * 0.5, g_windowFramebufferHeight * 0.5);
    }

    g_inputSequence = nextSequence;
}

void pollPreviewSelectionRequests()
{
    if (!g_publishPreviewFrames || g_scene == nullptr)
        return;

    std::ifstream selectionStream(runtime_preview::selectionMetadataPath());
    if (!selectionStream)
        return;

    uint64_t nextSequence = 0;
    int selectedGameObjectId = -1;
    if (!(selectionStream >> nextSequence >> selectedGameObjectId) || nextSequence <= g_selectionSequence)
        return;

    g_scene->setSelectedGameObjectById(selectedGameObjectId);
    g_selectionSequence = nextSequence;
}

void pollPreviewClickRequests()
{
    if (!g_publishPreviewFrames || g_scene == nullptr)
        return;

    std::ifstream clickStream(runtime_preview::clickMetadataPath());
    if (!clickStream)
        return;

    uint64_t nextSequence = 0;
    double clickX = 0.0;
    double clickY = 0.0;
    if (!(clickStream >> nextSequence >> clickX >> clickY) || nextSequence <= g_clickSequence)
        return;

    if (!g_remoteInputCapture)
        g_scene->setSelectedGameObject(pickGameObjectAt(*g_scene, clickX, clickY, std::max(g_windowFramebufferWidth, 1), std::max(g_windowFramebufferHeight, 1)));

    g_clickSequence = nextSequence;
}

void pollPreviewPauseRequests()
{
    if (!g_publishPreviewFrames || g_scene == nullptr)
        return;

    std::ifstream pauseStream(runtime_preview::pauseMetadataPath());
    if (!pauseStream)
        return;

    uint64_t nextSequence = 0;
    int paused = 0;
    if (!(pauseStream >> nextSequence >> paused) || nextSequence <= g_pauseSequence)
        return;

    g_previewPaused = (paused != 0);
    applySceneSimulationState();
    g_pauseSequence = nextSequence;
}

void pollPreviewObjectPatchRequests()
{
    if (!g_publishPreviewFrames || g_scene == nullptr)
        return;

    std::ifstream patchStream(runtime_preview::objectPatchPath());
    if (!patchStream)
        return;

    uint64_t nextSequence = 0;
    if (!(patchStream >> nextSequence) || nextSequence <= g_objectPatchSequence)
        return;

    scene_serialization::GameObjectSnapshot snapshot;
    if (!scene_serialization::detail::loadGameObjectSnapshotFromStream(patchStream, snapshot))
        return;

    if (!scene_serialization::applyGameObjectSnapshot(*g_scene, snapshot))
        return;

    g_objectPatchSequence = nextSequence;
    g_lastPublishedObjectStatePayload.clear();
}

void pollPreviewSceneSyncRequests()
{
    if (!g_publishPreviewFrames || g_scene == nullptr)
        return;

    std::ifstream syncStream(runtime_preview::sceneSyncMetadataPath());
    if (!syncStream)
        return;

    uint64_t nextSequence = 0;
    if (!(syncStream >> nextSequence) || nextSequence <= g_sceneSyncSequence)
        return;

    if (scene_serialization::loadSceneFromFile(*g_scene, runtime_preview::previewScenePath().string()))
    {
        if (g_windowFramebufferHeight > 0)
            g_scene->updateCamSettings((float)g_windowFramebufferWidth / (float)g_windowFramebufferHeight);

        enableRuntimeSceneSimulation();
        g_scene->setFpsControlEnabled(g_remoteInputCapture, g_window, true, g_windowFramebufferWidth * 0.5, g_windowFramebufferHeight * 0.5);
        g_lastPublishedSelectedGameObjectId = -2;
        g_lastPublishedObjectStatePayload.clear();
        g_lastPublishedScenePayload.clear();
        g_sceneScriptStartPending = true;
        g_sceneSyncSequence = nextSequence;
        std::cout << "\033[36m[player] Runtime scene synchronized from editor.\033[0m" << std::endl;
    }
    else
    {
        std::cerr << "\033[31m[player] Failed to synchronize preview scene.\033[0m" << std::endl;
    }
}

void pollPreviewMaterialRequests()
{
    if (!g_publishPreviewFrames || g_scene == nullptr)
        return;

    std::ifstream materialStream(runtime_preview::materialMetadataPath());
    if (!materialStream)
        return;

    uint64_t nextSequence = 0;
    if (!(materialStream >> nextSequence) || nextSequence <= g_materialSequence)
        return;

    materialStream.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::string materialAssetPath;
    if (!std::getline(materialStream, materialAssetPath) || materialAssetPath.empty())
        return;

    if (!g_scene->refreshMaterialAsset(materialAssetPath))
    {
        std::cerr << "\033[31m[player] Failed to refresh material asset: " << materialAssetPath << "\033[0m" << std::endl;
        return;
    }

    g_materialSequence = nextSequence;
}

void pollPreviewDataAssetRequests()
{
    if (!g_publishPreviewFrames || g_scene == nullptr)
        return;

    std::ifstream dataAssetStream(runtime_preview::dataAssetMetadataPath());
    if (!dataAssetStream)
        return;

    uint64_t nextSequence = 0;
    if (!(dataAssetStream >> nextSequence) || nextSequence <= g_dataAssetSequence)
        return;

    dataAssetStream.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
    std::string dataAssetPath;
    if (!std::getline(dataAssetStream, dataAssetPath) || dataAssetPath.empty())
        return;

    asset::DataAssetDefinition* definition = nullptr;
    if (!asset::AssetManager::instance().reloadDataAssetDefinition(dataAssetPath, definition))
    {
        std::cerr << "\033[31m[player] Failed to refresh data asset: " << dataAssetPath << "\033[0m" << std::endl;
        return;
    }

    cachePublishedDataAssetPayload(asset::AssetManager::normalizeRelativePath(dataAssetPath), definition);

    g_dataAssetSequence = nextSequence;
}

void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    (void)window;
    g_windowFramebufferWidth = width;
    g_windowFramebufferHeight = height;

    if (g_scene != nullptr && height > 0)
        g_scene->updateCamSettings((float)width / (float)height);
}

void setup_glfw_callbacks(GLFWwindow* glfwWindow)
{
    glfwSetMouseButtonCallback(glfwWindow, [](GLFWwindow* callbackWindow, int button, int action, int mods)
    {
        (void)mods;
        if (button != GLFW_MOUSE_BUTTON_LEFT || action != GLFW_PRESS)
            return;

        double clickWindowX = 0.0;
        double clickWindowY = 0.0;
        glfwGetCursorPos(callbackWindow, &clickWindowX, &clickWindowY);

        int windowWidth = 1;
        int windowHeight = 1;
        int framebufferWidth = 1;
        int framebufferHeight = 1;
        glfwGetWindowSize(callbackWindow, &windowWidth, &windowHeight);
        glfwGetFramebufferSize(callbackWindow, &framebufferWidth, &framebufferHeight);

        if (windowWidth <= 0 || windowHeight <= 0)
            return;

        g_sceneClickPending = true;
        g_sceneClickX = clickWindowX * static_cast<double>(framebufferWidth) / static_cast<double>(windowWidth);
        g_sceneClickY = clickWindowY * static_cast<double>(framebufferHeight) / static_cast<double>(windowHeight);
    });

    glfwSetFramebufferSizeCallback(glfwWindow, framebuffer_size_callback);
}
}

int main(int argc, char** argv)
{
    runtime_app::adoptProcessWorkingDirectoryToRuntimeRoot();

    RuntimeOptions runtimeOptions;
    parseRuntimeOptions(argc, argv, runtimeOptions);

    runtime_app::RuntimeWindow runtimeWindow;
    if (!runtime_app::initializeWindow(
            runtimeWindow,
            "glutglut_player",
            "Failed to open GLFW window",
            !runtimeOptions.hiddenPreviewWindow))
    {
        std::cerr << "\033[31m[player] Failed to initialize runtime window.\033[0m" << std::endl;
        return -1;
    }

    g_window = runtimeWindow.handle;
    g_windowFramebufferWidth = runtimeWindow.framebufferWidth;
    g_windowFramebufferHeight = runtimeWindow.framebufferHeight;

    if (runtimeOptions.hiddenPreviewWindow)
    {
        glfwSetWindowSize(g_window, runtimeOptions.previewWidth, runtimeOptions.previewHeight);
        glfwGetFramebufferSize(g_window, &g_windowFramebufferWidth, &g_windowFramebufferHeight);
    }

    g_scene = new Scene();
    setup_glfw_callbacks(g_window);
    if (g_windowFramebufferHeight > 0)
        g_scene->updateCamSettings((float)g_windowFramebufferWidth / (float)g_windowFramebufferHeight);

    if (!runtimeOptions.scenePath.empty())
    {
        const std::string scenePath = runtimeOptions.scenePath;
        if (!scene_serialization::loadSceneFromFile(*g_scene, scenePath))
        {
            std::cerr << "\033[31m[player] Failed to load scene: " << scenePath << "\033[0m" << std::endl;
            delete g_scene;
            runtime_app::shutdownWindow(runtimeWindow);
            return -1;
        }

        std::cout << "\033[32m[player] Loaded scene: " << scenePath << "\033[0m" << std::endl;
    }
    else
    {
        std::cout << "\033[33m[player] No scene argument provided, starting with the default scene.\033[0m" << std::endl;
    }

    if (g_windowFramebufferHeight > 0)
        g_scene->updateCamSettings((float)g_windowFramebufferWidth / (float)g_windowFramebufferHeight);
    enableRuntimeSceneSimulation();
    g_scene->setFpsControlEnabled(!runtimeOptions.hiddenPreviewWindow, g_window, false);
    gameplay::bootstrap();
    g_sceneScriptStartPending = true;

    g_lastPublishedDataAssetPayloads.clear();
    for (const std::string& initialDataAssetPath : collectReferencedDataAssetPaths(*g_scene))
    {
        asset::DataAssetDefinition* initialDefinition = asset::AssetManager::instance().loadDataAssetDefinition(initialDataAssetPath);
        cachePublishedDataAssetPayload(initialDataAssetPath, initialDefinition);
    }

    float lastFrame = 0.0f;
    while (glfwWindowShouldClose(g_window) == 0)
    {
        const float currentFrame = glfwGetTime();
        const float deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        const float clampedDeltaTime = std::max(deltaTime, 1e-6f);
        const int currentFps = std::clamp(static_cast<int>(std::lround(1.0f / clampedDeltaTime)), 0, 60);

        glfwPollEvents();
        pollPreviewResizeRequests();
        pollPreviewSceneSyncRequests();
        pollPreviewDataAssetRequests();
        pollPreviewMaterialRequests();
        pollPreviewPauseRequests();
        pollPreviewObjectPatchRequests();
        pollPreviewSelectionRequests();
        pollPreviewInputState();
        pollPreviewClickRequests();

        if (g_sceneClickPending && !g_scene->isFpsControlEnabled())
        {
            g_scene->setSelectedGameObject(nullptr);
        }

        if (g_sceneScriptStartPending)
        {
            gameplay::runSceneScript(*g_scene);
            gameplay::runPendingComponentScriptStarts(*g_scene);
            publishSceneState();
            g_sceneScriptStartPending = false;
        }

        gameplay::runPendingComponentScriptStarts(*g_scene);
        if (!g_previewPaused)
            gameplay::runComponentScriptUpdates(*g_scene, deltaTime);

        g_scene->update(
            g_previewPaused ? 0.0f : deltaTime,
            g_window,
            true,
            g_publishPreviewFrames ? g_remoteInputCapture : false,
            g_windowFramebufferWidth * 0.5,
            g_windowFramebufferHeight * 0.5);
        g_sceneClickPending = false;

        publishSceneState();

        glViewport(0, 0, g_windowFramebufferWidth, g_windowFramebufferHeight);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
        if (g_publishPreviewFrames)
            g_scene->renderSceneWithSelectionHighlight();
        else
            g_scene->renderScene();
        publishRuntimeState(currentFps);
        publishSelectedObjectState();
        publishDirtyDataAssetState();
        publishPreviewFrame(g_windowFramebufferWidth, g_windowFramebufferHeight);
        glfwSwapBuffers(g_window);
    }

    delete g_scene;
    runtime_app::shutdownWindow(runtimeWindow);
    return 0;
}