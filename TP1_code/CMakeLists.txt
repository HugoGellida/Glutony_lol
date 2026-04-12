# CMake entry point
cmake_minimum_required(VERSION 3.10)
project(Tutorials LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(FetchContent)

if(POLICY CMP0072)
    cmake_policy(SET CMP0072 NEW)
endif()

set(CMAKE_WARN_DEPRECATED OFF CACHE BOOL "Suppress deprecation warnings from bundled third-party CMake projects" FORCE)
set(CMAKE_SUPPRESS_DEVELOPER_WARNINGS 1 CACHE BOOL "Suppress developer warnings from bundled third-party CMake projects" FORCE)

set(OpenGL_GL_PREFERENCE LEGACY)

find_package(OpenGL REQUIRED)

if(CMAKE_BINARY_DIR STREQUAL CMAKE_SOURCE_DIR)
    message(FATAL_ERROR "Please select another Build Directory !")
endif()

if(CMAKE_SOURCE_DIR MATCHES " ")
    message("Your Source Directory contains spaces. If you experience problems when compiling, this can be the cause.")
endif()

if(CMAKE_BINARY_DIR MATCHES " ")
    message("Your Build Directory contains spaces. If you experience problems when compiling, this can be the cause.")
endif()

# On veut un exécutable standalone -> on préfère des libs statiques
option(BUILD_SHARED_LIBS "Build libraries as shared" OFF)

# GLFW options before add_subdirectory to avoid building its test suite.
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

# Options RmlUi avant son add_subdirectory
set(RMLUI_SAMPLES OFF CACHE BOOL "" FORCE)
set(RMLUI_LUA_BINDINGS OFF CACHE BOOL "" FORCE)
set(RMLUI_LOTTIE_PLUGIN OFF CACHE BOOL "" FORCE)
set(RMLUI_SVG_PLUGIN OFF CACHE BOOL "" FORCE)
set(RMLUI_PRECOMPILED_HEADERS OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)

option(GLUTONY_FETCH_FREETYPE "Download and build Freetype when it is not available on the host" ON)

if(GLUTONY_FETCH_FREETYPE)
    find_package(Freetype QUIET)

    if(FREETYPE_FOUND AND NOT TARGET Freetype::Freetype)
        add_library(Freetype::Freetype INTERFACE IMPORTED)
        set_target_properties(Freetype::Freetype PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${FREETYPE_INCLUDE_DIRS}"
            INTERFACE_LINK_LIBRARIES "${FREETYPE_LIBRARIES}"
        )
    endif()

    if(NOT TARGET Freetype::Freetype)
        message(STATUS "Freetype not found on the host, fetching a static copy for RmlUi.")

        set(FT_DISABLE_ZLIB ON CACHE BOOL "" FORCE)
        set(FT_DISABLE_BZIP2 ON CACHE BOOL "" FORCE)
        set(FT_DISABLE_PNG ON CACHE BOOL "" FORCE)
        set(FT_DISABLE_HARFBUZZ ON CACHE BOOL "" FORCE)
        set(FT_DISABLE_BROTLI ON CACHE BOOL "" FORCE)

        FetchContent_Declare(
            freetype
            URL https://download.savannah.gnu.org/releases/freetype/freetype-2.13.3.tar.xz
            DOWNLOAD_EXTRACT_TIMESTAMP FALSE
        )

        if(CMAKE_VERSION VERSION_GREATER_EQUAL "3.14")
            FetchContent_MakeAvailable(freetype)
        else()
            FetchContent_GetProperties(freetype)
            if(NOT freetype_POPULATED)
                FetchContent_Populate(freetype)
                add_subdirectory(${freetype_SOURCE_DIR} ${freetype_BINARY_DIR} EXCLUDE_FROM_ALL)
            endif()
        endif()

        if(TARGET freetype AND NOT TARGET Freetype::Freetype)
            add_library(Freetype::Freetype ALIAS freetype)
        endif()
    endif()
endif()

if(NOT TARGET Freetype::Freetype)
    message(FATAL_ERROR
        "Freetype is required by RmlUi. Install it on the host or leave GLUTONY_FETCH_FREETYPE=ON "
        "to build a bundled static copy automatically."
    )
endif()

# Compile external dependencies
add_subdirectory(external)

# RmlUi
add_subdirectory(external/RmlUi)
add_subdirectory(external/RmlUi/Backends)

list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/external/rpavlik-cmake-modules-fe2273")
include(MSVCMultipleProcessCompile) # /MP

if(INCLUDE_DISTRIB)
    add_subdirectory(distrib)
endif()

include_directories(
    external/glfw-3.1.2/include/
    external/glm-0.9.7.1/
    external/glew-1.13.0/include/
    external/stb_image/
    external/RmlUi/Backends/
    .
)

set(ALL_LIBS
    ${OPENGL_LIBRARY}
    glfw
    GLEW_1130
    RmlUi::RmlUi
)

add_definitions(
    -DTW_STATIC
    -DTW_NO_LIB_PRAGMA
    -DTW_NO_DIRECT3D
    -DGLEW_STATIC
    -D_CRT_SECURE_NO_WARNINGS
)

option(ENABLE_LSAN "Enable LeakSanitizer (LSan) for TP1" OFF)
option(GLUTONY_BUILD_GAMEPLAY "Build gameplay root sources" ON)

set(GLUTONY_GAMEPLAY_MANIFEST "${CMAKE_CURRENT_SOURCE_DIR}/gameplay/sources.cmake" CACHE FILEPATH "Gameplay manifest listing root cpp sources")
set(GLUTONY_GENERATED_SCENE_SCRIPTS_SOURCE "${CMAKE_CURRENT_BINARY_DIR}/GeneratedSceneScripts.cpp")

if(NOT EXISTS "${GLUTONY_GENERATED_SCENE_SCRIPTS_SOURCE}")
    file(WRITE "${GLUTONY_GENERATED_SCENE_SCRIPTS_SOURCE}" "#include \"GameplayEntry.hpp\"\n\nnamespace gameplay\n{\nvoid registerGeneratedSceneScripts()\n{\n}\n\nvoid registerGeneratedRenderUniformFactories()\n{\n}\n}\n")
endif()

set(GLUTONY_APP_COMMON_SOURCES
    common/app/RuntimeWindow.cpp
)

set(GLUTONY_RUNTIME_SOURCES
    common/gameobject/Transform.cpp
    common/gameobject/component/MeshRenderer.cpp
    common/gameobject/component/PointLight.cpp
    common/gameobject/component/ScriptComponent.cpp
    common/physics/PhysicEngine.cpp
    common/physics/RigidBody.cpp
    common/stb_image_impl.cpp
)

set(GLUTONY_EDITOR_SUPPORT_SOURCES
    common/UI/SceneEditorAssetBrowserModel.cpp
    common/UI/SceneEditorHierarchyModel.cpp
    common/UI/SceneEditorLayoutManager.cpp
    common/UI/UiBuilderDocumentSerializer.cpp
    common/UI/UiBuilderHierarchyModel.cpp
    common/UI/UiBuilderHierarchyPresenter.cpp
    common/UI/UiBuilderInspectorPresenter.cpp
    common/UI/UiBuilderLayoutManager.cpp
    common/UI/UiBuilderWidgetCatalogPresenter.cpp
    common/platform/NativeFileDialog.cpp
    common/ui/EditorUi.cpp
    common/ui/EditorUiCommon.cpp
    common/ui/EditorUiDispatcher.cpp
    common/ui/EditorUiDocuments.cpp
    common/ui/EditorUiPreviewDocument.cpp
    common/ui/SceneEditorController.cpp
    common/ui/SceneEditorControllerPresentation.cpp
    common/ui/SceneEditorDataAssetEditor.cpp
    common/ui/SceneEditorMaterialAssetEditor.cpp
    common/ui/SceneViewportOverlay.cpp
    common/ui/UIBinder.cpp
    common/ui/UIRenderer.cpp
    common/ui/UiBuilderController.cpp
    common/ui/widget/Widget.cpp
    common/ui/widget/WidgetRegistry.cpp
    common/ui/widget/ConstrainedBoxWidget.cpp
    common/ui/widget/StackLayoutWidget.cpp
    common/ui/widget/TextWidget.cpp
    common/ui/widget/ImageWidget.cpp
    common/ui/widget/ButtonWidget.cpp
    common/ui/widget/SpacerWidget.cpp
    common/ui/widget/ScrollRegionWidget.cpp
    external/RmlUi/Backends/RmlUi_Platform_GLFW.cpp
    external/RmlUi/Backends/RmlUi_Renderer_GL3.cpp
)

set(GLUTONY_GAMEPLAY_ROOT_SOURCES)
if(GLUTONY_BUILD_GAMEPLAY AND EXISTS "${GLUTONY_GAMEPLAY_MANIFEST}")
    include("${GLUTONY_GAMEPLAY_MANIFEST}")
endif()

add_library(app_common STATIC
    ${GLUTONY_APP_COMMON_SOURCES}
)

target_include_directories(app_common
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/external/glfw-3.1.2/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/glew-1.13.0/include
)

target_link_libraries(app_common
    PUBLIC
        ${OPENGL_LIBRARY}
        glfw
        GLEW_1130
)

add_library(engine_core STATIC
    ${GLUTONY_RUNTIME_SOURCES}
)

target_include_directories(engine_core
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/external/glfw-3.1.2/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/glm-0.9.7.1
        ${CMAKE_CURRENT_SOURCE_DIR}/external/glew-1.13.0/include
        ${CMAKE_CURRENT_SOURCE_DIR}/external/stb_image
)

target_link_libraries(engine_core
    PUBLIC
        ${OPENGL_LIBRARY}
        glfw
        GLEW_1130
)

add_library(editor_support STATIC
    ${GLUTONY_EDITOR_SUPPORT_SOURCES}
)

target_include_directories(editor_support
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/external/RmlUi/Backends
)

target_link_libraries(editor_support
    PUBLIC
        engine_core
        RmlUi::RmlUi
)

add_library(gameplay_sources STATIC EXCLUDE_FROM_ALL
    ${GLUTONY_GAMEPLAY_ROOT_SOURCES}
)

target_include_directories(gameplay_sources
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/gameplay
)

target_link_libraries(gameplay_sources
    PUBLIC
        engine_core
)

target_compile_definitions(editor_support PRIVATE
    RMLUI_GL3_CUSTOM_LOADER=<GL/glew.h>
)

if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(editor_support PRIVATE RMLUI_STATIC_LIB)
endif()

function(glutony_configure_runtime_assets target_name runtime_working_directory)
    set_target_properties(${target_name} PROPERTIES
        VS_DEBUGGER_WORKING_DIRECTORY "${runtime_working_directory}"
        XCODE_ATTRIBUTE_CONFIGURATION_BUILD_DIR "${runtime_working_directory}"
    )

    if(target_name STREQUAL "TP1" AND NOT ${CMAKE_GENERATOR} MATCHES "Xcode")
        add_custom_command(
            TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy
                    "${CMAKE_CURRENT_BINARY_DIR}/${CMAKE_CFG_INTDIR}/$<TARGET_FILE_NAME:${target_name}>"
                    "${CMAKE_CURRENT_SOURCE_DIR}/TP1/"
        )
    endif()
endfunction()

if(NOT ${CMAKE_GENERATOR} MATCHES "Xcode")
    add_custom_target(glutony_sync_editor_runtime_assets ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_CURRENT_SOURCE_DIR}/TP1/img"
                "${CMAKE_CURRENT_BINARY_DIR}/img"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_CURRENT_SOURCE_DIR}/TP1/vertex_shader.glsl"
                "${CMAKE_CURRENT_BINARY_DIR}/vertex_shader.glsl"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${CMAKE_CURRENT_SOURCE_DIR}/TP1/fragment_shader.glsl"
                "${CMAKE_CURRENT_BINARY_DIR}/fragment_shader.glsl"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/TP1/.pipeline_cache"
            "${CMAKE_CURRENT_BINARY_DIR}/.pipeline_cache"
        COMMAND ${CMAKE_COMMAND} -E make_directory
                "${CMAKE_CURRENT_BINARY_DIR}/Assets"
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_CURRENT_SOURCE_DIR}/TP1/built-in"
                "${CMAKE_CURRENT_BINARY_DIR}/built-in"
        COMMAND ${CMAKE_COMMAND} -E rm -f
                "${CMAKE_CURRENT_BINARY_DIR}/built-in/materials/lit_default.material.json"
                "${CMAKE_CURRENT_BINARY_DIR}/built-in/materials/selection_highlight.material.json"
        COMMENT "Synchronizing editor runtime assets into the build directory"
    )
