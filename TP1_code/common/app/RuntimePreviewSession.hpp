#pragma once

#include <filesystem>

#include "RuntimePaths.hpp"

namespace runtime_preview
{
inline std::filesystem::path runtimeRoot()
{
    return runtime_app::runtimeRoot();
}

inline std::filesystem::path sessionDirectory()
{
    return runtimeRoot() / ".glutony" / "runtime";
}

inline std::filesystem::path previewScenePath()
{
    return sessionDirectory() / "preview.scene";
}

inline std::filesystem::path frameMetadataPath()
{
    return sessionDirectory() / "frame.meta";
}

inline std::filesystem::path frameDataPath()
{
    return sessionDirectory() / "frame.rgba";
}

inline std::filesystem::path frameMetadataTempPath()
{
    return sessionDirectory() / "frame.meta.tmp";
}

inline std::filesystem::path frameDataTempPath()
{
    return sessionDirectory() / "frame.rgba.tmp";
}

inline std::filesystem::path resizeMetadataPath()
{
    return sessionDirectory() / "resize.meta";
}

inline std::filesystem::path resizeMetadataTempPath()
{
    return sessionDirectory() / "resize.meta.tmp";
}

inline std::filesystem::path inputMetadataPath()
{
    return sessionDirectory() / "input.meta";
}

inline std::filesystem::path inputMetadataTempPath()
{
    return sessionDirectory() / "input.meta.tmp";
}

inline std::filesystem::path clickMetadataPath()
{
    return sessionDirectory() / "click.meta";
}

inline std::filesystem::path clickMetadataTempPath()
{
    return sessionDirectory() / "click.meta.tmp";
}

inline std::filesystem::path selectionMetadataPath()
{
    return sessionDirectory() / "selection.meta";
}

inline std::filesystem::path selectionMetadataTempPath()
{
    return sessionDirectory() / "selection.meta.tmp";
}

inline std::filesystem::path sceneSyncMetadataPath()
{
    return sessionDirectory() / "scene_sync.meta";
}

inline std::filesystem::path sceneSyncMetadataTempPath()
{
    return sessionDirectory() / "scene_sync.meta.tmp";
}

inline std::filesystem::path sceneStatePath()
{
    return sessionDirectory() / "scene_state.scene";
}

inline std::filesystem::path sceneStateTempPath()
{
    return sessionDirectory() / "scene_state.scene.tmp";
}

inline std::filesystem::path sceneStateMetadataPath()
{
    return sessionDirectory() / "scene_state.meta";
}

inline std::filesystem::path sceneStateMetadataTempPath()
{
    return sessionDirectory() / "scene_state.meta.tmp";
}

inline std::filesystem::path objectPatchPath()
{
    return sessionDirectory() / "object_patch.meta";
}

inline std::filesystem::path objectPatchTempPath()
{
    return sessionDirectory() / "object_patch.meta.tmp";
}

inline std::filesystem::path objectStatePath()
{
    return sessionDirectory() / "object_state.meta";
}

inline std::filesystem::path objectStateTempPath()
{
    return sessionDirectory() / "object_state.meta.tmp";
}

inline std::filesystem::path pauseMetadataPath()
{
    return sessionDirectory() / "pause.meta";
}

inline std::filesystem::path pauseMetadataTempPath()
{
    return sessionDirectory() / "pause.meta.tmp";
}

inline std::filesystem::path stateMetadataPath()
{
    return sessionDirectory() / "state.meta";
}

inline std::filesystem::path stateMetadataTempPath()
{
    return sessionDirectory() / "state.meta.tmp";
}

inline std::filesystem::path materialMetadataPath()
{
    return sessionDirectory() / "material.meta";
}

inline std::filesystem::path materialMetadataTempPath()
{
    return sessionDirectory() / "material.meta.tmp";
}

inline std::filesystem::path materialStatePath()
{
    return sessionDirectory() / "material_state.meta";
}

inline std::filesystem::path materialStateTempPath()
{
    return sessionDirectory() / "material_state.meta.tmp";
}

inline std::filesystem::path dataAssetMetadataPath()
{
    return sessionDirectory() / "data_asset.meta";
}

inline std::filesystem::path dataAssetMetadataTempPath()
{
    return sessionDirectory() / "data_asset.meta.tmp";
}

inline std::filesystem::path dataAssetStatePath()
{
    return sessionDirectory() / "data_asset_state.meta";
}

inline std::filesystem::path dataAssetStateTempPath()
{
    return sessionDirectory() / "data_asset_state.meta.tmp";
}
}