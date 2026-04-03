set(_glutony_generated_scene_scripts_source ${CMAKE_CURRENT_LIST_DIR}/GeneratedSceneScripts.cpp)
if(DEFINED GLUTONY_GENERATED_SCENE_SCRIPTS_SOURCE)
    set(_glutony_generated_scene_scripts_source ${GLUTONY_GENERATED_SCENE_SCRIPTS_SOURCE})
endif()

set(GLUTONY_GAMEPLAY_ROOT_SOURCES
    ${CMAKE_CURRENT_LIST_DIR}/GameplayEntry.cpp
    ${_glutony_generated_scene_scripts_source}
)