endif()

# TP1
add_executable(TP1
    TP1/TP1.cpp
    TP1/vertex_shader.glsl
    TP1/fragment_shader.glsl
)

add_executable(runtime_game EXCLUDE_FROM_ALL
    TP1/player_main.cpp
    TP1/vertex_shader.glsl
    TP1/fragment_shader.glsl
)

if(ENABLE_LSAN)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(TP1 PRIVATE -fsanitize=leak -fno-omit-frame-pointer -g)
        target_link_options(TP1 PRIVATE -fsanitize=leak)
    else()
        message(WARNING "ENABLE_LSAN is only supported with Clang/GCC")
    endif()
endif()

target_link_libraries(TP1 PRIVATE app_common editor_support gameplay_sources)
target_link_libraries(runtime_game PRIVATE app_common engine_core gameplay_sources)

if(WIN32)
    target_link_libraries(TP1 PRIVATE Comdlg32)
    target_link_libraries(runtime_game PRIVATE Comdlg32)
endif()

# Si RmlUi est lié statiquement, il faut cette définition côté client
if(NOT BUILD_SHARED_LIBS)
    target_compile_definitions(TP1 PRIVATE RMLUI_STATIC_LIB)
endif()

if(TARGET glutony_sync_editor_runtime_assets)
    add_dependencies(TP1 glutony_sync_editor_runtime_assets)
    add_dependencies(runtime_game glutony_sync_editor_runtime_assets)
endif()

glutony_configure_runtime_assets(TP1 "${CMAKE_CURRENT_SOURCE_DIR}/TP1/")
glutony_configure_runtime_assets(runtime_game "${CMAKE_CURRENT_BINARY_DIR}")

SOURCE_GROUP(common REGULAR_EXPRESSION ".*/common/.*")
SOURCE_GROUP(shaders REGULAR_EXPRESSION ".*/.*shader$")